# Assignment compliance audit

## Requirements and evidence

| Requirement | Evidence |
|---|---|
| Qt Widgets, QTableView, product dialog, checkout | app/MainWindow.cpp, ProductTableModel.*, ProductDialog.*, SaleDialog.* |
| Logic separated from GUI | CMakeLists.txt: warehouse_core links only Qt6::Core and Qt6::Sql; core has no Widgets includes |
| Admin, WarehouseManager, Cashier polymorphism | core/User.h and User.cpp; pure virtual roleName and hasPermission |
| Discount Strategy Pattern | core/Discount.h and Discount.cpp; WarehouseService::setDiscountStrategy; SaleDialog::changeStrategy |
| Smart pointers for polymorphic collections | UserRepository::all and permission tests use vector<shared_ptr<User>>; service owns unique_ptr<DiscountStrategy> |
| Private members with accessors | Every authored class in core/*.h and app/*.h; tests have no data members |
| At least two constructors per class | Default/parameterized pairs in all core/app headers, including abstract bases; QObject test constructor pairs |
| Products, sales, receipts/invoices, financial reports | WarehouseService.*, repositories, MainWindow.*, ReceiptDialog.* |
| SQLite products, users, sales, reports | Database::open schema and seed; ProductRepository, UserRepository, SaleRepository |
| Required Qt Test coverage | tst_inventory, tst_discounts, tst_permissions; extra widget/PDF checks in tst_gui |
| UML roles, interfaces, Strategy Pattern | ../uml/class-diagram.puml and rendered ../uml/class-diagram.png |
| English and Bulgarian documentation | Matching README, ARCHITECTURE, USER_GUIDE, DEFENSE_NOTES and COMPLIANCE under docs/en and docs/bg |

## Reviewed behavior

Stock updates, sale header, and sale items commit together. A SQL-trigger failure test proves rollback.
Receipts escape user/product text. Prices and discount results use integer euro cents with nearest-cent rounding.
Permission tests exercise all 12 role/operation combinations and direct service calls.
Report ranges are inclusive; reversed dates are rejected. Empty periods return zero totals.
Referenced users/products cannot be deleted because sales retain foreign keys.
Product edits do not change historical receipt text or profit.

The user approved three sale_items snapshot columns: delivery_price, product_name and product_sku.
The user's later instruction changes the brief's currency to EUR throughout the application.
The invoice is a simple assignment document using the same sale number, customer, lines and totals.

## Simplicity audit

- Removed the redundant Environment function/files; the initial test checks QSqlDatabase directly.
- Removed Sale.cpp, which only included its data-only header.
- The initial version created its schema directly; the authorized extension adds only the migrations required to preserve existing records.
- Repositories are concrete and short-lived; there are no abstract repository interfaces.
- Widget pointers remain local and owned by Qt; the service is a non-owning pointer.
- Required accessor and constructor repetition stays because the assignment explicitly requires it.
- No frameworks, factories, threads, logging systems, configuration files or extra patterns were introduced.

## Verification and limits

On Windows 11, Qt 6.12.0, GCC 13.1.0 and CMake 3.30.5: all targets build with
-Wall -Wextra and zero compiler warnings; six CTest executables pass.
GUI screenshots were inspected for sales, reports and users. PDF export is tested by its header and file size, parsed and visually inspected.

The brief requested Qt 6.8; the installed 6.12 kit was used. Qt 6.8 and other OS builds are untested.
The original Bulgarian assignment mentioned in the brief was not provided, so this audit covers the supplied brief.

## Authorized extension

The user requested a professional UI, history, CRUD and at least five report types with PDF/XLS.
The original restriction against additional features is superseded for this scope.
Additive migrations now preserve existing records. Updated workflows and verification are in [OPERATIONS](OPERATIONS.md).
