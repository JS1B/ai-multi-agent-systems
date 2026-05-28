## Cursor Cloud specific instructions

- This repo's primary runnable flow is the C++ MAvis search client plus the Java `server.jar` harness; standard build/run commands are documented in `README.md`.
- There are no long-running local services, databases, or caches to start. End-to-end validation means building `searchclient_cpp/searchclient` and running `server.jar` against a level under `levels/`.
- The optional benchmark runner in `benchmarks/` expects a local `benchmarks.json`; only `benchmarks.example.json` is committed.
- As of this setup, `make test` starts correctly but the full target fails while compiling stale tests that reference an older `Level` API. The currently compiled `test_chargrid` and `test_foo` executables pass; use the MAvis server run as the stronger application-level validation until those tests are updated.
