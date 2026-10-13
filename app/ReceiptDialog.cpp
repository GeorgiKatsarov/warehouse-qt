#include "ReceiptDialog.h"
#include <QTextBrowser>
#include <QTextDocument>
#include <QPdfWriter>
#include <QFile>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QPushButton>
#include <QMessageBox>

ReceiptDialog::ReceiptDialog(QWidget *parent) : ReceiptDialog(QString(),parent) {}
ReceiptDialog::ReceiptDialog(QString html, QWidget *parent) : QDialog(parent),m_html(html)
{
    setWindowTitle(tr("Документ за продажба")); resize(760,600);
    auto layout = new QVBoxLayout(this);
    auto browser = new QTextBrowser(this); browser->setObjectName("receiptBrowser");
    browser->setHtml(html); layout->addWidget(browser);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close,this);
    buttons->button(QDialogButtonBox::Close)->setText(tr("Затваряне"));
    auto pdf = buttons->addButton(tr("Запис като PDF"),QDialogButtonBox::ActionRole);
    layout->addWidget(buttons);
    connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::accept);
    connect(pdf,&QPushButton::clicked,this,[this] {
        QString path = QFileDialog::getSaveFileName(this,tr("Запис на документ"),"receipt.pdf",tr("PDF документи (*.pdf)"));
        if (path.isEmpty()) return;
        if (!path.endsWith(".pdf",Qt::CaseInsensitive)) path += ".pdf";
        const QString error = savePdf(path);
        if (!error.isEmpty()) QMessageBox::warning(this,tr("Грешка"),error);
    });
}
void ReceiptDialog::setHtml(QString value)
{
    m_html = value; findChild<QTextBrowser*>("receiptBrowser")->setHtml(value);
}
QString ReceiptDialog::savePdf(const QString &path) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return tr("Документът не може да бъде записан: %1").arg(file.errorString());
    file.close();
    {
        QPdfWriter writer(path); writer.setResolution(96); writer.setPageSize(QPageSize(QPageSize::A4));
        QTextDocument document; document.setHtml(m_html); document.print(&writer);
    }
    if (!file.open(QIODevice::ReadOnly) || file.read(5) != "%PDF-")
        return tr("Неуспешно създаване на PDF.");
    return {};
}
