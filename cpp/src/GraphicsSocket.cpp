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
    , position_(type == SocketType::Input ? kSocketPositionLeftCenter : kSocketPositionRightCenter)
{
    setPen(QPen(QColor(QStringLiteral("#5a6578")), 1));
    setBrush(QBrush(type == SocketType::Input ? QColor(QStringLiteral("#44aa44"))
                                                : QColor(QStringLiteral("#aa4444"))));
    setZValue(1);
}

QJsonObject GraphicsSocket::toJson() const
{
    QJsonObject o;
    o.insert(QStringLiteral("id"), static_cast<qint64>(objectId_));
    o.insert(QStringLiteral("index"), index_);
    o.insert(QStringLiteral("multi_edges"), multiEdges_);
    o.insert(QStringLiteral("position"), position_);
    o.insert(QStringLiteral("socket_type"), dataType_);
    return o;
}

GraphicsSocket* GraphicsSocket::fromJson(GraphicsNode* node, SocketType type, const QJsonObject& o, bool restoreId,
                                        QHash<quint64, GraphicsSocket*>& socketMap)
{
    const int idx = o.value(QStringLiteral("index")).toInt();
    auto* s = new GraphicsSocket(node, type, idx, node);
    s->position_ = o.value(QStringLiteral("position")).toInt(
        type == SocketType::Input ? kSocketPositionLeftCenter : kSocketPositionRightCenter);
    s->multiEdges_ = o.value(QStringLiteral("multi_edges")).toBool(false);
    s->dataType_ = o.value(QStringLiteral("socket_type")).toString(QStringLiteral("any"));

    quint64 fileSocketId = 0;
    if (o.contains(QStringLiteral("id")))
        fileSocketId = static_cast<quint64>(o.value(QStringLiteral("id")).toVariant().toULongLong());
    if (restoreId && fileSocketId != 0)
        s->assignObjectId(fileSocketId);
    if (fileSocketId != 0)
        socketMap.insert(fileSocketId, s);
    else
        socketMap.insert(s->objectId(), s);
    return s;
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
