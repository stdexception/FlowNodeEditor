#include "GraphicsNode.hpp"
#include "EditorScene.hpp"
#include "GraphicsEdge.hpp"
#include "GraphicsScene.hpp"
#include "GraphicsSocket.hpp"

#include <QBrush>
#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QPen>
#include <QGraphicsTextItem>

GraphicsNode::GraphicsNode(EditorScene* scene, const QString& title)
    : QGraphicsRectItem(-100, -60, 200, 120)
    , scene_(scene)
    , title_(title)
{
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setBrush(QBrush(QColor(QStringLiteral("#2d3548"))));
    setPen(QPen(QColor(QStringLiteral("#4a5568")), 2));

    titleItem_ = new QGraphicsTextItem(this);
    titleItem_->setDefaultTextColor(QColor(QStringLiteral("#e2e8f0")));
    titleItem_->setFont(QFont(QStringLiteral("Segoe UI"), 10, QFont::DemiBold));
    setTitle(title_);

    addInputSocket();
    addOutputSocket();

    if (scene_ && scene_->graphicsScene())
        scene_->graphicsScene()->addItem(this);
    if (scene_)
        scene_->addNode(this);
}

void GraphicsNode::setTitle(const QString& t)
{
    title_ = t;
    if (titleItem_)
        titleItem_->setPlainText(title_);
    layoutTitle();
}

void GraphicsNode::layoutTitle()
{
    if (!titleItem_)
        return;
    const QRectF r = rect();
    const QRectF tr = titleItem_->boundingRect();
    titleItem_->setPos(r.center().x() - tr.width() / 2, r.top() + 8);
}

void GraphicsNode::addInputSocket()
{
    auto* s = new GraphicsSocket(this, GraphicsSocket::SocketType::Input, inputs_.size(), this);
    inputs_.append(s);
    s->updateSocketPosition();
}

void GraphicsNode::addOutputSocket()
{
    auto* s = new GraphicsSocket(this, GraphicsSocket::SocketType::Output, outputs_.size(), this);
    outputs_.append(s);
    s->updateSocketPosition();
}

QVariant GraphicsNode::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemPositionHasChanged || change == ItemScenePositionHasChanged) {
        for (auto* s : inputs_)
            s->updateSocketPosition();
        for (auto* s : outputs_)
            s->updateSocketPosition();
        if (scene()) {
            for (QGraphicsItem* it : scene()->items()) {
                if (auto* edge = dynamic_cast<GraphicsEdge*>(it))
                    edge->updatePath();
            }
        }
    }
    return QGraphicsRectItem::itemChange(change, value);
}
