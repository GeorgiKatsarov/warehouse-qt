#pragma once
#include "Sale.h"
#include <QDialog>

/// Reopens a saved snapshot with PDF and XLS export.
class ReportDialog : public QDialog
{
public:
    /// Constructs an empty viewer.
    explicit ReportDialog(QWidget *parent = nullptr);
    /// Constructs a viewer for a report.
    ReportDialog(Report report, QWidget *parent);
    /// Returns the snapshot.
    Report report() const { return m_report; }
    /// Replaces the viewed snapshot.
    void setReport(Report value);
    /// Exports the displayed snapshot.
    QString exportPdf(const QString &path) const;
    /// Exports the displayed snapshot as XLS.
    QString exportXls(const QString &path) const;
private:
    Report m_report;
};
