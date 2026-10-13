# Phase 0 — complete

Prepared: CMake targets `warehouse_core`, `warehouse_app`, and `warehouse_tests`;
Bulgarian startup window; SQLite driver check; smoke test; build/database ignores.

Environment verified on 2026-10-13:

- Git is installed.
- Qt Creator 20.0.2 has a detected Qt 6.12.0 MinGW 64-bit kit.
- Qt path: `C:\Qt\6.12.0\mingw_64`.
- Compiler: GCC 13.1.0 in `C:\Qt\Tools\mingw1310_64\bin`.
- CMake 3.30.5 in `C:\Qt\Tools\CMake_64\bin`; Ninja in `C:\Qt\Tools\Ninja`.
- All three targets build with zero errors and warnings (`-Wall -Wextra`).
- CTest passes 1/1 executable, including smoke and QSQLITE loading checks.
- The brief requests Qt 6.8; verification used installed Qt 6.12.0.
  Qt 6.8 compatibility remains untested.
- The original Bulgarian assignment referenced by the brief was not included.

## Reproduce in PowerShell

From the `warehouse-qt` directory:

```powershell
$env:PATH = 'C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.12.0\mingw_64\bin;' + $env:PATH
& 'C:\Qt\Tools\CMake_64\bin\cmake.exe' -S . -B build-verified -G Ninja '-DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe' '-DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe' '-DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64' -DCMAKE_BUILD_TYPE=Debug
& 'C:\Qt\Tools\CMake_64\bin\cmake.exe' --build build-verified
& 'C:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build-verified --output-on-failure
& '.\build-verified\warehouse_app.exe'
```

Do not use the old `C:\MinGW` compiler with this kit.
In Qt Creator, open `CMakeLists.txt` and select Desktop Qt 6.12.0 MinGW 64-bit.

Next: Phase 1 domain classes and tests, after the user's `continue`.
