#pragma once
#include <QDialog>

/// Displays a receipt or invoice and exports it as PDF.
class ReceiptDialog : public QDialog
{
public:
    /// Constructs an empty document viewer.
    explicit ReceiptDialog(QWidget *parent = nullptr);
    /// Constructs a viewer for HTML.
    ReceiptDialog(QString html, QWidget *parent);
    /// Returns the document HTML.
    QString html() const { return m_html; }
    /// Replaces the displayed document.
    void setHtml(QString value);
    /// Writes a PDF and returns an error or an empty string.
    QString savePdf(const QString &path) const;
private:
    QString m_html;
};
