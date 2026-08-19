# cpp2sqlite
C++ tool to generate sqlite database containing Swiss Healthcare Public Domain Drug Information
## History
This tool is a port of [aips2sqlite](https://github.com/zdavatz/aips2sqlite)
## Requirements

- Boost (BOOST_BIND_GLOBAL_PLACEHOLDERS is defined to suppress deprecated bind placeholder warnings)
- C++17
- sqlite
- cmake
- gcc-9
- g++-9 `sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-9 9`
- [xlnt](https://github.com/tfussell/xlnt) with `cmake -DSTATIC=on`, also apply [swissmedic.patch](https://github.com/zdavatz/cpp2sqlite/files/3584890/swissmedic.patch.txt)
- jq (Command-line JSON processor)
- [json](https://github.com/nlohmann/json.git)
- xmllint

## Installation
$ git clone\
$ git submodule init\
$ git submodule update

The remaining submodules are `json` and `xlnt`. The EAN13 barcode generator
used to be one as well; since 2026-08-19 `src/c2s/ean13/` is vendored in-tree
(see the README there), so a fix to it is a single ordinary commit.

See [build.sh](https://github.com/zdavatz/cpp2sqlite/blob/master/scripts/build.sh)
## Usage
### cpp2sqlite
`./cpp2sqlite --inDir ~/.software/cpp2sqlite/input`
### deepl translation
_ run `build/sappinfo` to generate the unique language file.\
_ comment in the line 20 of [build.sh](https://github.com/zdavatz/cpp2sqlite/blob/master/scripts/build.sh#L20) to do the translations.\
_ if there are no errors, then commit `intput/deepl.sappinfo.out.fr.txt`.
## Input Sources
_ [AIPS](http://download.swissmedicinfo.ch)\
_ [BAG XML](http://www.spezialitätenliste.ch/File.axd?file=XMLPublications.zip)\
_ [BAG FHIR](https://epl.bag.admin.ch) (ndjson, **default ON since 01.06.2026**; disable with `--no-fhir`, prices from BAGFHIR are used by Refdata and Swissmedic lookups)\
_ [Refdata Artikel](https://www.refdata.ch/de/artikel/abfrage/artikel-refdatabase-gtin)\
_ [Refdata Partner](https://www.refdata.ch/de/partner/abfrage/partner-refdatabase-gln)\
_ [Refdata SAI](https://sai.refdata.ch/download)\
_ [Swissmedic](https://www.swissmedic.ch/dam/swissmedic/de/dokumente/listen/excel-version_zugelasseneverpackungen.xlsx.download.xlsx/excel-version_zugelasseneverpackungen.xlsx)\
_ [Swissmedic HPC](https://www.swissmedic.ch/swissmedic/de/home/humanarzneimittel/marktueberwachung/health-professional-communication--hpc-.html)\
_ [Swissmedic Chargenrückrufe](https://www.swissmedic.ch/swissmedic/de/home/humanarzneimittel/marktueberwachung/qualitaetsmaengel-und-chargenrueckrufe/chargenrueckrufe.html)\
_ [EPha](http://download.epha.ch/data/matrix/matrix.csv)\
_ [Swisspeddose](https://swisspeddose.ch)\
_ [Sappinfo](https://sappinfo.ch)\
_ [Drugshortage](https://drugshortage.ch)
## Output Sqlite Database
_ [amiko_db_de](http://pillbox.oddb.org/amiko_db_full_idx_de.zip)\
_ [amiko_db_fr](http://pillbox.oddb.org/amiko_db_full_idx_fr.zip)

## Indikationscode (BAG XXXXX.NN)
When `--fhir` is set, two extra columns are appended at the tail of the
`amikodb` schema:

- `indikationscode` — comma-joined `XXXXX.NN` codes, deduped, in bundle order.
  Read directly from the explicit `indicationCode` extension on each limitation
  (BAG SL FHIR export >= v2.0.5); for older feeds without it, derived from the
  BAG FOPHDossierNumber + the ClinicalUseDefinition `.NN` suffix.
- `indikationscode_text` — newline-joined `XXXXX.NN: <limitations text>`
  lines for the same set of codes.

Both columns are bundle-scoped at the BAG preparation level and joined onto
each row by Swissmedic 5-digit registration number. Non-FHIR builds keep
the legacy schema (columns 0..18 only), so existing apps that read by
column index are unaffected. Mandatory transmission of IndC on prescriptions
and invoices for SL drugs starts 2026-07-01 (BAG Rundschreiben 2026-02-19).

## Zur Rose Artikelstamm (Exfact column)
As of 2026-05 the Zur Rose feed (`artikel_vollstamm_zurrose.csv` and
`artikel_stamm_zurrose.csv`) ships **22** semicolon-separated columns instead
of the previous 21. The new trailing column `Exfact` (V) is the Zur Rose
ex-factory price.

It is consumed by `zurrose` and written into the `exfprice` column of
`rose_db_new_full.db` / `rose_db_new_atc_only.db` as a fallback when BAG's
ex-factory price is missing for the article's GTIN — BAG values remain
canonical for SL-listed drugs. In `--fhir` builds (no BAG XML), this raises
`rosedb.exfprice` population from 0/163858 to 163858/163858 rows.

## AIPS content HTML (Refdata AllHtml.zip)
`downloads/aips.xml` only references the Fachinfo/Patinfo documents; the text
itself lives in `AllHtml.zip` (~1.1 GB, ~30'500 files), which `download.sh`
unpacks into `downloads/Refdata-AllHtml/`. **A medicine whose html file is not
on disk is dropped completely** by `src/c2s/aips.cpp` — no row in `amikodb`,
and therefore none of its Swissmedic packages either, even though
`swissmedic_packages.xlsx` and the Refdata SAI feed list them. The only trace
is the "Missing html files" section of `output/amiko_report_{de,fr}.html`.

On 2026-08-19 one run extracted 11'108 of 30'525 files (a full-length zip with
a corrupt middle region; `unzip` skipped the entries and still exited 0). 6'514
of the 10'232 German documents referenced by `aips.xml` were missing, the
German db came out with 1'694 rows, and registration 62069 (Levetiracetam
Desitin) lost all seven of its packages.

The download is therefore staged and verified before it replaces the live
folder: `wget` exit code, `unzip -t` over the whole archive, an entry-count
floor (`ALLHTML_MIN_FILES`, default 25000), extraction into
`Refdata-AllHtml.new`, and a comparison of the extracted file count against the
archive's entry count. Any failure keeps the previous `Refdata-AllHtml/` and
exits 1, so a build never runs on a half-populated folder.

## Zur Rose download and publishing
`scripts/download_zr.sh` fetches the eleven Zur Rose feeds over SFTP. It never
writes over a live input file directly: everything is staged in
`input/zurrose/.staging.<pid>`, validated, and only then moved into place, with
the previous version kept in `input/zurrose/.bak`. A feed that fails validation
leaves the last known-good file untouched.

All files are fetched in **one** sftp session (the server rate-limits new
connections), preceded by a connection test, and each file is checked against
the size the server reported. Then, per file: size and line floors, a shrink
guard against the current copy, no NUL bytes, no HTML error page, stable
encoding, and the `;`-column layout that the corresponding parser in `src/zur/`
requires. The spec table at the top of the script lists those numbers next to
the `src/zur/*.cpp` line that enforces each one — when Zur Rose changes a feed
layout (as with `Exfact` in 2026-05), bump the parser guard *and* the table.

    ./download_zr.sh              # download, validate, promote
    ZR_CHECK_ONLY=1 ./download_zr.sh   # validate the current inputs, no network
    ZR_DRY_RUN=1    ./download_zr.sh   # download and validate, replace nothing
    ZR_FORCE=1      ./download_zr.sh   # promote even if validation failed

Exit status: `0` all feeds updated, `1` at least one rejected (the rest were
updated), `2` the server could not be reached and nothing was touched.

`scripts/so_data` and `scripts/so_data_full` are the two cron entry points for
so.zurrose.ch — quick (`--zurrose=quick`) and full (`fulldb` + `atcdb`). Both
stop before building if a feed was rejected, stop before publishing if a build
failed, and verify every generated file (sqlite `integrity_check`, `rosedb` row
counts, JSON well-formedness, line counts, size floors, a shrink guard against
what is currently published, and that the file was actually rewritten by this
run). They also check a real basket and a real customer — `CANARY_PHARMACODES`
and `CANARY_GLNCODES` at the top of each script — so a build that would answer
`/smart/full` with nothing never goes live. Publishing is copy-then-rename, so
a client never reads a half-written database. The shared checks live in
`scripts/so_data_lib.sh`.

    6,16,26,36,46,56 7-19 * * 1-6 zdavatz /usr/bin/nice /usr/local/src/cpp2sqlite/scripts/so_data > /dev/null
    20 4,8,12,16,18   * * 1-6 zdavatz /usr/bin/nice /usr/local/src/cpp2sqlite/scripts/so_data_full > /dev/null

## zurrose SQLite lifecycle
`VOLL::closeDB()` finalizes the prepared statement and closes the SQLite
handle, so it must be called exactly once per run. A previous duplicate call
in `main()` double-freed both and caused `free(): invalid next size (fast)`
on exit of `--zurrose=fulldb` (`--zurrose=atcdb` happened not to trip the
allocator). Fixed in 3ed2fb5; do not reintroduce a second close.
## Performance
A full `cpp2sqlite --lang de` run takes about **151 s**, down from 463 s, with
byte-identical output. The work was single-threaded throughout; the database
writes were never the bottleneck (`insertRow` accounts for 1.9 s of the run).

What the profile showed, and what was done about it:

| Was | Now | Change |
| --- | --- | --- |
| lookups scanning 6'800-18'000 entries per call | `unordered_map` indexes built once after parsing | `swissmedic.cpp`, `refdata.cpp`, `bagFHIR.cpp`, `bag.cpp` |
| range-for copying `Article` / `Preparation` per iteration | `const&` | same four files |
| 76 `replace_all` passes over each ~85 KB document | one pass with a hash lookup | `beautify.cpp` |
| 2 `std::regex` per document | plain string operations | `refdata.cpp` |

The indexes assume the parsed lists are immutable after their parse function
returns, and they keep the *first* entry for a duplicate key because the scans
they replaced stopped at the first match. Both properties matter: appending to
those lists later, or switching `emplace` to `operator[]`, changes results
silently.

`bag.cpp` and `bagFHIR.cpp` are shared with **zurrose, pharma and sai**, so a
change there is not a cpp2sqlite-only change — `rosedb.exfprice`, the prices in
`pharma.csv` and `sai.db` all come through the same `getPricesAndFlags()`. The
other three tools got faster too: the full run matrix went from 622 s to 276 s.

Note that `--fhir` is **inverted** on zurrose, pharma and sai: omitting it uses
the FHIR ndjson, passing `--fhir` falls back to the legacy BAG XML. cpp2sqlite
is the other way round, with `--no-fhir` as the opt-out.

When changing any of this, verify by comparing the generated artifacts rather
than by spot checks: hash every table, column and row of the databases, and
normalise the build timestamp in the html footer (plus the run timestamp and
argv paths in the `*_report.html` files). Build the "before" side from the
previous commit in a `git worktree` — the binary in `build/` may be far older
than HEAD. Sixteen configurations cover every path these files touch:

    cpp2sqlite  --lang de | --lang fr | --pinfo | --no-fhir | --without-sappinfo
    zurrose     --zurrose=fulldb|atcdb|quick , each with and without --fhir
    pharma      default | --fhir | --storage
    sai         default | --fhir

## Glossary
_ [GTIN](http://www.ywesee.com/Main/EANCode)
