#!/bin/bash
# Alex Bettarini - 22 Jan 2019
# 2026-08-04: download to a staging area, validate, and only then overwrite
#             the files in input/zurrose. A bad/truncated/reshaped feed must
#             never clobber the last known-good input.
#             Transport switched from scp to sftp -b: OpenSSH 9+ scp exits 1
#             even on a successful transfer from the Serv-U server at Zur Rose,
#             so its exit code could not be used to detect a failed download.
#
# Usage:
#   ./download_zr.sh [test]
#
# Environment knobs:
#   ZR_SHRINK_PCT=70   fail if the new file is smaller than this % of the current one
#   ZR_FORCE=1         promote files even if validation failed (still reports)
#   ZR_DRY_RUN=1       download and validate, but keep the live files untouched
#   ZR_CHECK_ONLY=1    don't download, just validate the files already in input/zurrose
#   ZR_NO_VALIDATE=1   old behaviour: download straight over the live files
#   ZR_KEEP_STAGING=1  keep .staging/ after the run (for inspection)
#   ZR_VERBOSE=1       echo the full sftp session
#   ZR_SSH_STRICT=...  StrictHostKeyChecking value (default accept-new)
#
# Exit status: 0 = every file downloaded and validated
#              1 = at least one file was rejected, the rest were updated
#              2 = the server could not be reached, nothing was touched at all
# The caller (cron/build script) must check it before running ./zurrose.

set -uo pipefail

WD=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
SRC_DIR=$(realpath "$WD/..")

# Hosts live in zr_targets.conf, which is not in git; the account and password
# for the server stay in scripts/passwords, also not in git.
ZR_CONF=${ZR_CONF:-$WD/zr_targets.conf}
if [ -f "$ZR_CONF" ]; then
    source "$ZR_CONF"
else
    echo "no $ZR_CONF - copy zr_targets.conf.example and fill it in" >&2
    exit 1
fi
DOMAIN=${ZR_DOMAIN:-}
[ -n "$DOMAIN" ] || { echo "ZR_DOMAIN is not set (see $ZR_CONF)" >&2; exit 1; }
if [ "test" == "${1:-}" ] ; then
    DIR="/ywesee OutTest"
else
    DIR="/ywesee Out"
fi

BLD_DIR=$SRC_DIR/build
BIN_DIR=$BLD_DIR

SHRINK_PCT=${ZR_SHRINK_PCT:-70}
CHECK_ONLY=${ZR_CHECK_ONLY:-0}
DRY_RUN=${ZR_DRY_RUN:-0}
NO_VALIDATE=${ZR_NO_VALIDATE:-0}
FORCE=${ZR_FORCE:-0}

#-------------------------------------------------------------------------------
# zurrose

source "$WD/passwords"
ZURROSE_DIR="${SRC_DIR}/input/zurrose"
# Staging is per run ($$), so two concurrent downloads - the every-10-minutes
# job overlapping the nightly one - cannot wipe each other's half-fetched files.
STAGING_DIR="${ZURROSE_DIR}/.staging.$$"
BACKUP_DIR="${ZURROSE_DIR}/.bak"

mkdir -p "$ZURROSE_DIR" "$STAGING_DIR" "$BACKUP_DIR"

cleanup() {
    # nothing staged (e.g. the server was unreachable) - nothing worth keeping
    if [ -z "$(ls -A "$STAGING_DIR" 2>/dev/null | grep -v '^\.')" ]; then
        rm -rf "$STAGING_DIR"
        return
    fi
    if [ "${ZR_KEEP_STAGING:-0}" == "1" ] || [ "$DRY_RUN" == "1" ]; then
        log "staged files kept in $STAGING_DIR"
    else
        rm -rf "$STAGING_DIR"
    fi
}
trap cleanup EXIT INT TERM

# Sweep up after runs that were killed before their trap could fire
find "$ZURROSE_DIR" -maxdepth 1 -name '.staging.*' -type d -mmin +240 \
     -exec rm -rf {} + 2>/dev/null

#-------------------------------------------------------------------------------
# File specification table
#
#   remote name | local name | iconv from | min bytes | min lines | column rule
#
# The column rule mirrors what src/zur/*.cpp actually requires; most parsers
# call exit(EXIT_FAILURE) on a mismatch, and voll.cpp::parseCSV silently skips
# the line, which empties the output DB without any error message.
#
#   N       every line must split into exactly N ';' fields (strict parsers)
#   N~      the most common field count must be N, and >=90% of the lines
#           must have it (files with a multi-line quoted header)
#   nota    (fields - 1) % 5 == 0 on every line       (nota.cpp:71)
#   galen   every non-empty line is "<digits> <text>" (galen.cpp:44)
#   -       no column check
#
FILE_SPECS=(
    # Updated once per day                                     stamm.cpp:69, voll.cpp:362 -> exit()
    "Artikelstamm.csv|artikel_stamm_zurrose.csv||800000|4000|22"
    # Updated every 4 hours                                    stamm.cpp:196 -> exit()
    "Artikelstamm_Voigt.csv|artikel_stamm_voigt.csv||200000|20000|2"
    # Updated every 30 minutes                                 stamm.cpp:129 -> exit()
    "Artikelstamm_Vollstamm.csv|artikel_vollstamm_zurrose.csv|ISO-8859-1|20000000|100000|22"
    #                                                          direkt.cpp:60 -> exit()
    "direktsubstitution.csv|direct_subst_zurrose.csv||50|5|2"
    #                                                          nota.cpp:71 -> exit()
    "Nota.csv|nota_zurrose.csv||100000|500|nota"
    #                                                          galen.cpp:44 -> std::stoi throws
    "Vollstamm_Galenic_Form_Mapping_by_Code.txt|galenic_codes_map_zurrose.txt||1000|50|galen"
    #                                                          generika.cpp:94 -> exit()
    "Autogenerika.csv|Autogenerika.csv||2000|20|15~"
    #                                                          neu.cpp:154 -> tolerated, recovers
    "kunden_alle_dynamics_ce.csv|kunden_alle_dynamics_ce.csv|ISO-8859-1|150000|1000|8~"
    #                                                          neu.cpp:271 -> tolerated, skips
    "medix_kunden.csv|medix_kunden.csv||500|5|6~"
    # Frequently empty by design                               gripp.cpp:52
    "Grippeimpfstoff.csv|Grippeimpfstoff.csv||0|0|-"
    #                                                          stamm.cpp:285 -> exit()
    "19er_pharmaCodes.csv|19er_pharmaCodes.csv||500000|50000|1"
)

#-------------------------------------------------------------------------------

RESULT_LINES=()
N_FAILED=0
N_OK=0

log()  { echo "[zr] $*"; }
fail() { echo "[zr] FAIL $*" >&2; }

# Transport notes
#
# sftp, not scp: against the Serv-U server at Zur Rose, OpenSSH 9+ scp exits 1
# even on a perfectly good transfer ("Exit status -1"), so its exit code cannot
# gate anything. sftp -b returns 0/1/255 properly and reports the remote size,
# which is what the completeness check compares against.
#
# One session for all the files, not one per file. The server rate-limits new
# connections: two runs overlapping used to open dozens of sessions between them
# and got refused with rc=255 halfway through. A batch prefixed with '-' does
# not abort on the first missing file, so one session can fetch everything and
# report per-file what worked.

SFTP_OPTS=(-oBatchMode=no -o "StrictHostKeyChecking=${ZR_SSH_STRICT:-accept-new}"
           -o ConnectTimeout=30)

# sftp_session <batch file> - prints the transcript, returns sftp's exit code
sftp_session() {
    sshpass -p "$PASSWORD_ZUR" sftp "${SFTP_OPTS[@]}" -b "$1" "$USERNAME_ZUR@$DOMAIN" 2>&1
}

# sftp_ping - prove we can reach the server and log in, before touching anything
sftp_ping() {
    local batch="$STAGING_DIR/.sftp_ping" attempt out rc
    printf 'pwd\n' > "$batch"
    for attempt in 1 2 3; do
        out=$(sftp_session "$batch")
        rc=$?
        [ "${ZR_VERBOSE:-0}" == "1" ] && echo "$out"
        [ $rc -eq 0 ] && return 0
        log "cannot reach $DOMAIN (attempt $attempt/3, rc=$rc)"
        echo "$out" | grep -v '^sftp>' | tail -2 >&2
        [ $attempt -lt 3 ] && sleep 15
    done
    return 1
}

# remote_size <transcript> <remote name> - what the server said the file is
remote_size() {
    awk -v suf="/$2" '
        /^-/ && substr($0, length($0) - length(suf) + 1) == suf && $5 ~ /^[0-9]+$/ {
            print $5; exit
        }' <<< "$1"
}

# fetch_all <remote>:<destination>...
#
# Downloads everything in one session, then retries only what did not arrive
# complete. Fills FETCHED[<remote>] with "ok" or a reason.
declare -A FETCHED
fetch_all() {
    local todo=("$@") round batch out item remote dest rsize lsize next=()

    for round in 1 2 3; do
        [ ${#todo[@]} -eq 0 ] && break

        batch="$STAGING_DIR/.sftp_batch"
        : > "$batch"
        for item in "${todo[@]}"; do
            remote=${item%%:*}
            dest=${item#*:}
            rm -f "$dest"
            printf -- '-ls -l "%s/%s"\n-get "%s/%s" "%s"\n' \
                   "$DIR" "$remote" "$DIR" "$remote" "$dest" >> "$batch"
        done

        [ $round -gt 1 ] && log "retrying ${#todo[@]} file(s), round $round/3"
        out=$(sftp_session "$batch")
        [ "${ZR_VERBOSE:-0}" == "1" ] && echo "$out"

        next=()
        for item in "${todo[@]}"; do
            remote=${item%%:*}
            dest=${item#*:}

            if [ ! -f "$dest" ]; then
                FETCHED[$remote]="not downloaded"
                next+=("$item")
                continue
            fi

            rsize=$(remote_size "$out" "$remote")
            lsize=$(stat -c%s "$dest")
            if [ -n "$rsize" ] && [ "$lsize" != "$rsize" ]; then
                FETCHED[$remote]="incomplete transfer, got $lsize of $rsize bytes"
                next+=("$item")
                continue
            fi

            FETCHED[$remote]="ok"
        done

        todo=("${next[@]}")
        [ ${#todo[@]} -gt 0 ] && [ $round -lt 3 ] && sleep 10
    done
}

# check_columns <file> <rule> -> 0 ok, 1 bad; prints a diagnostic on stdout
check_columns() {
    local file="$1" rule="$2"

    case "$rule" in
    -)
        return 0
        ;;
    nota)
        awk -F';' '
            NF > 0 && (NF - 1) % 5 != 0 { bad++; if (bad <= 3) msg = msg " line " NR " has " NF " fields;" }
            END { if (bad) { printf "%d line(s) not 5n+1 fields:%s\n", bad, msg; exit 1 } }
        ' "$file"
        ;;
    galen)
        awk '
            length($0) == 0 { next }
            $0 !~ /^[0-9]+ ./ { bad++; if (bad <= 3) msg = msg " line " NR ";" }
            END { if (bad) { printf "%d line(s) not \"<code> <form>\":%s\n", bad, msg; exit 1 } }
        ' "$file"
        ;;
    *~)
        # tolerant: the modal field count must be the expected one
        local want=${rule%\~}
        awk -F';' -v want="$want" '
            { c[NF]++; total++ }
            END {
                for (k in c) if (c[k] > best) { best = c[k]; mode = k }
                if (mode != want) {
                    printf "expected %s columns, feed has %s (%d/%d lines)\n", want, mode, best, total
                    exit 1
                }
                if (best * 100 < total * 90) {
                    printf "only %d of %d lines have %s columns\n", best, total, want
                    exit 1
                }
            }
        ' "$file"
        ;;
    *)
        # strict: every line, header included
        awk -F';' -v want="$rule" '
            NF != want { c[NF]++; bad++; if (bad <= 3) msg = msg " line " NR " has " NF ";" }
            END { if (bad) { printf "expected %s columns, %d bad line(s):%s\n", want, bad, msg; exit 1 } }
        ' "$file"
        ;;
    esac
}

is_utf8() { iconv -f UTF-8 -t UTF-8 "$1" >/dev/null 2>&1; }

# validate <staged file> <live file> <min bytes> <min lines> <column rule> <converted?>
validate() {
    local new="$1" live="$2" min_bytes="$3" min_lines="$4" col_rule="$5" converted="${6:-}"
    local size lines old_size floor diag new_enc old_enc

    if [ ! -f "$new" ]; then
        fail "$(basename "$new"): not downloaded"
        return 1
    fi

    size=$(stat -c%s "$new")
    if [ "$size" -lt "$min_bytes" ]; then
        fail "$(basename "$new"): $size bytes, below the $min_bytes byte floor"
        return 1
    fi

    # An FTP/HTTP error page or an HTML login form instead of the CSV
    if [ "$size" -gt 0 ] && head -c 512 "$new" | grep -qi '<html\|<!doctype\|<?xml'; then
        fail "$(basename "$new"): looks like markup, not a data feed"
        return 1
    fi

    lines=$(wc -l < "$new")
    if [ "$lines" -lt "$min_lines" ]; then
        fail "$(basename "$new"): $lines lines, below the $min_lines line floor"
        return 1
    fi

    # Shrink guard: a feed that suddenly loses a third of its rows is
    # far more likely to be a broken export than a real change.
    if [ -f "$live" ] && [ "$size" -gt 0 ]; then
        old_size=$(stat -c%s "$live")
        floor=$(( old_size * SHRINK_PCT / 100 ))
        if [ "$size" -lt "$floor" ]; then
            fail "$(basename "$new"): $size bytes vs $old_size currently ( < ${SHRINK_PCT}% ), refusing to overwrite"
            return 1
        fi
    fi

    # No NUL bytes: the parsers read these with std::getline as text
    if [ "$size" -gt 0 ] && ! LC_ALL=C tr -d '\000' < "$new" | cmp -s - "$new"; then
        fail "$(basename "$new"): contains NUL bytes, not a text feed"
        return 1
    fi

    # Encoding. The files we run through iconv must come out as valid UTF-8.
    # The others are shipped by Zur Rose as ISO-8859-1 (artikel_stamm_zurrose.csv,
    # Autogenerika.csv, medix_kunden.csv) or plain ASCII and are consumed as-is,
    # so we only insist that the encoding does not silently flip: that would
    # garble every Umlaut downstream without any parser noticing.
    if [ -n "$converted" ]; then
        if ! is_utf8 "$new"; then
            fail "$(basename "$new"): not valid UTF-8 after conversion"
            return 1
        fi
    elif [ -f "$live" ] && [ "$size" -gt 0 ]; then
        if is_utf8 "$new"; then new_enc="utf-8"; else new_enc="8-bit"; fi
        if is_utf8 "$live"; then old_enc="utf-8"; else old_enc="8-bit"; fi
        if [ "$new_enc" != "$old_enc" ]; then
            fail "$(basename "$new"): encoding changed ($old_enc -> $new_enc), refusing to overwrite"
            return 1
        fi
    fi

    if ! diag=$(check_columns "$new" "$col_rule"); then
        fail "$(basename "$new"): $diag"
        fail "$(basename "$new"): if Zur Rose changed the layout, update the column guards in src/zur/ and the spec table in this script"
        return 1
    fi

    return 0
}

# promote <staged file> <live file>
promote() {
    local new="$1" live="$2"
    if [ "$DRY_RUN" == "1" ]; then
        log "dry run: not replacing $live"
        return 0
    fi
    local name=$(basename "$live")
    if [ -f "$live" ]; then
        cp -p "$live" "$BACKUP_DIR/.$name.$$" &&
            mv -f "$BACKUP_DIR/.$name.$$" "$BACKUP_DIR/$name"
    fi
    mv -f "$new" "$live"
}

# Sourcing with ZR_LIB=1 defines the helpers above without downloading anything,
# so the checks can be exercised on hand-made samples.
[ "${ZR_LIB:-0}" == "1" ] && return 0

#-------------------------------------------------------------------------------
# Legacy path: straight overwrite, no staging (ZR_NO_VALIDATE=1)

if [ "$NO_VALIDATE" == "1" ]; then
    log "ZR_NO_VALIDATE=1: downloading directly over the live files"
fi

#-------------------------------------------------------------------------------
# Phase 1: prove the connection works, then fetch everything in one session.
# Nothing on disk is touched until this succeeds.

if [ "$CHECK_ONLY" != "1" ]; then
    log "checking the connection to $DOMAIN"
    if ! sftp_ping; then
        echo
        echo "Cannot reach $DOMAIN - no file was touched, the previous inputs" >&2
        echo "are all still in place. Nothing to clean up." >&2
        exit 2
    fi

    TODO=()
    for spec in "${FILE_SPECS[@]}"; do
        IFS='|' read -r remote local_name from_charset min_bytes min_lines col_rule <<< "$spec"
        if [ -n "$from_charset" ]; then
            TODO+=("$remote:$STAGING_DIR/$local_name.raw")
        else
            TODO+=("$remote:$STAGING_DIR/$local_name")
        fi
    done

    log "fetching ${#TODO[@]} files in one sftp session"
    fetch_all "${TODO[@]}"
fi

#-------------------------------------------------------------------------------
# Phase 2: convert, validate and promote what arrived

for spec in "${FILE_SPECS[@]}"; do
    IFS='|' read -r remote local_name from_charset min_bytes min_lines col_rule <<< "$spec"

    live="$ZURROSE_DIR/$local_name"
    staged="$STAGING_DIR/$local_name"

    if [ "$CHECK_ONLY" == "1" ]; then
        if validate "$live" "" "$min_bytes" "$min_lines" "$col_rule" "$from_charset"; then
            RESULT_LINES+=("  ok       $local_name")
            N_OK=$((N_OK + 1))
        else
            RESULT_LINES+=("  INVALID  $local_name")
            N_FAILED=$((N_FAILED + 1))
        fi
        continue
    fi

    if [ "${FETCHED[$remote]:-missing}" != "ok" ]; then
        fail "$local_name: ${FETCHED[$remote]:-missing}, keeping the current file"
        RESULT_LINES+=("  DOWNLOAD $local_name (kept previous)")
        N_FAILED=$((N_FAILED + 1))
        continue
    fi

    if [ -n "$from_charset" ]; then
        raw="$STAGING_DIR/$local_name.raw"
        if ! iconv -f "$from_charset" -t UTF-8 "$raw" > "$staged"; then
            fail "$local_name: $from_charset -> UTF-8 conversion failed, keeping the current file"
            RESULT_LINES+=("  ICONV    $local_name (kept previous)")
            N_FAILED=$((N_FAILED + 1))
            continue
        fi
        rm -f "$raw"
    fi

    if [ "$NO_VALIDATE" == "1" ]; then
        promote "$staged" "$live"
        RESULT_LINES+=("  ok       $local_name (not validated)")
        N_OK=$((N_OK + 1))
        continue
    fi

    if validate "$staged" "$live" "$min_bytes" "$min_lines" "$col_rule" "$from_charset"; then
        nlines=$(wc -l < "$staged")
        promote "$staged" "$live"
        RESULT_LINES+=("  ok       $local_name ($nlines lines)")
        N_OK=$((N_OK + 1))
    elif [ "$FORCE" == "1" ]; then
        promote "$staged" "$live"
        RESULT_LINES+=("  FORCED   $local_name (validation failed, promoted anyway)")
        N_FAILED=$((N_FAILED + 1))
    else
        RESULT_LINES+=("  REJECTED $local_name (kept previous)")
        N_FAILED=$((N_FAILED + 1))
    fi
done

echo
echo "Zur Rose download summary ($ZURROSE_DIR)"
for line in "${RESULT_LINES[@]}"; do echo "$line"; done
echo "  --------"
echo "  $N_OK ok, $N_FAILED failed"

if [ "$N_FAILED" -gt 0 ]; then
    echo
    echo "At least one file was rejected. The previous copies are still in place;" >&2
    echo "the last promoted versions are backed up in $BACKUP_DIR." >&2
    echo "Do NOT publish a build made from a partially updated input set." >&2
    exit 1
fi

exit 0
