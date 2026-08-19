# Shared verification and publishing helpers for amiko_data.
# Sourced, not executed.
#
# Deliberately separate from so_data_lib.sh: that one belongs to the other
# publishing pipeline in this repository and checks a different set of files.
# Keeping them apart means a change here cannot break that one, and neither
# carries checks the other does not need.
#
# Hosts and directories are not named here. They live in amiko_targets.conf,
# which is not in git; amiko_targets.conf.example documents the format.
#
# Environment knobs:
#   AMIKO_SRC=/usr/local/src/cpp2sqlite   source tree
#   AMIKO_CONF=<path>                     targets file (default: next to this one)
#   AMIKO_WWW=<dir>                       local publish target
#   AMIKO_AI=<dir>                        fachinfo_ai checkout
#   AMIKO_SHRINK_PCT=70                   refuse a file that lost more than 30%
#   AMIKO_SKIP_DOWNLOAD=1                 use the inputs already in input/
#   AMIKO_SKIP_BUILD=1                    verify and publish the existing output/
#   AMIKO_NO_PUBLISH=1                    build and verify, publish nothing
#   AMIKO_NO_REMOTE=1                     publish to $AMIKO_WWW only

SRC=${AMIKO_SRC:-/usr/local/src/cpp2sqlite}
SHRINK_PCT=${AMIKO_SHRINK_PCT:-70}

AMIKO_REMOTES=()
CONF=${AMIKO_CONF:-${BASH_SOURCE[0]%/*}/amiko_targets.conf}   # no external dirname: cron PATH is minimal
if [ -f "$CONF" ]; then
    source "$CONF"
else
    echo "no $CONF - copy amiko_targets.conf.example and fill it in" >&2
    exit 1
fi

WWW=${AMIKO_WWW:-}
AI=${AMIKO_AI:-}
[ -n "$WWW" ] || { echo "AMIKO_WWW is not set (see $CONF)" >&2; exit 1; }

# The row and integrity checks are the whole point of this pipeline, so a
# missing sqlite3 is a hard error rather than a skipped check. Under cron the
# PATH is minimal, which is exactly where a silent "not checked" would hide.
command -v sqlite3 >/dev/null || { echo "sqlite3 is not on PATH ($PATH)" >&2; exit 1; }

OUT="$SRC/output"
STARTED_AT=$(date +%s)
WARNINGS=0
SSH_OPTS="-o BatchMode=yes -o ConnectTimeout=20"

step() { echo; echo "=== $* ==="; }
die()  { echo; echo "ABORT: $*" >&2; echo "Nothing further was published." >&2; exit 1; }
warn() { echo "WARNING: $*" >&2; WARNINGS=$((WARNINGS + 1)); }

# verify_outputs <dir> <spec>...
#
# Each spec is  name | min bytes | min rows (or -) | table (or - for non-sqlite)
#
# Nothing may go live that was not written by this run, that is suspiciously
# small, that lost a large part of its published size, or whose database does
# not open. The row floor is what would have caught the 2026-08-19 incident:
# an incompletely extracted AllHtml.zip left amiko_db_full_idx_de.db with
# 1'694 instead of 4'640 rows and it was published to four servers anyway.
verify_outputs() {
    local dir="$1"; shift
    local spec name min_bytes min_rows table path mtime size old floor check rows

    for spec in "$@"; do
        IFS='|' read -r name min_bytes min_rows table <<< "$spec"
        path="$dir/$name"

        [ -f "$path" ] || die "$name was not generated"

        # Must have been written by this run, not left over from the last one
        mtime=$(stat -c%Y "$path")
        [ "$mtime" -ge "$STARTED_AT" ] || die "$name is stale (not rewritten by this run)"

        size=$(stat -c%s "$path")
        [ "$size" -ge "$min_bytes" ] || die "$name is $size bytes, below the $min_bytes byte floor"

        # Do not replace a good published file with a much smaller one
        if [ -f "$WWW/$name" ]; then
            old=$(stat -c%s "$WWW/$name")
            floor=$(( old * SHRINK_PCT / 100 ))
            [ "$size" -ge "$floor" ] || \
                die "$name is $size bytes vs $old published ( < ${SHRINK_PCT}% )"
        fi

        if [ "$table" == "-" ]; then
            echo "  ok  $name ($size bytes)"
            continue
        fi

        check=$(sqlite3 "$path" "PRAGMA integrity_check;" 2>&1)
        [ "$check" == "ok" ] || die "$name failed sqlite integrity_check: $check"

        rows=$(sqlite3 "$path" "SELECT count(*) FROM \"$table\";" 2>&1)
        [[ "$rows" =~ ^[0-9]+$ ]] || die "$name has no readable $table table: $rows"
        [ "$rows" -ge "$min_rows" ] || \
            die "$name has $rows rows in $table, expected at least $min_rows"

        echo "  ok  $name ($size bytes, $rows rows)"
    done
}

# canary_regnr <db file> <registration number>...
#
# A build can clear every check above and still have lost whole medicines: a
# missing content html silently drops the medicine and all its packages
# (src/c2s/aips.cpp, see CLAUDE.md). These are registrations that have to be
# in every language database.
canary_regnr() {
    local db="$1" name r n; shift
    name=$(basename "$db")

    for r in "$@"; do
        n=$(sqlite3 "$db" "SELECT count(*) FROM amikodb WHERE regnrs LIKE '%$r%';" 2>&1)
        [[ "$n" =~ ^[0-9]+$ ]] || die "canary query on $name failed: $n"
        [ "$n" -ge 1 ] || \
            die "$name has no row for registration $r - a whole medicine went missing"
    done
    echo "  ok  canary: $# registrations present in $name"
}

# zip_output <dir> <zip name> <file name>
#
# The archive is removed first: zip updates an existing one in place, so a
# stale entry from a previous run would survive. This replaces the "rm *.zip"
# that used to sit at the top of three different scp scripts and had them
# deleting each other's archives.
zip_output() {
    local dir="$1" zip="$2" file="$3"

    cd "$dir" || die "no $dir"
    rm -f "$zip"
    zip -q "$zip" "$file" || die "could not zip $file"
    echo "  zipped $file -> $zip"
}

# publish_local <dir> <file> [name to publish it as]
#
# copy first, rename second: a client sees either the old or the new file,
# never a half-written one.
publish_local() {
    local dir="$1" file="$2" as="${3:-$2}"

    if [ "${AMIKO_NO_PUBLISH:-0}" == "1" ]; then
        echo "  AMIKO_NO_PUBLISH=1, not copying $file"
        return 0
    fi

    [ -d "$WWW" ]        || die "$WWW does not exist"
    [ -f "$dir/$file" ]  || die "$dir/$file disappeared before it could be published"

    cp "$dir/$file" "$WWW/.$as.tmp" || die "could not copy $file to $WWW"
    mv -f "$WWW/.$as.tmp" "$WWW/$as" || die "could not install $as in $WWW"
    echo "  published $as -> $WWW"
}

# publish_remote <user@host|remote dir|port> <local dir> <file>...
#
# Same two-step install as publish_local, over ssh. A host being down is a
# warning, not an abort: the other targets still get the build, and the run
# exits non-zero at the end so cron mails about it.
publish_remote() {
    local spec="$1" dir="$2"; shift 2
    local host rdir port f rc=0

    IFS='|' read -r host rdir port <<< "$spec"
    port=${port:-22}

    if [ "${AMIKO_NO_PUBLISH:-0}" == "1" ] || [ "${AMIKO_NO_REMOTE:-0}" == "1" ]; then
        echo "  not publishing to $host (AMIKO_NO_PUBLISH/AMIKO_NO_REMOTE)"
        return 0
    fi

    if ! timeout 60 ssh $SSH_OPTS -p "$port" "$host" true >/dev/null 2>&1 ; then
        warn "$host port $port is unreachable - nothing was published there"
        return 1
    fi

    for f in "$@"; do
        if [ ! -f "$dir/$f" ]; then
            warn "$dir/$f does not exist - not published to $host"
            rc=1; continue
        fi
        if ! scp $SSH_OPTS -P "$port" -q "$dir/$f" "$host:$rdir/.$f.tmp" ; then
            warn "could not copy $f to $host"
            rc=1; continue
        fi
        if ! ssh $SSH_OPTS -p "$port" "$host" "mv -f '$rdir/.$f.tmp' '$rdir/$f'" ; then
            warn "could not install $f on $host"
            rc=1; continue
        fi
        echo "  published $f -> $host:$rdir"
    done
    return $rc
}

# mark_done <marker file>
#
# The marker says "this update is complete", so it is written last, after
# every file it refers to is in place. Clients poll it.
mark_done() {
    local marker="$1"

    touch "$OUT/$marker" || die "could not create $OUT/$marker"
    publish_local "$OUT" "$marker"
}

finish() {
    echo
    if [ "$WARNINGS" -gt 0 ]; then
        echo "Done, but with $WARNINGS warning(s) - see above."
        exit 1
    fi
    echo "Done."
}
