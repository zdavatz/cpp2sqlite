# EAN13 barcode generator

Vendored from [ywesee/BarcodeGenerator](https://github.com/ywesee/BarcodeGenerator)
at commit `c1f17da3d2ed6216abc4ad83b6d1bb42ad2dcef1` (2026-05-22). Previously a
git submodule; kept in-tree since 2026-08-19 so that a fix lands in one commit
instead of three (submodule commit, parent pointer bump, `git submodule update`
on every consumer) — the split silently reverted the `<cstdint>` fix once.

Only the library is vendored: `functii.cpp` and `functii.h`. The upstream
standalone demo (`main.cpp`, `sample-input.txt`, `sample.html`) is not needed
here. cpp2sqlite uses a single entry point, `EAN13::createSvg()`, called from
`getBarcodesFromGtins()` in `src/c2s/cpp2sqlite.cpp` to embed an SVG barcode
per package GTIN into the Fachinfo html.

Originally written by Stefan Halus (2016), ©ywesee GmbH, GPLv3 — the same
licence as cpp2sqlite. Changes made here are not pushed back to
ywesee/BarcodeGenerator; port them by hand if that repo needs them.
