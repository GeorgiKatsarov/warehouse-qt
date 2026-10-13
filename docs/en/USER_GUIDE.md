# User guide

## Login and common navigation

1. Start warehouse_app and enter a default login from [README](README.md).
2. Use the Products, Sales and Reports tabs. Unavailable operations are disabled.
3. Administrators additionally see Users. Close and restart the app to change login.

All money is in EUR. Enter prices such as 12.34 or 12,34; at most two decimal places are accepted.
The app keeps its database between sessions.

## Products — administrator and warehouse manager

1. Open Products; the table shows name, SKU, stock, supplier and both prices.
2. Choose Добавяне. Enter a nonempty name and unique SKU, nonnegative stock and prices; supplier is optional.
3. Choose Запис. Duplicate SKUs are rejected by SQLite.
4. Select a row and choose Редактиране to change it.
5. Select a row and choose Изтриване, then confirm. Sold products cannot be deleted.
6. Обновяване reloads data; returning to the tab also refreshes stock.

Cashiers can view the catalog but cannot change it.

## Sales and documents — administrator and cashier

1. Open Sales, choose a product and quantity, then Добавяне в количката.
   Repeated additions of one product merge into one cart line.
2. Use Премахване на ред to remove the selected line.
3. Enter the customer, or leave blank for Клиент на дребно.
4. Tick VIP клиент when appropriate and choose one discount policy.
   Seasonal discount also requires a percentage and inclusive date interval.
5. Check the live total. Insufficient stock or an invalid period disables completion.
6. Tick Издаване на фактура for an invoice; otherwise a receipt is issued.
7. Choose Завършване на продажбата. Successful checkout saves the sale, reduces stock and clears the cart.
8. In the document window choose Запис като PDF and a file path, then Затваряне.

Quantity and VIP discounts are alternatives; they do not combine.
The simple invoice uses the sale number, customer and line totals.
Product changes after a sale do not alter that sale's document.

## Reports — administrator and warehouse manager

1. Open Reports and select От and До; both endpoint dates are included.
2. Choose Генериране.
3. Read the saved row: revenue, profit and units sold.
   Profit subtracts delivery costs from discounted revenue.
4. A period without sales produces zero totals. Reversed dates are rejected.

Every generated report remains saved even if a new report uses the same interval.

## Users — administrator only

1. Open Users and choose Нов потребител to clear the form.
2. Enter username, full name, password and role, then Запис.
3. Click a saved user's row to edit it. A blank password preserves the current hash.
4. Choose Запис to save the edits.
5. To delete, select a user, choose Изтриване and confirm.

Usernames must be unique. Your active admin account cannot be deleted or demoted.
Users referenced by sales cannot be deleted.

## Verification demonstration

Log in as cashier and show disabled product editing/report access.
Complete a sale of 10 P001 pencils with VIP policy: €9.00 revenue, €4.00 profit and 10 units sold.
Log in as manager to generate the corresponding report and show that sales are disabled.
Log in as admin to demonstrate Users. These demo results assume the original seeded prices.
