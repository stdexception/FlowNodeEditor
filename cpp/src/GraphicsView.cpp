#include "GraphicsView.hpp"
#include "GraphicsScene.hpp"
#include "GraphicsSocket.hpp"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>

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
}

void GraphicsView::resetMode()
{
    mode_ = ViewInteractionMode::NoOp;
    if (dragMode() != RubberBandDrag)
        setDragMode(RubberBandDrag);
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
    emit scenePosChanged(static_cast<int>(sp.x()), static_cast<int>(sp.y()));

    if (mode_ == ViewInteractionMode::EdgeDrag && dragging_.isDragging())
        dragging_.updateDestination(sp.x(), sp.y());

    QGraphicsView::mouseMoveEvent(event);
}

void GraphicsView::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && mode_ == ViewInteractionMode::EdgeDrag) {
        // Match Python: only finalize edge after pointer moved past EDGE_DRAG_START_THRESHOLD.
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
