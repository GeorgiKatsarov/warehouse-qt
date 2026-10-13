#include "LoginDialog.h"
#include "WarehouseService.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

LoginDialog::LoginDialog(QWidget *parent) : LoginDialog(nullptr,parent) {}
LoginDialog::LoginDialog(WarehouseService *service, QWidget *parent)
    : QDialog(parent), m_service(service)
{
    setWindowTitle(tr("Вход в складовата система"));
    auto layout = new QFormLayout(this);
    auto username = new QLineEdit(this); username->setObjectName("username");
    auto password = new QLineEdit(this); password->setObjectName("password");
    password->setEchoMode(QLineEdit::Password);
    auto errorLabel = new QLabel(this); errorLabel->setObjectName("loginError");
    errorLabel->setWordWrap(true);
    layout->addRow(tr("Потребителско име:"),username);
    layout->addRow(tr("Парола:"),password);
    layout->addRow(errorLabel);
    auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,this);
    buttons->button(QDialogButtonBox::Ok)->setText(tr("Вход"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Отказ"));
    layout->addRow(buttons);
    connect(buttons,&QDialogButtonBox::rejected,this,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,this,[this,username,password,errorLabel] {
        if (!m_service) { errorLabel->setText(tr("Липсва връзка със системата.")); return; }
        const QString error = m_service->login(username->text(),password->text());
        if (error.isEmpty()) accept();
        else { errorLabel->setText(error); password->clear(); password->setFocus(); }
    });
}
