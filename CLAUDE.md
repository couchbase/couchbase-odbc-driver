# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

The official ODBC driver for Couchbase (Analytics). It is a fork of the ClickHouse ODBC driver
(hence the `CH_ODBC_*` CMake options and the `clickhouse-odbc` branch of the bundled Poco) adapted to
talk to Couchbase via the `contrib/libcouchbase-odbc` submodule. C++17 + C11, built with CMake.

The same source compiles into **two** drivers via `declare_odbc_lib_targets` in `driver/CMakeLists.txt`:
- `couchbaseodbc` (ANSI, `UNICODE=0`)
- `couchbaseodbcw` (Unicode, `UNICODE=1`, defines `UNICODE`/`_UNICODE`)

## Build

Submodules are required. Always run before a fresh build:
```
git submodule update --init --recursive
```

OpenSSL is resolved per-platform (see `contrib/CMakeLists.txt`): **Windows** pulls it via cbdeps
(`cbdep_install`), **macOS** uses Homebrew (`brew --prefix openssl@3`). Keep that platform split intact
when editing dependency wiring.

### macOS
Prereqs (Homebrew): `cmake`, `icu4c`, `openssl@3`, `unixodbc`.
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DICU_ROOT=$(brew --prefix)/opt/icu4c \
  -DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)
cmake --build build -j8
```
Artifacts: `build/driver/libcouchbaseodbc.dylib` and `build/driver/libcouchbaseodbcw.dylib`.

### Windows (x64, Release, MSI)
```
cmake -A x64 -DLCB_USE_HDR_HISTOGRAM=OFF -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
cmake --build . --config Release --target package   # produces the MSI
```

More detail (driver-manager setup, DSN config, manual connection testing) is in `dev/DEV_README.md`.

## Tests

There is no in-tree automated test suite (`BUILD_TESTING` is ON but no `add_test` targets are
defined here). The `-impl` static-library split exists so non-public symbols *can* be unit-tested,
but tests currently live in the `libcouchbase-odbc` submodule, not this repo. Verification here is
manual against a running Couchbase/data source — see the "Test Driver" section of `dev/DEV_README.md`
(ODBC manager `isql`, or the sample apps under `dev/sample_odbc_applications`).

## Formatting

Style is `.clang-format` (project root). `format_sources` documents the KDevelop custom-formatter
hook; the canonical invocation is `clang-format -style=file`.

## Architecture

ODBC call flow, outermost to innermost:

- **`driver/api/odbc.cpp`** — the exported ODBC C entry points (`SQLConnect`, `SQLExecute`, …). This
  is the only translation unit in the *shared* library; everything else lives in the `-impl` static
  lib. Exports are controlled per-platform: a `.def` file on 32-bit Windows, a `version_script` on
  Linux. Calls are routed through `Driver::getInstance().call(...)` (`CALL_WITH_TYPED_HANDLE` macro).
- **`driver/api/impl/`** — the actual implementation behind the public API (`impl.cpp`, `queries.cpp`).
- **`driver/source/`** — the ODBC object model: `environment`, `connection`, `statement`,
  `descriptor`, `result_set`, `diagnostics`, plus `driver.cpp` (the global dispatcher/handle registry).
  Handle dispatch can be made unchecked via `CH_ODBC_ALLOW_UNSAFE_DISPATCH`.
- **`driver/format/`** — parsers for the server wire formats into ODBC result sets: `CBAS` (Couchbase
  Analytics), `ODBCDriver2`, `RowBinaryWithNamesAndTypes`. Add result-format handling here.
- **`driver/escaping/`** — ODBC escape-sequence handling (`{fn ...}`, `{d ...}`, etc.) via a generated
  lexer (`lexer.h`, `lexer_declare.h`, `function_declare.h`).
- **`driver/utils/`** — type parsing (`type_parser`), ODBC type metadata (`type_info`), and the
  value-conversion layer. Unicode conversion has two backends selected at configure time by
  `CH_ODBC_USE_ICU`: ICU (`conversion_icu.h`, default off-MSVC) vs C++ stdlib (`conversion_std.h`).
- **`driver/config/`** — DSN / connection-string parsing and INI handling.
- **`driver/platform/win/`** — Windows-only: `dllmain`, the DSN setup/config dialog (`setup.cpp` +
  `resource.rc`), and the per-target `.def` export lists.

Build-time behavior is injected through `platform/config_cmake.h.in`, configured **per driver variant**
into `config_cmake.h` / `config_cmakew.h`. The `WORKAROUND_*` variables in `driver/CMakeLists.txt`
toggle quirks based on the ODBC provider (UnixODBC vs iODBC) and options — consult those before
assuming a code path is unconditional.

## Dependencies (`contrib/`)

- **`libcouchbase-odbc`** (submodule) — the Couchbase client; linked as the `couchbase` target.
  Headers come from `contrib/libcouchbase-odbc/include` and generated `.../generated`.
- **`poco`** (submodule, `clickhouse-odbc` branch) — networking (`Poco::Net`, `Poco::Foundation`).
  On macOS it is built with `POCO_UNBUNDLED` (system zlib/pcre2/expat) to avoid patching bundled sources.

## Related repositories

- **[couchbase/couchbase-powerbi-connector](https://github.com/couchbase/couchbase-powerbi-connector)**
  (often cloned next to this repo at `../couchbase-powerbi-connector`) — the Power BI custom connector
  for Couchbase Analytics. Data path: `Power BI -> Couchbase Connector (.mez) -> Couchbase ODBC Driver -> CBAS`.
  It is a Power Query / M extension (`Couchbase/Couchbase.pq`) packaged as a `Couchbase.mez` (a zip of
  the `.pq` + icons + `resources.resx`), built on Windows via the Power Query SDK / `MakePQX`.

  **This driver is its backend, and the ODBC connection string is the contract between them.** The
  connector calls `Odbc.DataSource` with a user-provided `Dsn` plus auth fields that this driver must
  accept/parse (in `driver/config/`):
  - UsernamePassword → `User` / `Pass`
  - Client certificate → `AuthMode = "certificate"` (+ `ClientKeyPassword`)
  - OAuth2/JWT → `AuthMode = "jwt"`, `JWT = <token>`

  Changing connection-string keys, `AuthMode` values, DSN-dialog fields, or result formats here can
  break the connector. Note `JWT_E2E_WINDOWS_CHECK.md` in that repo, which tracks a known
  connector/driver contract gap for the JWT path (driver-side `AuthMode=jwt` / `lcbauth_set_jwt` wiring).

## Conventions worth knowing

- Anything that diverges by ANSI vs Unicode belongs in the `declare_odbc_lib_targets` function, not in
  ad-hoc per-file logic — both variants are generated from one definition.
- Platform-conditional dependency resolution (OpenSSL, Poco unbundling) is centralized in
  `contrib/CMakeLists.txt` and `cmake/Modules/CBDeps.cmake`; prefer extending those over scattering
  `if (APPLE)/if (WIN32)` blocks.
- `cmake/Modules/` holds the custom CMake modules (e.g. `CBDeps`, `prevent_in_source_builds`); in-source
  builds are explicitly blocked.
