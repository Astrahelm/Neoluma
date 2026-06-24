# Tests (regression suite)

Run the regression suite with:

```powershell
powershell -ExecutionPolicy Bypass -File tests/runner/run-tests.ps1
```

Or through CMake after building:

```powershell
cmake --build .build/cmake-debug --target frontend_tests
```

The runner creates a temporary project per case under `tests/.tmp/`, copies the case files into `src/`, runs `neoluma check --json`, and validates stable fields from `expect.json`.
