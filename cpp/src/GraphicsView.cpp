#include "GraphicsView.hpp"
#include "GraphicsScene.hpp"
#include "GraphicsSocket.hpp"
#include "GraphicsNode.hpp"
#include "GraphicsEdge.hpp"
#include "EditorScene.hpp"
#include "SceneClipboard.hpp"
#include "SceneHistory.hpp"

#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QJsonDocument>
#include <QList>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QWheelEvent>

GraphicsView::GraphicsView(GraphicsScene* grScene, QWidget* parent)
    : QGraphicsView(grScene, parent)
{
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::RubberBandDrag);
    setBackgroundBrush(QColor(QStringLiteral("#202b3c")));
    setContextMenuPolicy(Qt::DefaultContextMenu);
}

void GraphicsView::resetMode()
{
    mode_ = ViewInteractionMode::NoOp;
    if (dragMode() != RubberBandDrag)
        setDragMode(RubberBandDrag);
}

void GraphicsView::deleteSelectedItems()
{
    if (!scene() || !editorScene_)
        return;

    QList<QGraphicsItem*> sel = scene()->selectedItems();
    QList<GraphicsEdge*> edges;
    QList<GraphicsNode*> nodes;
    for (QGraphicsItem* it : sel) {
        if (auto* e = qgraphicsitem_cast<GraphicsEdge*>(it))
            edges.append(e);
        else if (auto* n = qgraphicsitem_cast<GraphicsNode*>(it))
            nodes.append(n);
    }
    for (GraphicsEdge* e : edges) {
        e->removeFromDocument();
        delete e;
    }
    for (GraphicsNode* n : nodes)
        n->removeFromDocument();
}

void GraphicsView::wheelEvent(QWheelEvent* event)
{
    const QPoint numDegrees = event->angleDelta() / 8;
    if (numDegrees.y() == 0) {
        QGraphicsView::wheelEvent(event);
        return;
    }
    const int steps = numDegrees.y() / 15;
    if (steps > 0) {
        if (zoom_ < zoomMax_) {
            ++zoom_;
            scale(zoomFactor_, zoomFactor_);
        }
    } else {
        if (zoom_ > zoomMin_) {
            --zoom_;
            scale(1.0 / zoomFactor_, 1.0 / zoomFactor_);
        }
    }
    event->accept();
}

QGraphicsItem* GraphicsView::itemAtClick(const QMouseEvent* event) const
{
    return itemAt(event->pos());
}

bool GraphicsView::distanceClickReleaseExceedsThreshold(const QMouseEvent* event) const
{
    const QPointF releaseScene = mapToScene(event->pos());
    const QPointF d = releaseScene - lastLmbScenePos_;
    const qreal distSq = d.x() * d.x() + d.y() * d.y();
    const qreal th = static_cast<qreal>(kEdgeDragThresholdPx);
    return distSq > th * th;
}

void GraphicsView::contextMenuEvent(QContextMenuEvent* event)
{
    if (!editorScene_ || !editorScene_->clipboard()) {
        QGraphicsView::contextMenuEvent(event);
        return;
    }
    QMenu menu(this);
    QAction* paste = menu.addAction(tr("Paste"));
    paste->setShortcut(QKeySequence::Paste);
    QAction* chosen = menu.exec(event->globalPos());
    if (chosen == paste) {
        const QJsonObject data = QJsonDocument::fromJson(QApplication::clipboard()->text().toUtf8()).object();
        if (!data.isEmpty())
            editorScene_->clipboard()->deserializeFromClipboard(data);
    }
}

void GraphicsView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        lastLmbScenePos_ = mapToScene(event->pos());

        QGraphicsItem* item = itemAtClick(event);
        if (auto* sock = qgraphicsitem_cast<GraphicsSocket*>(item)) {
            mode_ = ViewInteractionMode::EdgeDrag;
            setDragMode(QGraphicsView::NoDrag);
            dragging_.edgeDragStart(sock);
            event->accept();
            return;
        }
    }

    QGraphicsView::mousePressEvent(event);
}

void GraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    const QPointF sp = mapToScene(event->pos());
    lastSceneMousePosition_ = sp;
    emit scenePosChanged(static_cast<int>(sp.x()), static_cast<int>(sp.y()));

    if (mode_ == ViewInteractionMode::EdgeDrag && dragging_.isDragging())
        dragging_.updateDestination(sp.x(), sp.y());

    QGraphicsView::mouseMoveEvent(event);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && mode_ == ViewInteractionMode::EdgeDrag) {
        if (distanceClickReleaseExceedsThreshold(event)) {
            QGraphicsItem* item = itemAtClick(event);
            auto* sock = qgraphicsitem_cast<GraphicsSocket*>(item);
            dragging_.edgeDragEnd(sock);
        } else {
            dragging_.cancelDrag();
        }
        resetMode();
        event->accept();
        return;
    }

    if (event->button() == Qt::RightButton && mode_ == ViewInteractionMode::EdgeDrag) {
        dragging_.cancelDrag();
        resetMode();
    }

    QGraphicsView::mouseReleaseEvent(event);
}
