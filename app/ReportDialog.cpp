#include "ReportDialog.h"
#include "ReportExport.h"
#include "ReceiptDialog.h"
#include <QVBoxLayout>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

ReportDialog::ReportDialog(QWidget *parent) : ReportDialog(Report(),parent) {}
ReportDialog::ReportDialog(Report report, QWidget *parent) : QDialog(parent),m_report(report)
{
    resize(1100,720); setWindowTitle(tr("Запазен отчет"));
    auto layout = new QVBoxLayout(this);
    auto browser = new QTextBrowser(this); browser->setObjectName("reportBrowser"); layout->addWidget(browser);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Close,this);
    buttons->button(QDialogButtonBox::Close)->setText(tr("Затваряне"));
    auto pdf = buttons->addButton(tr("Експорт PDF"),QDialogButtonBox::ActionRole);
    auto xls = buttons->addButton(tr("Експорт XLS"),QDialogButtonBox::ActionRole);
    pdf->setObjectName("exportReportPdf"); xls->setObjectName("exportReportXls");
    layout->addWidget(buttons); connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::accept);
    connect(pdf,&QPushButton::clicked,this,[this] {
        QString path = QFileDialog::getSaveFileName(this,tr("Експорт PDF"),QString("report-%1.pdf").arg(m_report.id()),tr("PDF (*.pdf)"));
        if (path.isEmpty()) return;
        if (!path.endsWith(".pdf",Qt::CaseInsensitive)) path += ".pdf";
        const QString error = exportPdf(path); if (!error.isEmpty()) QMessageBox::warning(this,tr("Грешка"),error);
    });
    connect(xls,&QPushButton::clicked,this,[this] {
        QString path = QFileDialog::getSaveFileName(this,tr("Експорт XLS"),QString("report-%1.xls").arg(m_report.id()),tr("Excel 97–2003 (*.xls)"));
        if (path.isEmpty()) return;
        if (!path.endsWith(".xls",Qt::CaseInsensitive)) path += ".xls";
        const QString error = exportXls(path); if (!error.isEmpty()) QMessageBox::warning(this,tr("Грешка"),error);
    });
    setReport(report);
}
void ReportDialog::setReport(Report value)
{
    m_report = value; setWindowTitle(value.title());
    findChild<QTextBrowser*>("reportBrowser")->setHtml(reportHtml(value));
}
QString ReportDialog::exportPdf(const QString &path) const
{
    ReceiptDialog document(reportHtml(m_report),nullptr); return document.savePdf(path,m_report.headers().size()>5);
}
QString ReportDialog::exportXls(const QString &path) const { return saveReportXls(m_report,path); }
