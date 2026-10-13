#pragma once
#include "Sale.h"

/// Renders a stored report snapshot as styled, escaped HTML.
QString reportHtml(const Report &report);
/// Writes a genuine Excel 97–2003 BIFF8/OLE workbook, without Excel installed.
QString saveReportXls(const Report &report, const QString &path);
