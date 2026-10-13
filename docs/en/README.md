# Warehouse Qt

## Project

A university warehouse management assignment (Topic 28) built in C++17 with Qt Widgets.
The interface is Bulgarian. Products, users, sales and financial reports persist in SQLite.
All prices use integer **euro cents**; displayed amounts use **€**.

- [Architecture](ARCHITECTURE.md)
- [User guide](USER_GUIDE.md)
- [Defense notes](DEFENSE_NOTES.md)
- [Compliance audit](COMPLIANCE.md)
- [Български](../bg/README.md)

## Requirements

Qt 6.8 or newer with Core, Sql, Widgets, Test and the QSQLITE driver;
CMake 3.21 or newer; a Qt-compatible 64-bit compiler; Qt Creator or a terminal.
No other application libraries are required.

Verified on Windows 11 with Qt 6.12.0, Qt Creator 20.0.2, GCC 13.1.0, CMake 3.30.5 and Ninja.
Qt 6.8 and other operating systems have not been tested.

## Build, run and test

In Qt Creator, open the root CMakeLists.txt, select **Desktop Qt 6.12.0 MinGW 64-bit**,
configure and build. Select warehouse_app to run. Run the registered CTest tests.

These PowerShell commands reproduce the verified local build from the repository directory.
Adjust installation paths if Qt was installed elsewhere. Stop if a build command fails.

```powershell
$env:PATH = 'C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.12.0\mingw_64\bin;' + $env:PATH
& 'C:\Qt\Tools\CMake_64\bin\cmake.exe' -S . -B build-verified -G Ninja '-DCMAKE_MAKE_PROGRAM=C:/Qt/Tools/Ninja/ninja.exe' '-DCMAKE_CXX_COMPILER=C:/Qt/Tools/mingw1310_64/bin/g++.exe' '-DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64' -DCMAKE_BUILD_TYPE=Debug
& 'C:\Qt\Tools\CMake_64\bin\cmake.exe' --build build-verified
& 'C:\Qt\Tools\CMake_64\bin\ctest.exe' --test-dir build-verified --output-on-failure
& '.\build-verified\warehouse_app.exe'
```

CMake enables -Wall -Wextra on MinGW. All five test executables must pass:
warehouse_tests, tst_inventory, tst_discounts, tst_permissions and tst_gui.
Core tests use isolated :memory: databases; one persistence test uses a temporary file.
GUI tests use the native Windows Qt platform, or offscreen on other systems.
For a detailed GUI failure log:

```powershell
& '.\build-verified\tst_gui.exe' -o build-verified/gui-results.txt,txt
Get-Content build-verified/gui-results.txt
```

## Default logins

| Username | Password | Role |
|---|---|---|
| admin | admin | Administrator |
| manager | manager | Warehouse manager |
| cashier | cashier | Cashier |

The first run creates three users and ten demo products.
Passwords are stored as SHA-256 hashes, as specified by the assignment.
To use another role, close the app and log in again.

## Data and project status

The app uses QStandardPaths::AppDataLocation. On this Windows installation:
%APPDATA%\University\WarehouseQt\warehouse.sqlite.
Data survives closing the app and is not committed to Git.
The demo values are denominated in EUR; this project has no currency-conversion feature.

Phases 0–6 are complete: environment/build skeleton, domain, persistence/service,
product GUI, remaining GUI, UML/audit, and bilingual documentation.

The user approved sale-time delivery cost/name/SKU snapshot columns and changed the currency to EUR.
The original Bulgarian assignment referenced by the brief was not supplied;
the compliance audit therefore covers the provided brief and subsequent instructions.
