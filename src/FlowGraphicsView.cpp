#include "FlowGraphicsView.h"

#include "FlowEdgeItem.h"
#include "FlowEditorScene.h"
#include "FlowEditorSceneHistory.h"
#include "FlowNodeItem.h"
#include "FlowSocketItem.h"
#include "NodeTypeRegistry.h"

#include <QAction>

#include <QContextMenuEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QScrollBar>
#include <QWheelEvent>
#include <utility>
#include <QtGlobal>

static const QString kNodeMime = QStringLiteral("application/x-flow-node-type");
static const char kClipboardMime[] = "application/x-flownodeeditor-clipboard";

namespace
{
constexpr int kEdgeBezier = 2;
constexpr int kEdgeDirect = 1;
constexpr int kEdgeSquare = 3;
constexpr qreal kSnapRadius = 24.0;
constexpr qreal kZoomFactor = 1.15;
constexpr qreal kZoomMin = 0.2;
constexpr qreal kZoomMax = 3.0;
} // namespace

static bool edgeIntersectsRect(FlowEdgeItem *edge, const QRectF &rect)
{
    if (!edge)
    {
        return false;
    }
    QPainterPath r;
    r.addRect(rect);

    // Give the rect a bit of thickness so "almost touching" is considered intersecting (closer to Python's grScene.items(rect)).
    QPainterPathStroker stroker;
    stroker.setWidth(6.0);
    const QPainterPath thick = stroker.createStroke(r);
    return edge->path().intersects(thick);
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
    // Parity with old PyQt view: keep scrollbars hidden; we rely on panning/zooming.
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setAcceptDrops(true);
    setContextMenuPolicy(Qt::DefaultContextMenu);
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

FlowSocketItem *FlowGraphicsView::nearestSocketSnap(const QPointF &scenePos, FlowSocketItem *ignore) const
{
    FlowSocketItem *best = nullptr;
    qreal bestDist = kSnapRadius * kSnapRadius;
    const QRectF scan(scenePos.x() - kSnapRadius, scenePos.y() - kSnapRadius, kSnapRadius * 2, kSnapRadius * 2);
    const QList<QGraphicsItem *> items = scene()->items(scan, Qt::IntersectsItemShape);
    for (QGraphicsItem *gi : items)
    {
        if (gi->type() != FlowSocketItem::Type)
        {
            continue;
        }
        auto *sock = static_cast<FlowSocketItem *>(gi);
        if (sock == ignore)
        {
            continue;
        }
        const QPointF p = sock->pinScenePos() - scenePos;
        const qreal d = QPointF::dotProduct(p, p);
        if (d < bestDist)
        {
            bestDist = d;
            best = sock;
        }
    }
    return best;
}

void FlowGraphicsView::trySnapEdgeTarget(const QPoint &viewPos)
{
    if (!m_dragOrigin || !m_dragLine)
    {
        return;
    }
    QPointF target = mapToScene(viewPos);
    if (FlowSocketItem *snap = nearestSocketSnap(target, m_dragOrigin))
    {
        target = snap->pinScenePos();
    }
    const QPointF s = m_dragOrigin->pinScenePos();
    m_dragLine->setPath(bezierPreview(s, target));
}

void FlowGraphicsView::finishCutMode()
{
    if (m_cutRubber)
    {
        m_flowScene->removeItem(m_cutRubber);
        delete m_cutRubber;
        m_cutRubber = nullptr;
    }
    if (m_cutPoints.size() >= 2)
    {
        m_flowScene->cutEdgesAlongPolyline(m_cutPoints);
    }
    m_cutPoints.clear();
    m_mode = Mode::Noop;
    unsetCursor();
}

void FlowGraphicsView::mousePressEvent(QMouseEvent *event)
{
    m_lastSceneMousePos = mapToScene(event->pos());
    emit sceneCursorPosChanged(qRound(m_lastSceneMousePos.x()), qRound(m_lastSceneMousePos.y()));

    if (event->button() == Qt::LeftButton && (event->modifiers() & Qt::ControlModifier))
    {
        if (!itemAt(event->pos()))
        {
            m_mode = Mode::EdgeCut;
            m_cutPoints.clear();
            m_cutPoints.append(m_lastSceneMousePos);
            if (!m_cutRubber)
            {
                m_cutRubber = new QGraphicsPathItem;
                m_cutRubber->setZValue(1000);
                QPen cutPen(QColor(QStringLiteral("#ff7a59")), 2, Qt::DashLine);
                cutPen.setCosmetic(true);
                m_cutRubber->setPen(cutPen);
                m_flowScene->addItem(m_cutRubber);
            }
            QPainterPath p(m_lastSceneMousePos);
            m_cutRubber->setPath(p);
            setCursor(Qt::CrossCursor);
            event->accept();
            return;
        }
    }

    if (event->button() == Qt::LeftButton && m_mode == Mode::Noop)
    {
        FlowSocketItem *sock = socketAt(event->pos());
        if (sock)
        {
            if ((event->modifiers() & Qt::ControlModifier) && !sock->edges.isEmpty())
            {
                startEdgeReroute(sock);
                event->accept();
                return;
            }
            m_mode = Mode::EdgeDrag;
            m_dragOrigin = sock;
            if (!m_dragLine)
            {
                m_dragLine = new QGraphicsPathItem;
                m_dragLine->setZValue(-2);
                QPen wirePen(QColor(QStringLiteral("#7fe9f5")), 1.0);
                wirePen.setCosmetic(true);
                m_dragLine->setPen(wirePen);
                m_flowScene->addItem(m_dragLine);
            }
            const QPointF s = m_dragOrigin->pinScenePos();
            m_dragLine->setPath(bezierPreview(s, mapToScene(event->pos())));
            event->accept();
            return;
        }

        QGraphicsItem *hit = itemAt(event->pos());
        if (hit)
        {
            m_maybeDragNode = true;
            m_draggedNode = (hit->type() == FlowNodeItem::Type) ? static_cast<FlowNodeItem *>(hit) : nullptr;
            m_pressViewPos = event->pos();
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void FlowGraphicsView::mouseMoveEvent(QMouseEvent *event)
{
    m_lastSceneMousePos = mapToScene(event->pos());
    emit sceneCursorPosChanged(qRound(m_lastSceneMousePos.x()), qRound(m_lastSceneMousePos.y()));

    if (m_mode == Mode::EdgeCut && (event->buttons() & Qt::LeftButton))
    {
        m_cutPoints.append(m_lastSceneMousePos);
        QPainterPath p;
        if (!m_cutPoints.isEmpty())
        {
            p.moveTo(m_cutPoints.first());
            for (int i = 1; i < m_cutPoints.size(); ++i)
            {
                p.lineTo(m_cutPoints.at(i));
            }
        }
        if (m_cutRubber)
        {
            m_cutRubber->setPath(p);
        }
        event->accept();
        return;
    }

    if (m_mode == Mode::EdgeDrag && m_dragOrigin && m_dragLine)
    {
        trySnapEdgeTarget(event->pos());
        event->accept();
        return;
    }

    if (m_mode == Mode::EdgeReroute && m_rerouteStart)
    {
        updateEdgeReroutePreview(event->pos());
        event->accept();
        return;
    }

    QGraphicsView::mouseMoveEvent(event);
}

void FlowGraphicsView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_mode == Mode::EdgeCut)
    {
        finishCutMode();
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_mode == Mode::EdgeDrag && m_dragOrigin)
    {
        FlowSocketItem *target = socketAt(event->pos());
        if (!target)
        {
            target = nearestSocketSnap(mapToScene(event->pos()), m_dragOrigin);
        }
        if (m_dragLine)
        {
            m_flowScene->removeItem(m_dragLine);
            delete m_dragLine;
            m_dragLine = nullptr;
        }
        if (target && target != m_dragOrigin)
        {
            if (m_flowScene->makeEdge(m_dragOrigin, target, -1, kEdgeBezier))
            {
                m_flowScene->history()->storeHistory(QStringLiteral("Created edge"));
            }
        }
        m_dragOrigin = nullptr;
        m_mode = Mode::Noop;
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_mode == Mode::EdgeReroute && m_rerouteStart)
    {
        FlowSocketItem *target = socketAt(event->pos());
        if (!target)
        {
            target = nearestSocketSnap(mapToScene(event->pos()), m_rerouteStart);
        }
        finishEdgeReroute(target);
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton && m_maybeDragNode)
    {
        m_maybeDragNode = false;
        const bool moved = (event->pos() - m_pressViewPos).manhattanLength() > 3;
        bool splitDone = false;
        if (moved && m_draggedNode)
        {
            splitDone = tryDropNodeOnEdgeSplit(m_draggedNode);
        }
        if (moved && !splitDone)
        {
            m_flowScene->history()->storeHistory(QStringLiteral("Node moved"));
        }
        m_draggedNode = nullptr;
    }

    QGraphicsView::mouseReleaseEvent(event);
}

bool FlowGraphicsView::tryDropNodeOnEdgeSplit(FlowNodeItem *node)
{
    if (!node)
    {
        return false;
    }

    // Only split if node has one input+output available and no existing connections (matches old Python behavior).
    if (node->inputs().isEmpty() || node->outputs().isEmpty())
    {
        return false;
    }
    for (FlowSocketItem *s : (node->inputs() + node->outputs()))
    {
        if (!s->edges.isEmpty())
        {
            return false;
        }
    }

    const QRectF nodeRect = node->sceneBoundingRect();
    const QList<QGraphicsItem *> hits = scene()->items(nodeRect, Qt::IntersectsItemShape);

    FlowEdgeItem *hitEdge = nullptr;
    for (QGraphicsItem *gi : hits)
    {
        if (gi->type() != FlowEdgeItem::Type)
        {
            continue;
        }
        auto *edge = static_cast<FlowEdgeItem *>(gi);
        if (edge->startSocket() && edge->startSocket()->flowNode() == node)
        {
            continue;
        }
        if (edge->endSocket() && edge->endSocket()->flowNode() == node)
        {
            continue;
        }
        if (!edgeIntersectsRect(edge, nodeRect))
        {
            continue;
        }
        hitEdge = edge;
        break;
    }

    if (!hitEdge || !hitEdge->startSocket() || !hitEdge->endSocket())
    {
        return false;
    }

    FlowSocketItem *a = hitEdge->startSocket();
    FlowSocketItem *b = hitEdge->endSocket();
    if (!a || !b)
    {
        return false;
    }

    // Ensure a is output, b is input.
    if (!a->isInput() && b->isInput())
    {
        // ok
    }
    else if (a->isInput() && !b->isInput())
    {
        qSwap(a, b);
    }
    else
    {
        return false;
    }

    FlowSocketItem *nodeIn = node->inputs().first();
    FlowSocketItem *nodeOut = node->outputs().first();

    m_flowScene->beginBulkEdit();
    const int edgeType = hitEdge->edgeType();
    m_flowScene->removeEdge(hitEdge);
    m_flowScene->makeEdge(a, nodeIn, -1, edgeType);
    m_flowScene->makeEdge(nodeOut, b, -1, edgeType);
    m_flowScene->endBulkEdit();
    m_flowScene->history()->storeHistory(QStringLiteral("Insert node on edge"), true);
    return true;
}

void FlowGraphicsView::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier)
    {
        const qreal factor = event->angleDelta().y() > 0 ? kZoomFactor : (1.0 / kZoomFactor);
        qreal nz = m_zoom * factor;
        nz = qBound(kZoomMin, nz, kZoomMax);
        const qreal apply = nz / m_zoom;
        m_zoom = nz;
        scale(apply, apply);
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}

void FlowGraphicsView::contextMenuEvent(QContextMenuEvent *event)
{
    const QPoint viewPos = viewport()->mapFromGlobal(event->globalPos());
    QGraphicsItem *item = itemAt(viewPos);
    if (item && (item->type() == FlowSocketItem::Type || item->type() == FlowNodeItem::Type))
    {
        QGraphicsView::contextMenuEvent(event);
        return;
    }
    if (item && item->type() == FlowEdgeItem::Type)
    {
        auto *edge = static_cast<FlowEdgeItem *>(item);
        QMenu menu(this);
        QAction *aBezier = menu.addAction(tr("Bezier edge"));
        QAction *aDirect = menu.addAction(tr("Direct edge"));
        QAction *aSquare = menu.addAction(tr("Square edge"));
        QAction *chosen = menu.exec(event->globalPos());
        if (chosen == aBezier)
        {
            edge->setEdgeType(kEdgeBezier);
        }
        else if (chosen == aDirect)
        {
            edge->setEdgeType(kEdgeDirect);
        }
        else if (chosen == aSquare)
        {
            edge->setEdgeType(kEdgeSquare);
        }
        edge->updatePath();
        if (chosen)
        {
            m_flowScene->history()->storeHistory(QStringLiteral("Changed edge type"));
        }
        event->accept();
        return;
    }

    if (!item || item->type() == FlowNodeItem::Type)
    {
        if (item && item->type() == FlowNodeItem::Type)
        {
            QGraphicsView::contextMenuEvent(event);
            return;
        }

        NodeTypeRegistry *reg = m_flowScene->registry();
        if (!reg)
        {
            QGraphicsView::contextMenuEvent(event);
            return;
        }
        QMenu root(this);
        for (const QString &cat : reg->categories())
        {
            QMenu *sub = root.addMenu(cat);
            for (const NodeTypeInfo &info : reg->typesInCategory(cat))
            {
                QAction *a = sub->addAction(info.nodeType);
                a->setData(info.nodeType);
            }
        }
        QAction *ac = root.exec(event->globalPos());
        if (ac)
        {
            const QString nt = ac->data().toString();
            const QPointF sp = mapToScene(viewPos);
            m_flowScene->spawnNode(nt, sp);
            m_flowScene->history()->storeHistory(QStringLiteral("Created node"));
        }
        event->accept();
        return;
    }

    QGraphicsView::contextMenuEvent(event);
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
    m_flowScene->history()->storeHistory(QStringLiteral("Created node"));
    event->acceptProposedAction();
}

void FlowGraphicsView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && m_mode == Mode::EdgeDrag && m_dragOrigin)
    {
        if (m_dragLine)
        {
            m_flowScene->removeItem(m_dragLine);
            delete m_dragLine;
            m_dragLine = nullptr;
        }
        m_dragOrigin = nullptr;
        m_mode = Mode::Noop;
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape && m_mode == Mode::EdgeReroute && m_rerouteStart)
    {
        finishEdgeReroute(nullptr);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Escape && m_mode == Mode::EdgeCut)
    {
        if (m_cutRubber)
        {
            m_flowScene->removeItem(m_cutRubber);
            delete m_cutRubber;
            m_cutRubber = nullptr;
        }
        m_cutPoints.clear();
        m_mode = Mode::Noop;
        unsetCursor();
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_Delete)
    {
        const QList<QGraphicsItem *> sel = scene()->selectedItems();
        if (!sel.isEmpty())
        {
            m_flowScene->beginBulkEdit();
        }
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
        if (!sel.isEmpty())
        {
            m_flowScene->endBulkEdit();
            m_flowScene->history()->storeHistory(QStringLiteral("Delete"), true);
        }
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void FlowGraphicsView::startEdgeReroute(FlowSocketItem *startSocket)
{
    if (!startSocket || startSocket->edges.isEmpty())
    {
        return;
    }

    // Reset any previous reroute state if present.
    finishEdgeReroute(nullptr);

    m_mode = Mode::EdgeReroute;
    m_rerouteStart = startSocket;
    m_rerouteAffectedEdges = startSocket->edges;

    // Hide affected edges and build dashed previews from the "other" socket to the mouse.
    m_rerouteOtherSockets.clear();
    m_reroutePreview.clear();

    const QPointF startPos = startSocket->pinScenePos();
    for (FlowEdgeItem *e : m_rerouteAffectedEdges)
    {
        if (!e || !e->startSocket() || !e->endSocket())
        {
            continue;
        }
        e->setVisible(false);
        FlowSocketItem *other = (e->startSocket() == startSocket) ? e->endSocket() : e->startSocket();
        if (!other)
        {
            continue;
        }
        m_rerouteOtherSockets.append(other);

        auto *pv = new QGraphicsPathItem;
        pv->setZValue(-2);
        QPen pen(QColor(QStringLiteral("#bbbbbb")), 1.0, Qt::DashLine);
        pen.setCosmetic(true);
        pv->setPen(pen);
        pv->setBrush(Qt::NoBrush);
        pv->setPath(bezierPreview(other->pinScenePos(), startPos));
        m_flowScene->addItem(pv);
        m_reroutePreview.append(pv);
    }
}

void FlowGraphicsView::updateEdgeReroutePreview(const QPoint &viewPos)
{
    if (!m_rerouteStart)
    {
        return;
    }
    QPointF target = mapToScene(viewPos);
    if (FlowSocketItem *snap = nearestSocketSnap(target, m_rerouteStart))
    {
        target = snap->pinScenePos();
    }

    const int n = qMin(m_rerouteOtherSockets.size(), m_reroutePreview.size());
    for (int i = 0; i < n; ++i)
    {
        FlowSocketItem *other = m_rerouteOtherSockets.at(i);
        QGraphicsPathItem *pv = m_reroutePreview.at(i);
        if (!other || !pv)
        {
            continue;
        }
        pv->setPath(bezierPreview(other->pinScenePos(), target));
    }
}

void FlowGraphicsView::finishEdgeReroute(FlowSocketItem *targetSocket)
{
    // If we are not in reroute mode, do nothing.
    if (!m_rerouteStart)
    {
        return;
    }

    // Restore visibility of affected edges (we may recreate them below).
    for (FlowEdgeItem *e : qAsConst(m_rerouteAffectedEdges))
    {
        if (e)
        {
            e->setVisible(true);
        }
    }

    // Remove preview items.
    for (QGraphicsPathItem *pv : qAsConst(m_reroutePreview))
    {
        if (!pv)
        {
            continue;
        }
        m_flowScene->removeItem(pv);
        delete pv;
    }
    m_reroutePreview.clear();
    m_rerouteOtherSockets.clear();

    const bool cancel = (targetSocket == nullptr || targetSocket == m_rerouteStart);
    if (!cancel)
    {
        // Reconnect all valid edges to targetSocket, preserving IDs and edge types.
        m_flowScene->beginBulkEdit();
        for (FlowEdgeItem *e : qAsConst(m_rerouteAffectedEdges))
        {
            if (!e || !e->startSocket() || !e->endSocket())
            {
                continue;
            }

            FlowSocketItem *other = (e->startSocket() == m_rerouteStart) ? e->endSocket() : e->startSocket();
            if (!other)
            {
                continue;
            }

            // Validate connection (makeEdge will also enforce direction and multi-edge constraints).
            if (!FlowEdgeItem::validateConnection(other, targetSocket)
                && !FlowEdgeItem::validateConnection(targetSocket, other))
            {
                continue;
            }

            const qint64 id = e->edgeId();
            const int type = e->edgeType();
            m_flowScene->removeEdge(e);
            m_flowScene->makeEdge(other, targetSocket, id, type);
        }
        m_flowScene->endBulkEdit();
        m_flowScene->history()->storeHistory(QStringLiteral("Rerouted edges"), true);
    }

    m_rerouteStart = nullptr;
    m_rerouteAffectedEdges.clear();
    m_mode = Mode::Noop;
}
