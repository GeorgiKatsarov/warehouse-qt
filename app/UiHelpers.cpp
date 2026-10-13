#include "UiHelpers.h"
#include <QApplication>
#include <QPalette>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QIcon>
#include <QPainter>

void applyTheme(QApplication &application)
{
    application.setStyle("Fusion"); application.setFont(QFont("Segoe UI",10));
    QPalette palette;
    palette.setColor(QPalette::Window,QColor("#f4f7fb")); palette.setColor(QPalette::WindowText,QColor("#17243a"));
    palette.setColor(QPalette::Base,Qt::white); palette.setColor(QPalette::AlternateBase,QColor("#f8fafc"));
    palette.setColor(QPalette::Text,QColor("#17243a")); palette.setColor(QPalette::Button,Qt::white);
    palette.setColor(QPalette::PlaceholderText,QColor("#94a3b8"));
    palette.setColor(QPalette::ButtonText,QColor("#17243a")); palette.setColor(QPalette::Highlight,QColor("#2563eb"));
    palette.setColor(QPalette::HighlightedText,Qt::white); application.setPalette(palette);
    application.setStyleSheet(
        "QMainWindow,QDialog{background:#f4f7fb} QWidget{font-family:'Segoe UI';font-size:10pt}"
        "QLabel{color:#17243a} QLabel[heading='true']{font-size:23pt;font-weight:700;color:#0f172a}"
        "QLabel[subtle='true']{color:#64748b} QLabel[metric='true']{font-size:28pt;font-weight:700;color:#1d4ed8}"
        "QLabel[error='true']{color:#b91c1c;padding:6px} QFrame[card='true']{background:white;border:1px solid #e2e8f0;border-radius:10px}"
        "QLineEdit,QComboBox,QSpinBox,QDateEdit{background:white;border:1px solid #cbd5e1;border-radius:6px;padding:8px;min-height:20px}"
        "QLineEdit:focus,QComboBox:focus,QSpinBox:focus,QDateEdit:focus{border:1px solid #2563eb}"
        "QPushButton{background:white;color:#334155;border:1px solid #cbd5e1;border-radius:6px;padding:9px 15px;font-weight:600}"
        "QPushButton:hover{background:#eff6ff;border-color:#93c5fd} QPushButton:pressed{background:#dbeafe}"
        "QPushButton:disabled{color:#94a3b8;background:#f1f5f9;border-color:#e2e8f0}"
        "QPushButton[primary='true']{background:#2563eb;color:white;border-color:#2563eb}"
        "QPushButton[primary='true']:hover{background:#1d4ed8} QPushButton[primary='true']:disabled{background:#cbd5e1;border-color:#cbd5e1}"
        "QPushButton[danger='true']{color:#b91c1c;border-color:#fecaca;background:#fff7f7}"
        "QPushButton[danger='true']:disabled{color:#94a3b8;border-color:#e2e8f0;background:#f1f5f9}"
        "QTableView,QTableWidget{background:white;alternate-background-color:#f8fafc;border:1px solid #e2e8f0;border-radius:8px;gridline-color:#edf2f7;selection-background-color:#dbeafe;selection-color:#17243a}"
        "QHeaderView::section{background:#edf2f7;color:#475569;font-weight:600;border:0;border-bottom:1px solid #e2e8f0;padding:10px}"
        "QTableView::item{padding:6px;border:0} QTableView::item:selected{background:#dbeafe;color:#17243a}"
        "QTabWidget::pane{border:0} QGroupBox{background:white;border:1px solid #e2e8f0;border-radius:8px;margin-top:14px;padding:16px;font-weight:600}"
        "QGroupBox::title{subcontrol-origin:margin;left:16px;padding:0 6px;color:#334155}"
        "#sidebar{background:#0f172a;border:0} #sidebar QLabel{color:#e2e8f0}"
        "#brand{font-size:20pt;font-weight:700;color:white} #sidebar QListWidget{background:transparent;border:0;outline:0;color:#cbd5e1}"
        "#sidebar QListWidget::item{padding:13px 12px;margin:3px 0;border-radius:6px}"
        "#sidebar QListWidget::item:selected{background:#2563eb;color:white}"
        "#sidebar QListWidget::item:hover{background:#1e293b} #sidebar QListWidget::item:disabled{color:#64748b}"
        "#sidebar QPushButton{background:#1e293b;color:#e2e8f0;border-color:#334155}"
        "QStatusBar{background:white;border-top:1px solid #e2e8f0;color:#64748b;padding:4px}"
        "QTextBrowser{background:white;color:#17243a;border:1px solid #e2e8f0;border-radius:8px;padding:14px}");
}
void addPageHeading(QVBoxLayout *layout, QString title, QString subtitle)
{
    auto heading = new QLabel(title,layout->parentWidget()); heading->setProperty("heading",true);
    heading->setObjectName("pageHeading");
    auto description = new QLabel(subtitle,layout->parentWidget()); description->setProperty("subtle",true);
    description->setObjectName("pageSubtitle");
    description->setWordWrap(true); layout->addWidget(heading); layout->addWidget(description); layout->addSpacing(12);
}
bool confirmAction(QWidget *parent, QString title, QString message)
{
    QMessageBox box(QMessageBox::Question,title,message,QMessageBox::Yes | QMessageBox::No,parent);
    box.button(QMessageBox::Yes)->setText(QObject::tr("Да, потвърждавам"));
    box.button(QMessageBox::No)->setText(QObject::tr("Не, върни ме")); box.setDefaultButton(QMessageBox::No);
    return box.exec()==QMessageBox::Yes;
}

QIcon navigationIcon(int type)
{
    QPixmap image(28,28); image.fill(Qt::transparent);
    QPainter painter(&image); painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#cbd5e1"),2,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
    if (type==0) {
        for (int x : {4,16}) for (int y : {4,16}) painter.drawRoundedRect(QRect(x,y,8,8),1,1);
    } else if (type==1) {
        painter.drawRect(4,8,20,16); painter.drawLine(4,14,24,14); painter.drawLine(11,8,11,14); painter.drawLine(17,8,17,14);
    } else if (type==2) {
        painter.drawLine(3,5,6,5); painter.drawLine(6,5,9,19); painter.drawLine(9,19,23,19);
        painter.drawLine(8,10,24,10); painter.drawLine(24,10,22,16); painter.drawLine(9,16,22,16);
        painter.drawEllipse(QPoint(11,24),1,1); painter.drawEllipse(QPoint(21,24),1,1);
    } else if (type==3) {
        painter.drawEllipse(QRect(4,4,20,20)); painter.drawLine(14,8,14,14); painter.drawLine(14,14,20,17);
    } else if (type==4) {
        painter.drawLine(4,24,25,24); painter.drawRect(6,14,4,10); painter.drawRect(13,7,4,17); painter.drawRect(20,11,4,13);
    } else {
        painter.drawEllipse(QRect(9,3,10,10)); painter.drawArc(QRect(5,15,18,14),0,180*16);
    }
    return QIcon(image);
}
