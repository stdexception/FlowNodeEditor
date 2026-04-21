#include "GraphicsView.hpp"
#include "GraphicsScene.hpp"

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

void GraphicsView::mouseMoveEvent(QMouseEvent* event)
{
    const QPointF sp = mapToScene(event->pos());
    emit scenePosChanged(static_cast<int>(sp.x()), static_cast<int>(sp.y()));
    QGraphicsView::mouseMoveEvent(event);
}
