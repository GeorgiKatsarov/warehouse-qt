#pragma once
#include <QMainWindow>
class WarehouseService;

/// Role-aware main application window.
class MainWindow : public QMainWindow
{
public:
    /// Constructs an unconnected window.
    explicit MainWindow(QWidget *parent = nullptr);
    /// Constructs the window for a logged-in service.
    MainWindow(WarehouseService *service, QWidget *parent);
    /// Returns the service.
    WarehouseService *service() const { return m_service; }
    /// Rebuilds the window with a service.
    void setService(WarehouseService *value);
private:
    WarehouseService *m_service = nullptr;
    void build();
    QWidget *productsTab();
};
