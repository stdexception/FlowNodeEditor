#include "FlowSocketItem.h"

#include "FlowNodeItem.h"

#include <QBrush>
#include <QFont>
#include <QGraphicsTextItem>
#include <QPen>

namespace
{
constexpr qreal kRadius = 6.0;
}

FlowSocketItem::FlowSocketItem(FlowNodeItem *node, int index, bool isInput, const QString &name,
                               const QString &dataType, int countOnSide, qint64 socketId)
    : QGraphicsEllipseItem(-kRadius, -kRadius, kRadius * 2, kRadius * 2, node)
    , m_node(node)
    , m_index(index)
    , m_isInput(isInput)
    , m_name(name)
    , m_dataType(dataType)
    , m_id(socketId)
{
    Q_UNUSED(countOnSide);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    setPen(QPen(QColor(QStringLiteral("#FFFFFF")), 0.5));
    setBrush(QBrush(QColor(QStringLiteral("#1294ba"))));
    setToolTip(m_dataType);

    auto *label = new QGraphicsTextItem(this);
    QFont f(QStringLiteral("Roboto"), 10);
    label->setFont(f);
    label->setDefaultTextColor(QColor(QStringLiteral("#a2abba")));
    label->setPlainText(m_name);
    if (m_isInput)
    {
        label->setPos(5, -12);
    }
    else
    {
        const qreal w = label->boundingRect().width();
        label->setPos(-(w + 12), -12);
    }
}

QPointF FlowSocketItem::pinScenePos() const
{
    return mapToScene(QPointF(0, 0));
}
