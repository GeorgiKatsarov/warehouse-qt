#pragma once
#include <QDialog>
class WarehouseService;

/// Authenticates through the core service.
class LoginDialog : public QDialog
{
public:
    /// Constructs an unconnected login.
    explicit LoginDialog(QWidget *parent = nullptr);
    /// Constructs a login for a service.
    LoginDialog(WarehouseService *service, QWidget *parent);
    /// Returns the service.
    WarehouseService *service() const { return m_service; }
    /// Sets the service.
    void setService(WarehouseService *value) { m_service = value; }
private:
    WarehouseService *m_service = nullptr;
};
