#pragma once
#include <QDate>
#include <QString>

/// Strategy Pattern interface; apply returns a discounted line total.
class DiscountStrategy
{
public:
    /// Constructs a strategy with no label.
    DiscountStrategy() = default;
    /// Constructs a strategy with its receipt label.
    explicit DiscountStrategy(QString label) : m_label(label) {}
    virtual ~DiscountStrategy() = default;
    /// Calculates a line total in cents.
    virtual qint64 apply(qint64 unitPrice, int quantity, bool isVip, QDate date) const = 0;
    /// Returns the Bulgarian receipt label.
    virtual QString name() const = 0;
    /// Returns the label.
    QString label() const { return m_label; }
    /// Sets the label.
    void setLabel(QString value) { m_label = value; }
private:
    QString m_label;
};

/// NoDiscount price policy.
class NoDiscount : public DiscountStrategy
{
public:
    /// Constructs the default policy.
    NoDiscount();
    /// Constructs a policy with a custom label.
    explicit NoDiscount(QString label) : DiscountStrategy(label) {}
    /// Calculates the line total.
    qint64 apply(qint64 unitPrice, int quantity, bool isVip, QDate date) const override;
    /// Returns the receipt label.
    QString name() const override { return label(); }
};

/// QuantityDiscount price policy.
class QuantityDiscount : public DiscountStrategy
{
public:
    /// Constructs the default policy.
    QuantityDiscount();
    /// Constructs a policy with a custom label.
    explicit QuantityDiscount(QString label) : DiscountStrategy(label) {}
    /// Calculates the line total.
    qint64 apply(qint64 unitPrice, int quantity, bool isVip, QDate date) const override;
    /// Returns the receipt label.
    QString name() const override { return label(); }
};

/// VipDiscount price policy.
class VipDiscount : public DiscountStrategy
{
public:
    /// Constructs the default policy.
    VipDiscount();
    /// Constructs a policy with a custom label.
    explicit VipDiscount(QString label) : DiscountStrategy(label) {}
    /// Calculates the line total.
    qint64 apply(qint64 unitPrice, int quantity, bool isVip, QDate date) const override;
    /// Returns the receipt label.
    QString name() const override { return label(); }
};

/// Percentage discount on an inclusive date interval.
class SeasonalDiscount : public DiscountStrategy
{
public:
    /// Constructs an inactive seasonal discount.
    SeasonalDiscount();
    /// Constructs a percentage and inclusive dates.
    SeasonalDiscount(int percent, QDate from, QDate to);
    /// Calculates the line total.
    qint64 apply(qint64 unitPrice, int quantity, bool isVip, QDate date) const override;
    /// Returns the receipt label.
    QString name() const override { return label(); }
    /// Returns the percentage.
    int percent() const { return m_percent; }
    /// Sets the percentage, clamped to 0..100.
    void setPercent(int value);
    /// Returns the first eligible date.
    QDate from() const { return m_from; }
    /// Sets the first eligible date.
    void setFrom(QDate value) { m_from = value; }
    /// Returns the last eligible date.
    QDate to() const { return m_to; }
    /// Sets the last eligible date.
    void setTo(QDate value) { m_to = value; }
private:
    int m_percent = 0;
    QDate m_from;
    QDate m_to;
};
