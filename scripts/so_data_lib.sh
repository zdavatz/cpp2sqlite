# Shared verification and publishing helpers for so_data and so_data_full.
# Sourced, not executed.
#
# Environment knobs (both callers):
#   ZR_SRC=/usr/local/src/cpp2sqlite       source tree
#   ZR_WWW=/var/www/so.zurrose.ch/rose     publish target
#   ZR_SHRINK_PCT=70                       refuse a file that lost more than 30%
#   ZR_SKIP_DOWNLOAD=1                     use the inputs already in input/
#   ZR_SKIP_BUILD=1                        verify and publish the existing output/
#   ZR_NO_PUBLISH=1                        build and verify, publish nothing

SRC=${ZR_SRC:-/usr/local/src/cpp2sqlite}
WWW=${ZR_WWW:-/var/www/so.zurrose.ch/rose}
SHRINK_PCT=${ZR_SHRINK_PCT:-70}
STARTED_AT=$(date +%s)

step() { echo; echo "=== $* ==="; }
die()  { echo; echo "ABORT: $*" >&2; echo "Nothing was published." >&2; exit 1; }

# verify_outputs <spec>...
#
# Each spec is  name | min bytes | min rows (or -) | kind
# where kind is one of sqlite, json, csv. Nothing may go live that was not
# written by this run, that is suspiciously small, or that does not parse.
verify_outputs() {
    local spec name min_bytes min_rows kind mtime size old_size floor check rows lines

    cd "$SRC/output" || die "no $SRC/output"

    for spec in "$@"; do
        IFS='|' read -r name min_bytes min_rows kind <<< "$spec"

        [ -f "$name" ] || die "$name was not generated"

        # Must have been written by this run, not left over from the last one
        mtime=$(stat -c%Y "$name")
        [ "$mtime" -ge "$STARTED_AT" ] || die "$name is stale (not rewritten by this run)"

        size=$(stat -c%s "$name")
        [ "$size" -ge "$min_bytes" ] || die "$name is $size bytes, below the $min_bytes byte floor"

        # Do not replace a good published file with a much smaller one
        if [ -f "$WWW/$name" ]; then
            old_size=$(stat -c%s "$WWW/$name")
            floor=$(( old_size * SHRINK_PCT / 100 ))
            [ "$size" -ge "$floor" ] || \
                die "$name is $size bytes vs $old_size published ( < ${SHRINK_PCT}% )"
        fi

        case "$kind" in
        sqlite)
            if command -v sqlite3 >/dev/null; then
                check=$(sqlite3 "$name" "PRAGMA integrity_check;" 2>&1)
                [ "$check" == "ok" ] || die "$name failed sqlite integrity_check: $check"
                rows=$(sqlite3 "$name" "SELECT count(*) FROM rosedb;" 2>&1)
                [[ "$rows" =~ ^[0-9]+$ ]] || die "$name has no readable rosedb table: $rows"
                [ "$rows" -ge "$min_rows" ] || die "$name has $rows rows, expected at least $min_rows"
                echo "  ok  $name ($size bytes, $rows rows)"
            else
                echo "  ??  $name ($size bytes, sqlite3 not installed - integrity not checked)"
            fi
            ;;
        json)
            if command -v jq >/dev/null; then
                jq -e . "$name" >/dev/null 2>&1 || die "$name is not well-formed JSON"
            elif command -v python3 >/dev/null; then
                python3 -c "import json,sys; json.load(open(sys.argv[1]))" "$name" \
                    || die "$name is not well-formed JSON"
            fi
            echo "  ok  $name ($size bytes)"
            ;;
        csv)
            lines=$(wc -l < "$name")
            [ "$lines" -ge "$min_rows" ] || die "$name has $lines lines, expected at least $min_rows"
            echo "  ok  $name ($size bytes, $lines lines)"
            ;;
        *)
            die "unknown check kind '$kind' for $name"
            ;;
        esac
    done
}

# Canaries: a build can pass every structural check above and still answer an
# order with nothing at all, because the one article or customer being asked
# about is missing. These check a real basket and a real customer against the
# freshly built files, before they go live. A miss aborts the run.

# canary_db <db file> <pharmacode>...
canary_db() {
    local db="$1" p n; shift
    if ! command -v sqlite3 >/dev/null; then
        echo "  ??  canary skipped for $db (sqlite3 not installed)"
        return 0
    fi
    for p in "$@"; do
        n=$(sqlite3 "$db" "SELECT count(*) FROM rosedb WHERE pharmacode='$p';" 2>&1)
        [[ "$n" =~ ^[0-9]+$ ]] || die "canary query on $db failed: $n"
        [ "$n" -ge 1 ] || die "$db has no row for pharmacode $p - an order containing it would come back empty"
    done
    echo "  ok  canary: $# pharmacodes present in $db"
}

# canary_csv <csv file> <value in column 1>...
canary_csv() {
    local file="$1" v; shift
    for v in "$@"; do
        grep -q "^$v;" "$file" || die "$file has no line for pharmacode $v"
    done
    echo "  ok  canary: $# pharmacodes present in $file"
}

# canary_json_key <json object file> <key>...
canary_json_key() {
    local file="$1" k; shift
    if ! command -v jq >/dev/null; then
        echo "  ??  canary skipped for $file (jq not installed)"
        return 0
    fi
    for k in "$@"; do
        jq -e --arg k "$k" 'has($k)' "$file" >/dev/null 2>&1 || \
            die "$file has no entry for $k - that customer would get an empty answer"
    done
    echo "  ok  canary: $# keys present in $file"
}

# publish_outputs <spec>...
publish_outputs() {
    local spec name

    if [ "${ZR_NO_PUBLISH:-0}" == "1" ]; then
        echo "ZR_NO_PUBLISH=1, stopping before the copy"
        exit 0
    fi

    [ -d "$WWW" ] || die "$WWW does not exist"
    cd "$SRC/output" || die "no $SRC/output"

    for spec in "$@"; do
        name=${spec%%|*}
        # copy first, rename second: readers see either the old or the new file,
        # never a partially written one
        cp "$name" "$WWW/.$name.tmp" || die "could not copy $name to $WWW"
        mv -f "$WWW/.$name.tmp" "$WWW/$name" || die "could not install $name in $WWW"
        echo "  published $name"
    done
}
