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
    attachSockets();
    updatePath();
}

GraphicsEdge::~GraphicsEdge()
{
    detachSockets();
    if (QGraphicsScene* gs = scene())
        gs->removeItem(this);
    if (scene_) {
        scene_->removeEdge(this);
        scene_ = nullptr;
    }
    start_ = nullptr;
    end_ = nullptr;
    freeEndScene_.reset();
}

void GraphicsEdge::setEndSocket(GraphicsSocket* end)
{
    if (end_ == end)
        return;
    if (end_)
        end_->removeEdge(this);
    end_ = end;
    clearFreeEnd();
    if (end_)
        end_->addEdge(this);
    updatePath();
}

void GraphicsEdge::setFreeEndScene(const QPointF& scenePos)
{
    freeEndScene_ = scenePos;
    updatePath();
}

void GraphicsEdge::clearFreeEnd()
{
    freeEndScene_.reset();
    updatePath();
}

void GraphicsEdge::attachSockets()
{
    if (start_)
        start_->addEdge(this);
    if (end_)
        end_->addEdge(this);
}

void GraphicsEdge::detachSockets()
{
    if (start_)
        start_->removeEdge(this);
    if (end_)
        end_->removeEdge(this);
}

void GraphicsEdge::removeFromDocument()
{
    detachSockets();
    if (QGraphicsScene* gs = scene())
        gs->removeItem(this);
    if (scene_)
        scene_->removeEdge(this);
    scene_ = nullptr;
    start_ = nullptr;
    end_ = nullptr;
    freeEndScene_.reset();
}

void GraphicsEdge::updatePath()
{
    if (!start_)
        return;

    const QPointF p1 = start_->scenePos();
    QPointF p2;
    if (end_)
        p2 = end_->scenePos();
    else if (freeEndScene_)
        p2 = *freeEndScene_;
    else
        p2 = p1;

    QPainterPath path(p1);
    const qreal dx = std::max(40.0, std::abs(p2.x() - p1.x()) * 0.5);
    QPointF c1(p1.x() + dx, p1.y());
    QPointF c2(p2.x() - dx, p2.y());
    path.cubicTo(c1, c2, p2);
    setPath(path);
}
