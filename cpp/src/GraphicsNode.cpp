#include "GraphicsNode.hpp"
#include "EditorScene.hpp"
#include "GraphicsEdge.hpp"
#include "GraphicsScene.hpp"
#include "GraphicsSocket.hpp"

#include <QBrush>
#include <QFont>
#include <QJsonArray>
#include <QJsonObject>
#include <QPen>
#include <QGraphicsTextItem>

GraphicsNode::GraphicsNode(EditorScene* scene, const QString& title, const QString& nodeType, bool addDefaultSockets)
    : QGraphicsRectItem(-100, -60, 200, 120)
    , scene_(scene)
    , title_(title)
    , nodeType_(nodeType)
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

    if (addDefaultSockets) {
        addInputSocket();
        addOutputSocket();
    }

    if (scene_ && scene_->graphicsScene())
        scene_->graphicsScene()->addItem(this);
    if (scene_)
        scene_->addNode(this);
}

GraphicsNode::~GraphicsNode()
{
    detachFromDocument();
}

void GraphicsNode::detachFromDocument()
{
    if (detached_)
        return;
    detached_ = true;

    const QVector<GraphicsSocket*> all = inputs_ + outputs_;
    QVector<GraphicsEdge*> toDelete;
    for (GraphicsSocket* s : all) {
        for (GraphicsEdge* e : s->edges()) {
            if (!toDelete.contains(e))
                toDelete.append(e);
        }
    }
    for (GraphicsEdge* e : toDelete) {
        e->removeFromDocument();
        delete e;
    }

    if (scene_ && scene_->graphicsScene())
        scene_->graphicsScene()->removeItem(this);
    if (scene_)
        scene_->removeNode(this);
    clearSockets();
    scene_ = nullptr;
}

void GraphicsNode::clearSockets()
{
    for (GraphicsSocket* s : inputs_)
        delete s;
    for (GraphicsSocket* s : outputs_)
        delete s;
    inputs_.clear();
    outputs_.clear();
}

void GraphicsNode::removeFromDocument()
{
    delete this;
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
    s->setMultiEdges(true);
    inputs_.append(s);
    s->updateSocketPosition();
}

void GraphicsNode::addOutputSocket()
{
    auto* s = new GraphicsSocket(this, GraphicsSocket::SocketType::Output, outputs_.size(), this);
    s->setMultiEdges(true);
    outputs_.append(s);
    s->updateSocketPosition();
}

QJsonObject GraphicsNode::toJson() const
{
    QJsonArray inArr;
    QJsonArray outArr;
    for (GraphicsSocket* s : inputs_)
        inArr.append(s->toJson());
    for (GraphicsSocket* s : outputs_)
        outArr.append(s->toJson());

    QJsonObject o;
    o.insert(QStringLiteral("id"), static_cast<qint64>(objectId_));
    o.insert(QStringLiteral("title"), title_);
    o.insert(QStringLiteral("node_type"), nodeType_);
    o.insert(QStringLiteral("pos_x"), scenePos().x());
    o.insert(QStringLiteral("pos_y"), scenePos().y());
    o.insert(QStringLiteral("inputs"), inArr);
    o.insert(QStringLiteral("outputs"), outArr);
    o.insert(QStringLiteral("content"), QJsonObject{});
    return o;
}

GraphicsNode* GraphicsNode::fromJson(EditorScene* scene, const QJsonObject& o, bool restoreId,
                                    QHash<quint64, GraphicsSocket*>& socketMap)
{
    const QString title = o.value(QStringLiteral("title")).toString(QStringLiteral("Node"));
    const QString ntype = o.value(QStringLiteral("node_type")).toString(QStringLiteral("Node"));
    auto* n = new GraphicsNode(scene, title, ntype, false);
    if (restoreId && o.contains(QStringLiteral("id")))
        n->assignObjectId(static_cast<quint64>(o.value(QStringLiteral("id")).toVariant().toULongLong()));
    else if (o.contains(QStringLiteral("id")))
        n->assignObjectId(static_cast<quint64>(o.value(QStringLiteral("id")).toVariant().toULongLong()));

    const QJsonArray inArr = o.value(QStringLiteral("inputs")).toArray();
    for (const QJsonValue& v : inArr) {
        GraphicsSocket* s = GraphicsSocket::fromJson(n, GraphicsSocket::SocketType::Input, v.toObject(), restoreId, socketMap);
        n->inputs_.append(s);
    }
    const QJsonArray outArr = o.value(QStringLiteral("outputs")).toArray();
    for (const QJsonValue& v : outArr) {
        GraphicsSocket* s = GraphicsSocket::fromJson(n, GraphicsSocket::SocketType::Output, v.toObject(), restoreId, socketMap);
        n->outputs_.append(s);
    }
    for (GraphicsSocket* s : n->inputs_)
        s->updateSocketPosition();
    for (GraphicsSocket* s : n->outputs_)
        s->updateSocketPosition();

    n->setPos(o.value(QStringLiteral("pos_x")).toDouble(), o.value(QStringLiteral("pos_y")).toDouble());
    n->setTitle(o.value(QStringLiteral("title")).toString(title));
    return n;
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
