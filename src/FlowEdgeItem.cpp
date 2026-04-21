#include "FlowEdgeItem.h"

#include "FlowSocketItem.h"

#include <QPainter>
#include <QPainterPath>

namespace
{
constexpr int kEdgeBezier = 2;
}

FlowEdgeItem::FlowEdgeItem(FlowSocketItem *startSocket, FlowSocketItem *endSocket, qint64 edgeId, int edgeType)
    : m_start(startSocket)
    , m_end(endSocket)
    , m_edgeId(edgeId)
    , m_edgeType(edgeType)
{
    setZValue(-1);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setPen(QPen(QColor(QStringLiteral("#7f8ba0")), 1.5));

    if (m_start)
    {
        m_start->edges.append(this);
    }
    if (m_end)
    {
        m_end->edges.append(this);
    }

    updatePath();
}

QPainterPath FlowEdgeItem::computeBezier(const QPointF &s, const QPointF &d) const
{
    const qreal distX = qAbs(d.x() - s.x());
    const qreal c1x = distX * 0.55;
    const qreal c2x = -distX * 0.25;

    QPainterPath path(s);
    path.cubicTo(s.x() + c1x, s.y(), d.x() + c2x, d.y(), d.x(), d.y());
    return path;
}

void FlowEdgeItem::updatePath()
{
    m_dragging = false;
    if (!m_start)
    {
        return;
    }

    const QPointF s = m_start->pinScenePos();
    const QPointF d = m_end ? m_end->pinScenePos() : s;
    QPainterPath path;
    if (m_edgeType == kEdgeBezier)
    {
        path = computeBezier(s, d);
    }
    else
    {
        path.moveTo(s);
        path.lineTo(d);
    }
    setPath(path);
}

void FlowEdgeItem::setDragEndPoint(const QPointF &scenePos)
{
    m_dragging = true;
    m_dragEndScene = scenePos;
    if (!m_start)
    {
        return;
    }
    const QPointF s = m_start->pinScenePos();
    QPainterPath path = computeBezier(s, scenePos);
    setPath(path);
}

bool FlowEdgeItem::validateConnection(FlowSocketItem *a, FlowSocketItem *b)
{
    if (!a || !b || a == b)
    {
        return false;
    }
    if (a->flowNode() == b->flowNode())
    {
        return false;
    }
    if (a->isInput() == b->isInput())
    {
        return false;
    }
    if (a->dataType() != b->dataType())
    {
        return false;
    }
    return true;
}
