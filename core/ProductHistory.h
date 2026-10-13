#pragma once
#include <QDateTime>
#include <QString>

/// Product audit entry written by service transactions.
class ProductHistoryEntry
{
public:
    /// Constructs an empty entry.
    ProductHistoryEntry() = default;
    /// Constructs an entry for a product and action.
    ProductHistoryEntry(int productId, QString action) : m_productId(productId), m_action(action) {}
    /// Returns id.
    int id() const { return m_id; }
    /// Sets id.
    void setId(int value) { m_id = value; }
    /// Returns productId.
    int productId() const { return m_productId; }
    /// Sets productId.
    void setProductId(int value) { m_productId = value; }
    /// Returns createdAt.
    QDateTime createdAt() const { return m_createdAt; }
    /// Sets createdAt.
    void setCreatedAt(QDateTime value) { m_createdAt = value; }
    /// Returns actorName.
    QString actorName() const { return m_actorName; }
    /// Sets actorName.
    void setActorName(QString value) { m_actorName = value; }
    /// Returns action.
    QString action() const { return m_action; }
    /// Sets action.
    void setAction(QString value) { m_action = value; }
    /// Returns quantityBefore.
    int quantityBefore() const { return m_quantityBefore; }
    /// Sets quantityBefore.
    void setQuantityBefore(int value) { m_quantityBefore = value; }
    /// Returns quantityAfter.
    int quantityAfter() const { return m_quantityAfter; }
    /// Sets quantityAfter.
    void setQuantityAfter(int value) { m_quantityAfter = value; }
    /// Returns nameBefore.
    QString nameBefore() const { return m_nameBefore; }
    /// Sets nameBefore.
    void setNameBefore(QString value) { m_nameBefore = value; }
    /// Returns nameAfter.
    QString nameAfter() const { return m_nameAfter; }
    /// Sets nameAfter.
    void setNameAfter(QString value) { m_nameAfter = value; }
    /// Returns deliveryBefore.
    qint64 deliveryBefore() const { return m_deliveryBefore; }
    /// Sets deliveryBefore.
    void setDeliveryBefore(qint64 value) { m_deliveryBefore = value; }
    /// Returns deliveryAfter.
    qint64 deliveryAfter() const { return m_deliveryAfter; }
    /// Sets deliveryAfter.
    void setDeliveryAfter(qint64 value) { m_deliveryAfter = value; }
    /// Returns priceBefore.
    qint64 priceBefore() const { return m_priceBefore; }
    /// Sets priceBefore.
    void setPriceBefore(qint64 value) { m_priceBefore = value; }
    /// Returns priceAfter.
    qint64 priceAfter() const { return m_priceAfter; }
    /// Sets priceAfter.
    void setPriceAfter(qint64 value) { m_priceAfter = value; }
    /// Returns saleId.
    int saleId() const { return m_saleId; }
    /// Sets saleId.
    void setSaleId(int value) { m_saleId = value; }
    /// Returns notes.
    QString notes() const { return m_notes; }
    /// Sets notes.
    void setNotes(QString value) { m_notes = value; }
private:
    int m_id = 0;
    int m_productId = 0;
    QDateTime m_createdAt;
    QString m_actorName;
    QString m_action;
    int m_quantityBefore = 0;
    int m_quantityAfter = 0;
    QString m_nameBefore;
    QString m_nameAfter;
    qint64 m_deliveryBefore = 0;
    qint64 m_deliveryAfter = 0;
    qint64 m_priceBefore = 0;
    qint64 m_priceAfter = 0;
    int m_saleId = 0;
    QString m_notes;
};
