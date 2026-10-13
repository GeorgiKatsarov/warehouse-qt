# Defense notes

## Architecture and polymorphism

**Why separate core from app?** The core only links Core and Sql; it can run in tests without Widgets.
The GUI gathers input and displays service results. Permission and stock rules stay in WarehouseService.

**How does virtual dispatch work?** A shared_ptr<User> can refer to Admin, WarehouseManager or Cashier.
Calling hasPermission() executes the actual object's override. User is abstract because its methods are pure virtual.

**Why Strategy Pattern?** Four interchangeable policies implement the same apply() interface.
The combo box replaces the service's unique_ptr, and checkout calls the selected implementation through the base pointer.
There is no GUI-specific pricing branch in the service.

**Why two constructors and private fields?** Both are explicit assignment requirements.
A default constructor supplies empty data; a parameterized constructor initializes stored fields.
Getters and setters provide controlled public access. Repositories/services perform validation before storage.

## Ownership and integer prices

**shared_ptr versus unique_ptr?** Users may be shared by a collection and the active session.
Exactly one discount strategy belongs to the service, so unique_ptr expresses sole ownership.
Replacing it destroys the old policy automatically.

**Why virtual destructors?** Derived roles/strategies are destroyed through base pointers.
A virtual destructor ensures the derived destructor also runs.

**Why not smart pointers for widgets?** Qt parent-child ownership destroys widgets and their layouts.
Combining a smart pointer and Qt ownership for the same object risks double destruction.
The service pointer in the GUI is non-owning; main keeps the service alive until windows are destroyed.

**Why euro cents?** qint64 avoids binary floating-point money errors.
€12.34 is stored as 1234. We round a discounted line once, to the nearest cent.
3 × 123 cents with VIP: 369 × 90% = 332.1, stored as 332 cents.
Overflow checks reject unrepresentable totals.

## SQLite, stock and historical data

**What protects stock?** One SQL transaction covers stock decreases, sale header and all lines.
A conditional update also requires available quantity. Any failure rolls everything back.

**How is rollback demonstrated?** tst_inventory installs a SQLite trigger that rejects a sale item.
After the failed checkout it verifies unchanged stock, no extra sale header and an unchanged caller sale ID.

**Why snapshot fields?** Current product name/cost can change. A sold line keeps its original cost, name and SKU.
Reports use that cost; receipts use that text. A regression test edits the product and compares the old receipt/profit.

**How do reports work?** Inclusive dates filter sales. Revenue sums discounted line totals;
profit subtracts sale-time delivery cost times quantity; units sold sums quantity.
The report row is then saved.

**How are SQL inputs handled?** QSqlQuery prepared statements bind entered data.
UNIQUE constraints enforce usernames/SKUs; foreign keys preserve referenced sale history.

**How are passwords checked?** The assignment specifies SHA-256 via QCryptographicHash.
Login hashes the entered password and compares the stored hash. A failed login clears the session.

**How is PDF output created?** Core produces escaped HTML. QTextBrowser displays it.
QTextDocument prints the same HTML to QPdfWriter in the GUI.

## Tests and demonstration order

**Which tests prove the requirements?**

| Test | Evidence |
|---|---|
| warehouse_tests | Initial smoke check and actual QSQLITE driver loading |
| tst_inventory | Validation, stock decrease, rejected/failed sales, persistence, snapshots, reports and overflow |
| tst_discounts | 9/10/49/50 boundaries, VIP, inclusive seasonal dates, rounding and strategy swapping |
| tst_permissions | Full role table, direct service refusals, user edits and self-account protection |
| tst_gui | Login/buttons, decimal input, checkout, stock refresh, PDF, reports and user creation |

Run CTest, then demonstrate cashier checkout, manager reports and admin user management.
Show User.h, Discount.h, registerSale(), the rollback test and the UML diagram.

## Scope and decisions

The app is a deliberately small student implementation. Repositories are concrete, data stays plain,
and there is no threading, factory, dependency-injection framework or logging system.
Every hard requirement is mapped in [Compliance audit](COMPLIANCE.md).

Two approved corrections: use EUR, and snapshot cost/name/SKU for sold items.
The installed Qt 6.12 kit was tested; Qt 6.8 was not.
The original Bulgarian assignment text was absent, so evidence is against the supplied brief.
