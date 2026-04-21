#include "GraphicsEdge.hpp"
#include "EditorScene.hpp"
#include "GraphicsScene.hpp"
#include "GraphicsSocket.hpp"

#include <algorithm>
#include <cmath>

#include <QPainterPath>
#include <QPen>

GraphicsEdge::GraphicsEdge(EditorScene* scene, GraphicsSocket* start, GraphicsSocket* end)
    : scene_(scene)
    , start_(start)
    , end_(end)
{
    setZValue(-1);
    QPen p(QColor(QStringLiteral("#8b9bb4")), 2);
    p.setCosmetic(true);
    setPen(p);
    if (scene_ && scene_->graphicsScene()) {
        scene_->graphicsScene()->addItem(this);
        scene_->addEdge(this);
    }
    updatePath();
}

void GraphicsEdge::updatePath()
{
    if (!start_ || !end_)
        return;

    const QPointF p1 = start_->scenePos();
    const QPointF p2 = end_->scenePos();

    QPainterPath path(p1);
    const qreal dx = std::max(40.0, std::abs(p2.x() - p1.x()) * 0.5);
    QPointF c1(p1.x() + dx, p1.y());
    QPointF c2(p2.x() - dx, p2.y());
    path.cubicTo(c1, c2, p2);
    setPath(path);
}
