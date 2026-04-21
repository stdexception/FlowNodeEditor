#include "FlowGraphicsView.h"

#include "FlowEdgeItem.h"
#include "FlowEditorScene.h"
#include "FlowNodeItem.h"
#include "FlowSocketItem.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QScrollBar>

static const QString kNodeMime = QStringLiteral("application/x-flow-node-type");

namespace
{
constexpr int kEdgeBezier = 2;
}

FlowGraphicsView::FlowGraphicsView(FlowEditorScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent)
    , m_flowScene(scene)
{
    setRenderHint(QPainter::Antialiasing);
    setRenderHint(QPainter::TextAntialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setAcceptDrops(true);
}

QPointF FlowGraphicsView::visibleSceneCenter() const
{
    return mapToScene(viewport()->rect().center());
}

QPainterPath FlowGraphicsView::bezierPreview(const QPointF &s, const QPointF &d)
{
    const qreal distX = qAbs(d.x() - s.x());
    const qreal c1x = distX * 0.55;
    const qreal c2x = -distX * 0.25;
    QPainterPath path(s);
    path.cubicTo(s.x() + c1x, s.y(), d.x() + c2x, d.y(), d.x(), d.y());
    return path;
}

FlowSocketItem *FlowGraphicsView::socketAt(const QPoint &viewPos) const
{
    const QPointF scenePos = mapToScene(viewPos);
    const QList<QGraphicsItem *> hits = scene()->items(scenePos);
    for (QGraphicsItem *gi : hits)
    {
        if (gi->type() == FlowSocketItem::Type)
        {
            return static_cast<FlowSocketItem *>(gi);
        }
    }
    return nullptr;
}

void FlowGraphicsView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        FlowSocketItem *sock = socketAt(event->pos());
        if (sock)
        {
            m_dragOrigin = sock;
            if (!m_dragLine)
            {
                m_dragLine = new QGraphicsPathItem;
                m_dragLine->setZValue(-2);
                m_dragLine->setPen(QPen(QColor(QStringLiteral("#FFFFFF")), 0.5));
                m_flowScene->addItem(m_dragLine);
            }
            const QPointF s = m_dragOrigin->pinScenePos();
            m_dragLine->setPath(bezierPreview(s, mapToScene(event->pos())));
            event->accept();
            return;
        }
    }
    QGraphicsView::mousePressEvent(event);
}

void FlowGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragOrigin && m_dragLine)
    {
        const QPointF s = m_dragOrigin->pinScenePos();
        m_dragLine->setPath(bezierPreview(s, mapToScene(event->pos())));
        event->accept();
        return;
    }
    QGraphicsView::mouseMoveEvent(event);
}

void FlowGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragOrigin)
    {
        FlowSocketItem *target = socketAt(event->pos());
        if (m_dragLine)
        {
            m_flowScene->removeItem(m_dragLine);
            delete m_dragLine;
            m_dragLine = nullptr;
        }
        if (target && target != m_dragOrigin)
        {
            m_flowScene->makeEdge(m_dragOrigin, target, -1, kEdgeBezier);
        }
        m_dragOrigin = nullptr;
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
}

void FlowGraphicsView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(kNodeMime))
    {
        event->acceptProposedAction();
    }
    else
    {
        QGraphicsView::dragEnterEvent(event);
    }
}

void FlowGraphicsView::dropEvent(QDropEvent *event)
{
    if (!event->mimeData()->hasFormat(kNodeMime))
    {
        QGraphicsView::dropEvent(event);
        return;
    }
    const QString nodeType = QString::fromUtf8(event->mimeData()->data(kNodeMime));
    const QPointF scenePos = mapToScene(event->pos());
    m_flowScene->spawnNode(nodeType, scenePos);
    event->acceptProposedAction();
}

void FlowGraphicsView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && m_dragOrigin)
    {
        if (m_dragLine)
        {
            m_flowScene->removeItem(m_dragLine);
            delete m_dragLine;
            m_dragLine = nullptr;
        }
        m_dragOrigin = nullptr;
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Delete)
    {
        const QList<QGraphicsItem *> sel = scene()->selectedItems();
        for (QGraphicsItem *gi : sel)
        {
            if (gi->type() == FlowEdgeItem::Type)
            {
                m_flowScene->removeEdge(static_cast<FlowEdgeItem *>(gi));
            }
        }
        for (QGraphicsItem *gi : sel)
        {
            if (gi->type() == FlowNodeItem::Type)
            {
                m_flowScene->removeNode(static_cast<FlowNodeItem *>(gi));
            }
        }
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}
