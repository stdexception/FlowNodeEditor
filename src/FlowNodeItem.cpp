#include "FlowNodeItem.h"

#include "FlowEditorScene.h"
#include "FlowSocketItem.h"
#include "NodeTypeRegistry.h"

#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace
{
constexpr int kLeftCenter = 2;
constexpr int kRightCenter = 5;
constexpr qreal kSocketSpacing = 22.0;
constexpr qreal kTitleHeight = 24.0;
constexpr qreal kTitleVPadding = 10.0;
constexpr qreal kEdgePadding = 0.0;

QString camelToSnake(const QString &src)
{
    QString out;
    for (int i = 0; i < src.size(); ++i)
    {
        const QChar c = src.at(i);
        if (c.isUpper() && i > 0)
        {
            out.append(QLatin1Char('_'));
        }
        out.append(c.toLower());
    }
    return out;
}

QString capitalizeWords(const QString &src)
{
    const QStringList parts = src.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    QStringList out;
    for (QString p : parts)
    {
        if (!p.isEmpty())
        {
            p[0] = p.at(0).toUpper();
        }
        out.append(p);
    }
    return out.join(QLatin1Char(' '));
}

QString prettyTitleFromType(const QString &nodeType, int occurrenceIndex)
{
    QString snake = camelToSnake(nodeType).replace(QLatin1Char('_'), QLatin1Char(' '));
    QString base = capitalizeWords(snake);
    if (occurrenceIndex > 1)
    {
        base += QLatin1Char('_') + QString::number(occurrenceIndex);
    }
    return base;
}
} // namespace

FlowNodeItem::FlowNodeItem(FlowEditorScene *editorScene, const NodeTypeInfo &info, qint64 nodeId,
                           const QString &titleOverride, bool registerTypeOccurrence,
                           const QVector<qint64> &presetInputSocketIds,
                           const QVector<qint64> &presetOutputSocketIds)
    : m_editorScene(editorScene)
    , m_nodeId(nodeId)
    , m_nodeType(info.nodeType)
    , m_settings(info.defaultSettings)
{
    const int maxSockets = qMax(info.inputs.size(), info.outputs.size());
    m_height = 35.0 + kSocketSpacing * qreal(qMax(1, maxSockets));
    m_width = 150.0;

    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setAcceptHoverEvents(true);

    if (!titleOverride.isEmpty())
    {
        m_title = titleOverride;
    }
    else
    {
        const int occ =
            registerTypeOccurrence ? m_editorScene->registerNodeTypeOccurrence(m_nodeType) : 1;
        m_title = prettyTitleFromType(m_nodeType, occ);
    }

    int ix = 0;
    for (const PortInfo &p : info.inputs)
    {
        const qint64 sid = (ix < presetInputSocketIds.size())
            ? presetInputSocketIds.at(ix)
            : m_editorScene->takeNextId();
        auto *s = new FlowSocketItem(this, ix, true, p.name, p.dataType, info.inputs.size(), sid);
        s->multiEdges = p.multiEdges;
        m_inputs.append(s);
        ++ix;
    }
    ix = 0;
    for (const PortInfo &p : info.outputs)
    {
        const qint64 sid = (ix < presetOutputSocketIds.size())
            ? presetOutputSocketIds.at(ix)
            : m_editorScene->takeNextId();
        auto *s = new FlowSocketItem(this, ix, false, p.name, p.dataType, info.outputs.size(), sid);
        s->multiEdges = p.multiEdges;
        m_outputs.append(s);
        ++ix;
    }

    layoutSockets();
}

QRectF FlowNodeItem::boundingRect() const
{
    return QRectF(0, 0, m_width, m_height);
}

void FlowNodeItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget);
    const bool selected = option->state & QStyle::State_Selected;
    const bool hover = option->state & QStyle::State_MouseOver;

    painter->setRenderHint(QPainter::Antialiasing);

    QRectF body(0, 0, m_width, m_height);
    const QColor bg(QStringLiteral("#171d2a"));
    const QColor bgHover(QStringLiteral("#1b2230"));
    painter->setBrush(hover ? bgHover : bg);
    if (selected)
    {
        painter->setPen(QPen(QColor(QStringLiteral("#28daed")), 1.5));
    }
    else
    {
        painter->setPen(QPen(QColor(QStringLiteral("#28324a")), 1.0));
    }
    painter->drawRoundedRect(body, 6.0, 6.0);

    QRectF titleRect(0, 0, m_width, kTitleHeight + kTitleVPadding);
    painter->setBrush(QColor(QStringLiteral("#222a3b")));
    painter->setPen(Qt::NoPen);
    painter->drawRoundedRect(titleRect.adjusted(0, 0, 0, 6), 6.0, 6.0);

    painter->setPen(QColor(QStringLiteral("#e6ebf2")));
    QFont f(QStringLiteral("Roboto"), 10);
    painter->setFont(f);
    painter->drawText(QRectF(4, 4, m_width - 8, kTitleHeight), Qt::AlignLeft | Qt::AlignVCenter, m_title);
}

QVariant FlowNodeItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged || change == ItemPositionChange)
    {
        if (m_editorScene)
        {
            m_editorScene->refreshEdgesForNode(this);
        }
    }
    return QGraphicsItem::itemChange(change, value);
}

void FlowNodeItem::setTitle(const QString &t)
{
    m_title = t;
    update();
}

FlowSocketItem *FlowNodeItem::findSocketById(qint64 socketId) const
{
    for (FlowSocketItem *s : m_inputs)
    {
        if (s->socketId() == socketId)
        {
            return s;
        }
    }
    for (FlowSocketItem *s : m_outputs)
    {
        if (s->socketId() == socketId)
        {
            return s;
        }
    }
    return nullptr;
}

QPointF FlowNodeItem::socketLocalPos(int index, bool isInput, int countOnSide) const
{
    const int position = isInput ? kLeftCenter : kRightCenter;
    qreal x = 0;
    if (position == kLeftCenter)
    {
        x = -1;
    }
    else
    {
        x = m_width + 1;
    }

    const qreal topOffset = kTitleHeight + 2.0 * kTitleVPadding + kEdgePadding;
    const qreal availableHeight = m_height - topOffset;
    const int numSockets = countOnSide;
    qreal y = topOffset + availableHeight / 2.0 + (qreal(index) - 0.5) * kSocketSpacing;
    if (numSockets > 1)
    {
        y -= kSocketSpacing * qreal(numSockets - 1) / 2.0;
    }
    return QPointF(x, y);
}

void FlowNodeItem::layoutSockets()
{
    for (int i = 0; i < m_inputs.size(); ++i)
    {
        m_inputs[i]->setPos(socketLocalPos(i, true, m_inputs.size()));
    }
    for (int i = 0; i < m_outputs.size(); ++i)
    {
        m_outputs[i]->setPos(socketLocalPos(i, false, m_outputs.size()));
    }
}
