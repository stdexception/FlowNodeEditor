#include "GraphicsSocket.hpp"
#include "GraphicsNode.hpp"
#include "GraphicsEdge.hpp"

#include <QBrush>
#include <QPen>

GraphicsSocket::GraphicsSocket(GraphicsNode* node, SocketType type, int index, QGraphicsItem* parent)
    : QGraphicsEllipseItem(-kRadius, -kRadius, 2 * kRadius, 2 * kRadius, parent)
    , node_(node)
    , type_(type)
    , index_(index)
{
    setPen(QPen(QColor(QStringLiteral("#5a6578")), 1));
    setBrush(QBrush(type == SocketType::Input ? QColor(QStringLiteral("#44aa44"))
                                                : QColor(QStringLiteral("#aa4444"))));
    setZValue(1);
}

void GraphicsSocket::updateSocketPosition()
{
    if (!node_)
        return;
    const QRectF br = node_->boundingRect();
    const qreal y = br.top() + 40 + index_ * 28;
    if (type_ == SocketType::Input)
        setPos(br.left(), y);
    else
        setPos(br.right(), y);
}

void GraphicsSocket::addEdge(GraphicsEdge* e)
{
    if (!e || edges_.contains(e))
        return;
    edges_.push_back(e);
}

void GraphicsSocket::removeEdge(GraphicsEdge* e)
{
    edges_.removeAll(e);
}

void GraphicsSocket::removeAllEdges()
{
    const QVector<GraphicsEdge*> copy = edges_;
    for (GraphicsEdge* e : copy) {
        e->removeFromDocument();
        delete e;
    }
    edges_.clear();
}
