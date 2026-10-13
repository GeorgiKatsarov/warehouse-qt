# Extended workflows

## Navigation and product history

The application now opens an overview dashboard with product counts, low-stock alerts,
warehouse units and today's sales/revenue. The sidebar provides Products, New sale,
Sales history, Reports and Users according to role. Ctrl+R refreshes the active page.
Switch user closes the current session; unfinished carts are not persisted.

Products supports name/SKU/supplier search, low-stock filtering and column sorting.
Double-click a row, choose Details and history, or use the right-click menu.
The profile shows an immutable timeline of stock, price and product changes with actor and sale number.
Old sales are imported during upgrade. Unknown old before/after stock is shown as a dash rather than invented.

Create/edit/delete product and user actions remain permission checked.
Products referenced by any sale and users referenced by sales cannot be physically deleted.
Product IDs are not reused after deletion, so different products cannot share an audit timeline.

## Sales CRUD and validation

Create sales from New sale. Use Sales history to read documents, edit an active sale,
or cancel it. Cashiers see and modify their own sales; administrators can manage all sales.
Cancellation is the safe delete operation: it restores stock but retains the original sale and audit trail.
Cancelled records cannot be edited or cancelled twice and are excluded from newly generated financial reports.

Corrections restore the old quantities, validate the new cart, recalculate at current product prices
with the currently selected discount, and save new quantities inside one transaction.
The correction window clearly states that prices and discounts are recalculated.
Quantity/VIP policies are selected from the original sale; a seasonal percentage and interval must be entered again.
Any stock, SQL, item-save or audit failure rolls back the complete correction or cancellation.

Names/suppliers are limited to 160 characters; SKUs to 64.
Usernames are 3–64 Latin letters, digits, dots, underscores or hyphens.
New/reset passwords require 8–128 characters; unchanged existing passwords remain valid.
Customer names are limited to 160 characters and carts to 1000 distinct product lines.
Prices, quantities, dates, duplicate SKUs/usernames and signed-in permissions are validated.
The active admin and the last administrator are protected. User edits/deletes are transactional.
Permissions are reloaded from SQLite for each protected operation, so a removed/demoted session loses access.

## Five saved report types

| Type | Content |
|---|---|
| Financial summary | Revenue, delivery cost, profit, units and sale count |
| Sales by product | SKU/name, sale count, units, revenue, cost and profit |
| Daily sales | Daily sale count, units, revenue, cost and profit |
| Inventory valuation | Current stock and unit/extended cost and retail value |
| Low stock | Products with 10 or fewer units, prices and stock values |

Financial reports use inclusive date ranges and exclude cancelled sales.
Inventory reports snapshot stock at generation time; their date selectors do not reconstruct past stock.
The reports table stores type, title and a JSON snapshot of column names, rows and numeric formats.
Results remain exactly as generated after product edits, sale corrections and restarts.

Generate a report, search the archive, and double-click or choose Open.
Rename changes its title only. Delete removes that saved snapshot after confirmation.
PDF and XLS can be exported directly from the archive or from an opened report.
Legacy summary reports reopen from their original saved totals; unavailable old sale counts are marked unknown.

## Export and preservation

PDF uses A4 landscape for wide reports and portrait for receipts. Cyrillic and EUR are preserved.
Both PDF and XLS exports use QSaveFile, so a failed write does not truncate an existing destination.
HTML escapes entered text. XLS exports text cells as text, including values beginning with =;
it does not create formulas from entered data.

XLS is genuine BIFF8 in an OLE Compound File, not renamed CSV or HTML.
Money is numeric EUR with a currency number format. Large integers beyond 15 significant digits
are exported as text to preserve exact values. Limits: 65,530 data rows, 256 columns,
4,000 characters in a text cell and approximately 7 MB per workbook.
No Excel installation or new application library is required.
The format follows Microsoft's [XLS](https://learn.microsoft.com/en-us/openspecs/office_file_formats/ms-xls/cd03cb5f-ca02-4934-a391-bb674cb8aa06)
and [Compound File](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-cfb/68530324-9d3d-4441-9ea9-66a2c8f79567) specifications.

Existing databases receive additive schema upgrades inside a transaction.
Existing products, users, sales and reports are retained. Audit records are intentionally read-only.

## Verification

Six CTest executables cover the original requirements plus report snapshots/CRUD,
sale editing/cancellation rollback, history isolation, old-schema upgrades, password validation,
session permission refresh, product double-click and report export.
PDFs are parsed/rendered and XLS workbooks independently read with xlrd in development QA.
No QA-only Python packages are runtime dependencies.
