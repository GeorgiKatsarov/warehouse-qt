#include "Discount.h"
#include <QCoreApplication>
#include <limits>

namespace {
qint64 lineTotal(qint64 price, int quantity)
{
    if (price < 0 || quantity <= 0 || price > std::numeric_limits<qint64>::max() / quantity)
        return -1;
    return price * quantity;
}

// Round the discounted line total to the nearest cent; avoid total * 100 overflow.
qint64 discounted(qint64 total, int percent)
{
    if (total < 0)
        return -1;
    const int remaining = 100 - percent;
    return (total / 100) * remaining + ((total % 100) * remaining + 50) / 100;
}
}

NoDiscount::NoDiscount()
    : DiscountStrategy(QCoreApplication::translate("Discount", "Без отстъпка")) {}
QuantityDiscount::QuantityDiscount()
    : DiscountStrategy(QCoreApplication::translate("Discount", "Отстъпка за количество")) {}
VipDiscount::VipDiscount()
    : DiscountStrategy(QCoreApplication::translate("Discount", "VIP отстъпка")) {}

qint64 NoDiscount::apply(qint64 unitPrice, int quantity, bool, QDate) const
{
    return lineTotal(unitPrice, quantity);
}
qint64 QuantityDiscount::apply(qint64 unitPrice, int quantity, bool, QDate) const
{
    return discounted(lineTotal(unitPrice, quantity), quantity >= 50 ? 10 : quantity >= 10 ? 5 : 0);
}
qint64 VipDiscount::apply(qint64 unitPrice, int quantity, bool isVip, QDate) const
{
    return discounted(lineTotal(unitPrice, quantity), isVip ? 10 : 0);
}
SeasonalDiscount::SeasonalDiscount()
    : DiscountStrategy(QCoreApplication::translate("Discount", "Сезонна отстъпка")) {}
SeasonalDiscount::SeasonalDiscount(int percent, QDate from, QDate to)
    : SeasonalDiscount()
{
    setPercent(percent);
    m_from = from;
    m_to = to;
}
void SeasonalDiscount::setPercent(int value) { m_percent = qBound(0, value, 100); }
qint64 SeasonalDiscount::apply(qint64 unitPrice, int quantity, bool, QDate date) const
{
    const bool active = date.isValid() && m_from.isValid() && m_to.isValid()
                        && date >= m_from && date <= m_to;
    return discounted(lineTotal(unitPrice, quantity), active ? m_percent : 0);
}
