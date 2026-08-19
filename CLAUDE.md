# CLAUDE.md

## Project Overview
cpp2sqlite is a C++ tool that generates SQLite databases containing Swiss Healthcare Public Domain Drug Information. It aggregates data from multiple Swiss health authority sources (Swissmedic, BAG, Refdata, etc.) into SQLite databases used by AmiKo and other applications.

## Build
```bash
cd scripts && source steps_public1.source && ./download.sh
source steps_public2.source && ./download.sh
cd ../build && cmake .. && make -j9
```

Note: `BOOST_BIND_GLOBAL_PLACEHOLDERS` is defined in CMakeLists.txt to suppress Boost bind placeholder deprecation warnings (the project doesn't use boost::bind directly, but Boost headers trigger the warning internally).

## Key Executables
- `cpp2sqlite` - Main tool, generates amiko_db SQLite databases
- `pharma` - Generates pharma.csv
- `sai` - Processes SAI (Refdata structured article information)
- `zurrose` - Processes Zur Rose data

All use BAG FHIR ndjson by default since 01.06.2026 (`flagFHIR=true`); pass `--no-fhir` to fall back to the legacy BAG XML. The `--fhir` flag still works as an explicit opt-in.

## FHIR NDJSON Parsing
- Source: `src/bagFHIR.cpp` / `src/bagFHIR.hpp`
- Input: `downloads/fhir-sl-{de,fr,it}.ndjson` from https://epl.bag.admin.ch/static/fhir/foph-sl-export-latest-{de,fr,it}.ndjson (per-language NDJSON)
- Each line is a FHIR Bundle with entries: MedicinalProductDefinition, RegulatedAuthorization, PackagedProductDefinition, Ingredient, ClinicalUseDefinition
- Prices (EFP/PP) and reimbursement data are in RegulatedAuthorization extensions under `reimbursementSL` > `productPrice`
- Legal status codes map to categories: A (756005022001), B (756005022003), C (756005022005), D (756005022007, 756005022008), E (756005022009)
- When `--fhir` is active, all price lookups (including from Refdata and Swissmedic) must use `BAGFHIR::getPricesAndFlags()` instead of `BAG::getPricesAndFlags()`, since `BAG::parseXML()` is not called and its prepList is empty

## Indikationscode (BAG XXXXX.NN)
- Mandatory on SL prescriptions and invoices from 2026-07-01 (BAG Rundschreiben 2026-02-19); insurers may reject non-compliant invoices from 2027-01-01
- Source: `src/bagFHIR.cpp` `collectBundleIndC()` — bundle-scoped pass over each NDJSON line (one Bundle per line)
- Preferred source (BAG SL FHIR export >= v2.0.5): the explicit `indicationCode` sub-extension carried on each limitation (`RegulatedAuthorization.indication[].extension[regulatedAuthorization-limitation].extension[indicationCode].valueString`, e.g. `20403.01`). The BAG changelog states the limitation code (`ClinicalUseDefinition.id`) and the indication code are **independent** fields, so `XXXXX.NN` must NOT be reconstructed from the CUD id suffix. The limitation text is resolved via the sibling `limitationIndication` reference (e.g. `ClinicalUseDefinition/CYRAMZA.01`) into that CUD's text. Verified on the live DE feed: 718 / 6'775 bundles carry the explicit field.
- Fallback (older feeds without the extension): `XXXXX` from `RegulatedAuthorization.extension[reimbursementSL].extension[FOPHDossierNumber].valueIdentifier.value`; `.NN` is the trailing all-digit segment of each `ClinicalUseDefinition.id` (e.g. `CYRAMZA.01`); only `type == "indication"` CUDs are picked up. The feed ships `type` as a plain string and `indication` as a single object — both shapes are handled.
- CUD text comes from `indication.diseaseSymptomProcedure.concept.text`, with fallback `indication.extension[url == "limitationText"].valueString`
- Stored on `BAG::Preparation.indicationCodes` (and copied onto every `Pack`) as `IndicationCode {code, cudId, text}`
- `BAGFHIR::getIndCByRegnr(regnr, &codes, &text)` returns the joined columns the schema expects: comma-joined codes + newline-joined `code: text` lines, deduped by code, in bundle order. Mirrors rust2xml ≥ 3.1.12 (`INDIKATIONSCODE` / `INDIKATIONSCODE_TEXT` XML elements). The function walks the entire `prepList` and merges IndicationCodes from every prep that shares the queried Swissmedic regnr, deduped by code — required because the same 5-digit Swissmedic regnr typically spans multiple FHIR bundles (only one of which carries the type=indication ClinicalUseDefinitions). An earlier early-return on the first matching prep with empty indicationCodes silently dropped IndC for ~80% of regnrs.
- Schema/binding: `Sql::useIndC` is set from `flagFHIR` in `openDB()`. When true, the `amikodb` schema gains two trailing TEXT columns (`indikationscode`, `indikationscode_text`) and `insertRow` binds positions 19/20. Non-FHIR builds emit the legacy 19-column schema, so apps reading by column index keep working.
- iOS consumer: `generikacc/Generika/IndCSection.swift` (issue zdavatz/generikacc#102) — guarded by `AmikoDBManager isIndcColumnAvailable` so older DB snapshots don't break the app

## AIPS content HTML (downloads/Refdata-AllHtml)
- `downloads/aips.xml` is only an index: each `MedicinalDocumentsBundle` carries the regnrs plus a `DocumentReference` URL. The text lives in `AllHtml.zip` (~1.1 GB, ~30'500 files), unpacked by `scripts/download.sh` into `downloads/Refdata-AllHtml/`.
- `src/c2s/aips.cpp:246` sets `Med.contentHTMLPath` only if the html exists on disk, and `medList.push_back(Med)` is guarded by `!Med.contentHTMLPath.empty()`. **A missing html silently removes the whole medicine** — no `amikodb` row, hence no Swissmedic packages either, even though `swissmedic_packages.xlsx` and `sai.db` have them. The only trace is the "Missing html files" section of `output/amiko_report_*.html`.
- One bundle can list several regnrs (e.g. `61848` + `62069` share `6795ea24…-de.html`), so one missing file can take several registrations with it.
- Debugging recipe: `grep -o 'MedicinalDocuments/[0-9a-f]*-de\.html' downloads/aips.xml | sed 's|.*/||' | sort -u` vs. `ls downloads/Refdata-AllHtml | grep -- '-de\.html' | sort -u`, then `comm -23`. A healthy DE run has <10 missing out of ~10'200.
- Incident 2026-08-19: the 04:02 run extracted 11'108 of 30'525 files — a full-length zip with a corrupt middle region (files present for offsets 0–19 MB and 705 MB–end, a 686 MB hole between). `unzip` 6.00 skipped those entries and **still exited 0**; the old `download.sh` checked neither `wget`'s nor `unzip`'s exit code and deleted the zip afterwards. Result: 6'514 of 10'232 DE documents missing, `amiko_db_full_idx_de.db` at 1'694 rows, regnr 62069 gone. The server blob was fine and unchanged (same ETag/Last-Modified hours later), so the fault was in transit or local.
- `scripts/download.sh` now stages the archive: `wget` rc, `unzip -tqq`, an entry-count floor (`ALLHTML_MIN_FILES`, default 25000), extraction into `Refdata-AllHtml.new`, and extracted-count vs. archive-entry-count. Any failure keeps the previous folder and exits 1. Do not "simplify" this back to a bare `wget && unzip` — unzip exiting 0 is not evidence that it extracted anything.

## AIPS build and publishing (scripts/amiko_data)

- `scripts/amiko_data` + `scripts/amiko_lib.sh` are the gated version of the AiPS cron pipeline. They replace nine hand-written scripts in `/usr/local/src`, one per build (aips, fachinfo de, fachinfo fr, sai, pinfo) and one per publishing target, each calling the next.
- Four stages, run in order by default, individually selectable: `aips` (download + `cpp2sqlite` de/fr), `fachinfo` (`fachinfo_ai` frequency dbs), `sai` (`sai` + `nonpharma`), `pinfo` (`cpp2sqlite --pinfo`). One cron entry instead of five; the order is what removes the races, so do not split them back onto fixed clock times.
- **Hosts, addresses and directories are not in the repository.** They live in `scripts/amiko_targets.conf`, which is gitignored; `amiko_targets.conf.example` documents the format. The conf sets `AMIKO_WWW` (local web root), `AMIKO_AI` (fachinfo_ai checkout) and `AMIKO_REMOTES` (`user@host|dir|port` entries). Every value honours an environment override, so a test run can redirect the whole pipeline. A missing conf aborts before anything runs.
- What the old layout got wrong, and why each guard exists:
  - **No exit code was ever checked.** `download.sh` refuses a bad `AllHtml.zip` and exits 1, but the caller built anyway. Every stage is now gated by `|| die`.
  - **Nothing looked at the databases.** On 2026-08-19 a 1'694-row `amiko_db_full_idx_de.db` (instead of 4'640) went to four servers. `verify_outputs` enforces mtime (must be written by this run), a byte floor, a shrink guard against the published copy, `PRAGMA integrity_check` and a row floor on the named table.
  - **Registration canaries.** `canary_regnr` checks that specific regnrs are present, because a missing content html silently drops a whole medicine without changing anything structural. `CANARY_REGNRS` starts with 62069, the casualty of that incident.
  - **`rm *.zip`.** Three of the old publishing scripts began by deleting every archive in `output/`, so the 05:00 sai run wiped the 04:00 amiko archives and the 07:00 copy to the web root then copied files that no longer existed. `zip_output` removes only the archive it is about to write (`zip` updates an existing archive in place, so it does have to be removed).
  - **Fixed-offset race.** The fachinfo scripts ran at 04:30 while the de/fr build needs ~35 min, with no interlock, so they could copy a database that was still being written.
  - **Non-atomic publishing.** A bare `scp` of a 400 MB database over the live one lets a client fetch a half-written file. `publish_local` and `publish_remote` both copy to a temp name and rename.
- An unreachable target is a warning, not an abort: the remaining targets still get the build and the run exits 1 so cron mails about it. This matters — the old fachinfo scripts ran under `set -e` with a long-dead host **first** in their list, so every run aborted before reaching the two hosts that did work, and one of them served `amiko_frequency_*.db` from 2026-07-16 for a month. That entry is commented out in the conf; do not leave dead hosts in the list, a warning on every run trains everyone to ignore it.
- One of the remote targets also pulls some files by itself with `wget`. Do not read a fresh file there as proof that the push worked — that is exactly what masked the stale month.
- Downloading happens once, in `stage_aips`; `fachinfo`, `sai` and `pinfo` build from the same `input/`/`downloads/`. `pinfo` therefore never refetches, and it does not run at all if the download was rejected — unlike the old 07:00 job, which built regardless.
- **Cron PATH is minimal (`/usr/bin:/bin`).** `sqlite3` is checked at load time and a miss is fatal: `verify_outputs` used to print "not checked" and carry on, i.e. a run with no gate that still reports success and publishes. `cargo` is not on that PATH either, so the `fachinfo` stage sources `~/.cargo/env`. The targets file is resolved with `${BASH_SOURCE[0]%/*}` rather than `dirname`, for the same reason.
- Marker files (`amiko_update_done`, `sai_non_pharma_done`) are written **last**, after every file they refer to is in place; clients poll them.
- Knobs: `AMIKO_SKIP_DOWNLOAD=1`, `AMIKO_SKIP_BUILD=1` (verify and publish what is already in `output/`), `AMIKO_NO_PUBLISH=1`, `AMIKO_NO_REMOTE=1`, `AMIKO_SHRINK_PCT`, `AMIKO_SRC`, `AMIKO_CONF`.
- `amiko_lib.sh` is deliberately **not** shared with `so_data_lib.sh`: the two pipelines publish different files to different places, and keeping them apart means a change here cannot break the other one.
- `download.sh` unzips SwissPedDose and `transfer.zip` with `-o`. Without it `unzip` prompts when the extracted file already exists, which is why the old download wrapper `rm`ed those two files first; under cron a prompt is a hang, not a question.

## Zur Rose Artikelstamm (Exfact column)
- Feed: `artikel_vollstamm_zurrose.csv` and `artikel_stamm_zurrose.csv` are downloaded via `scripts/download_zr.sh` (SFTP from ftp.zur-rose.ch).
- Schema change (2026-05): a trailing column `Exfact` (V, index 21) was appended, growing the row from 21 to 22 semicolon-separated fields. All four column-count guards must match the new width — `src/zur/stamm.cpp:69`, `src/zur/stamm.cpp:129`, `src/zur/voll.cpp:152`, `src/zur/voll.cpp:355`. Note that `voll.cpp::parseCSV` (line 152) silently `continue`s on mismatch instead of exiting, so a missed bump there empties the output DB without an error message.
- `Exfact` is the Zur Rose ex-factory price and is wired into `rosedb.exfprice` as a fallback for the BAG EFP in `voll.cpp::parseCSV` (~line 304). BAG values stay canonical for SL-listed drugs; Zur Rose Exfact fills the long tail. In `--fhir` builds without BAG XML this lifts `rosedb.exfprice` population from 0/163858 to 163858/163858.

## Zur Rose download integrity (scripts/download_zr.sh)
- Every feed is fetched into `input/zurrose/.staging.<pid>` (per run, so overlapping cron jobs cannot wipe each other's half-fetched files; an `EXIT` trap removes it, plus a sweep for anything older than 4 h), validated, and only then moved over the live file; the previous version is kept in `input/zurrose/.bak`. A rejected feed leaves the last known-good input in place and the script exits 1 — callers must check that before running `./zurrose`.
- Checks per file: sftp exit code, remote-vs-local byte count, size/line floors, shrink guard vs. the current file (`ZR_SHRINK_PCT`, default 70%), no NUL bytes, no HTML error page, encoding stability, and the `;`-column count each parser in `src/zur/` requires. The spec table at the top of the script carries those numbers with the `src/zur/*.cpp` line that enforces them — when a feed layout changes, bump the parser guard **and** the table, otherwise the download is rejected.
- Three feeds ship as ISO-8859-1 and are consumed as-is (`artikel_stamm_zurrose.csv`, `Autogenerika.csv`, `medix_kunden.csv`); only `Artikelstamm_Vollstamm.csv` and `kunden_alle_dynamics_ce.csv` are converted to UTF-8. The validator fails on an encoding *flip* rather than demanding UTF-8 everywhere.
- Transport is `sftp -oBatchMode=no -b <batchfile>`, not `scp`: against the Serv-U server at Zur Rose, OpenSSH 9+ scp reports `Exit status -1` and exits 1 even when the transfer succeeded, so its exit code cannot gate anything. sftp also reports the remote size (`ls -l`), which is what the transfer-completeness check compares against.
- **One session for all 11 files**, not one per file. The server rate-limits new connections: with a session per file, two overlapping runs opened dozens between them and got refused with `rc=255` partway through (measured: 4 of 11 feeds lost). Batch lines are prefixed with `-` so a missing file does not abort the rest; each file is then verified locally and only the incomplete ones are retried (3 rounds). A full run is ~7 s and 2 connections, and two concurrent runs both complete 11/11.
- A `sftp_ping` preflight (`pwd` in its own session) runs before anything is staged. If the server is unreachable or the login fails, the script exits **2** having touched nothing — `so_data` treats that as "skip this cycle" and exits 0, `so_data_full` treats it as an error. Exit 1 still means "connected, but at least one feed was rejected".
- `ZR_CHECK_ONLY=1` validates the current inputs without downloading; `ZR_DRY_RUN=1` downloads and validates without replacing anything; `ZR_FORCE=1` promotes anyway.
- `scripts/so_data_full` and `scripts/so_data` are the gated versions of the two so.zurrose.ch cron jobs — full (`download.sh` + `download_zr.sh`, fulldb + atcdb) and quick (`download_zr.sh`, `--zurrose=quick`). Both abort if a feed was rejected or a build failed, verify the generated files (sqlite `integrity_check`, `rosedb` row counts, JSON well-formedness, line counts, size floors, shrink guard vs. what is published, and mtime — a file not rewritten by this run is refused), then install them into `/var/www/so.zurrose.ch/rose` atomically (copy to a temp name, then rename). The checks live in `scripts/so_data_lib.sh`, which both source; each script only carries its own list of output files and their floors.
- Canaries (`canary_db` / `canary_csv` / `canary_json_key` in the lib): a build can pass every structural check and still answer `https://so.zurrose.ch/smart/full` with nothing, because the specific article or customer asked about is missing. Both scripts therefore check a real basket (`CANARY_PHARMACODES`) against `rosedb`/`rose_stock.csv` and a real customer (`CANARY_GLNCODES`) against `rose_ids.json` and `rose_conditions_new.json` before publishing. Edit those arrays at the top of `so_data` / `so_data_full` when the sample order stops being representative.
- `--zurrose=quick` still regenerates `rose_conditions_new.json`, `rose_ids.json`, `rose_autogenerika.json`, `rose_direct_subst.json`, `rose_nota.json` and `Grippeimpfstoff.json` (those run unconditionally in `src/zur/main.cpp`), plus `rose_stock.csv` which is quick-only. `Grippeimpfstoff.json` is legitimately `[\n]` (5 bytes) out of flu season, hence its 2-byte floor.

## zurrose SQLite lifecycle (do not double-close)
`VOLL::closeDB()` calls `sqlite3_finalize(statement)` then `sqlite3_close(db)`, so it must be called exactly once. Calling it twice double-frees both handles and corrupts the glibc heap — the failure surfaces as `free(): invalid next size (fast)` on process exit and, depending on arena state, may not reproduce on smaller workloads (atcdb hid it while fulldb crashed every run). `src/zur/main.cpp` previously had a stray second `closeDB` block right after the fulldb/atcdb branch; the single call inside the branch is the correct lifecycle.

## ean13 is vendored, not a submodule (since 2026-08-19)
- `src/c2s/ean13/` holds `functii.cpp` + `functii.h` as ordinary files, copied from `ywesee/BarcodeGenerator` at `c1f17da` (the `<cstdint>` fix). Edit them like any other source; one commit, no pointer bump, no `git submodule update`.
- It used to be a submodule, and the three-step dance was a real hazard: a `git pull` without `git submodule update` left the working tree on the 2019 commit `7516528`, which silently *removed* the `<cstdint>` include again. `git status` then shows a submodule "change" that is actually a revert — committing it would have broken the Funtoo build server.
- Only the library is vendored; upstream's standalone demo (`main.cpp`, `sample-input.txt`, `sample.html`) was dropped. Changes here do NOT flow back to `ywesee/BarcodeGenerator` — port by hand if that repo needs them.
- Single consumer: `EAN13::createSvg()` in `getBarcodesFromGtins()` (`src/c2s/cpp2sqlite.cpp:156`), which embeds one SVG barcode per package GTIN into the Fachinfo html. `CMakeLists.txt:50` and `:71` list the two files directly.

## cpp2sqlite performance (lookup indexes, single-pass cleanup)
`cpp2sqlite --lang de` used to take 463 s; it now takes 151 s (3.07x), single-threaded, with byte-identical output. The profile that drove this: 45.5% of the medicine loop was package lookups, 53.6% was HTML processing, and `sqlDb.insertRow` was **1.9 s — 0.47%**. Writing the database is not the bottleneck and never was.

- **Never scan a list to find a record.** `swissmedic.cpp`, `refdata.cpp`, `bagFHIR.cpp` and `bag.cpp` each build `unordered_map` indexes at the end of their parse function (`rowsByRegnr`/`rowByGtin`/`rowByGtin12`, `byGtin5`/`byGtin13`/`byRegnr5`, `prepsByRegnr`/`packByGtin`). These are only safe because the containers are filled during parsing and never touched afterwards — if you ever append to `prepList`/`artList`/`theWholeSpreadSheet` later, the indexes go stale.
- The indexes use `emplace`, deliberately: most of the old scans `break` on the first hit, so the **first** entry must win. `operator[]` would keep the *last* one and silently change prices, categories and names.
- Regnr indexes store ascending positions, so `getNames`/`getAdditionalNames` still append packages in list order — that order reaches the `packages` column.
- `getPricesAndFlags()` (both `bag.cpp` and `bagFHIR.cpp`) lazily fills `packMap`, which `getPackageFieldsByGtin()` reads afterwards. The index lookup keeps that side effect. Its `goto prepareResult` is gone: with a map there are no nested loops left to break out of.
- **Range-for over these lists must use `const&`.** `for (Article art : artList)` and `for (BAG::Preparation prep : prepList)` copied a struct full of `std::string`s — the latter dragging its whole `packs` vector — on every iteration. Measured: Swissmedic's indexed scan did 97.4 M iterations in 25.9 s while BAG's copying scan did 36.6 M in 71.1 s, i.e. **7x the cost per element purely from copying**.
- `BEAUTY::cleanupForNonHtmlUsage()` is one pass with a hash lookup on each `&...;`, not 76 consecutive `boost::replace_all` calls over an ~85 KB document. Equivalent because no replacement output contains `&` (so passes cannot cascade) and every key runs from `&` to its first `;` (so at most one key can match at a position). Add new entities to `entityTable()`; `maxEntityLength()` adapts automatically.
- The two `std::regex` in `REFDATA::getArticleDocument()` are now `fixLeadingDiv()` / `fixHtmlTagQuotes()`. `std::regex` cost 288 s across the 29'693 downloaded html files versus 1.2 s for plain string ops. `fixHtmlTagQuotes` must reproduce greedy `\S*`, i.e. match the **last** `">"` inside the run of non-space characters after `<html `, and it needs *separate* search and copy cursors — conflating them corrupted 11 files in the first attempt.
- `fixLeadingDiv` matches in **0** of 29'693 files, so that path is unexercised by current data.

**`bag.cpp` / `bagFHIR.cpp` are shared — a change there is not a cpp2sqlite change.** `CMakeLists.txt` links them into `cpp2sqlite`, `zurrose`, `pharma` and `sai` (`beautify.cpp` additionally into `cpp2sqlite` and `pharma`, `refdata.cpp`/`swissmedic.cpp` only into `cpp2sqlite`). Verifying cpp2sqlite alone proves nothing about the other three: `rosedb.exfprice`, `pharma.csv` prices and `sai.db` all come through `getPricesAndFlags()`.

**Careful with `--fhir` outside cpp2sqlite: `zurrose` has it inverted.** `src/zur/main.cpp:199` does `bool flagFHIR = false; if (!vm.count("fhir")) flagFHIR = true;`, so for `zurrose` *omitting* the flag selects FHIR and *passing* `--fhir` selects the legacy BAG XML. `pharma` (`src/pha/main.cpp:107`) and `sai` (`src/sai/main.cpp:205`) use the normal polarity (`if (vm.count("fhir")) flagFHIR = true;`) — passing `--fhir` there selects FHIR, and omitting it selects the legacy BAG XML. `cpp2sqlite` defaults to FHIR and opts out with `--no-fhir`. So to exercise `bag.cpp` you pass `--fhir` to `zurrose`, `--no-fhir` to `cpp2sqlite`, and *nothing* to `pharma` and `sai`.

**Verifying a change here:** build both versions and compare the generated artifacts, not just row counts. For sqlite, hash every table, column and row; normalise the build timestamp in the html footer (`Auto-generated by ... on <date>`) and, for the `*_report.html` files, the run timestamp and the paths echoed from argv. Two runs of unchanged code then hash identically, so any real difference shows up. Build the "before" side from the previous commit in a `git worktree` rather than trusting whatever binary happens to sit in `build/` — it may be months older than HEAD.

The matrix that covers every code path these files touch, 16 configurations:
- `cpp2sqlite`: `--lang de`, `--lang fr`, `--pinfo`, `--no-fhir` (19-column legacy schema), `--without-sappinfo`
- `zurrose`: `--zurrose=` `fulldb` / `atcdb` / `quick`, each with and without `--fhir`
- `pharma`: default, `--fhir`, `--storage`
- `sai`: default, `--fhir`

The 3x commit (`371cec4`) was verified this way: identical `amikodb`, `rose_db_new_full.db`, `rose_db_new_atc_only.db`, `rose_stock.csv`, all six zurrose JSON outputs, `pharma.csv` and `sai.db`.

## GCC 13+ header transitivity
GCC 13+ (and current libstdc++) no longer pulls many standard headers transitively. New code that uses standard types/streams must `#include` them explicitly or the Funtoo build server breaks:
- `<cstdint>` for `uint8_t` / `int32_t` / etc. (tripped `src/c2s/ean13/functii.cpp` — fixed in ywesee/BarcodeGenerator c1f17da)
- `<fstream>` for `std::ofstream` / `std::ifstream` (tripped `src/sap/main.cpp`, `src/dru/main.cpp` — fixed in 0203368, plus `bagFHIR.cpp`, `c2s/peddose.cpp`, `c2s/sappinfo.cpp`, `c2s/refdata.cpp` cleaned up preemptively)

If a new source file uses these (or `<sstream>`, `<iomanip>`, etc.) without an explicit include, it will compile locally on older toolchains and break on the build server.

## Directory Structure
- `src/` - C++ source files
- `src/c2s/` - cpp2sqlite specific sources
- `src/pha/` - pharma specific sources
- `src/sai/` - SAI specific sources
- `src/zur/` - zurrose specific sources
- `scripts/` - Build and download scripts
- `input/` - Input data files (not in git)
- `downloads/` - Downloaded source files (not in git)
- `output/` - Generated databases and reports (not in git)
