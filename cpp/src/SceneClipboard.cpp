#include "SceneClipboard.hpp"
#include "EditorScene.hpp"
#include "GraphicsScene.hpp"
#include "GraphicsEdge.hpp"
#include "GraphicsNode.hpp"
#include "GraphicsSocket.hpp"
#include "GraphicsView.hpp"
#include "SceneHistory.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>

SceneClipboard::SceneClipboard(EditorScene* scene)
    : scene_(scene)
{
}

QJsonObject SceneClipboard::serializeSelected(bool deleteAfter)
{
    QJsonObject out;
    QJsonArray nodesArr;
    QJsonArray edgesArr;

    if (!scene_ || !scene_->graphicsScene())
        return out;

    QSet<GraphicsNode*> selNodes;
    for (QGraphicsItem* it : scene_->graphicsScene()->selectedItems()) {
        if (auto* n = dynamic_cast<GraphicsNode*>(it))
            selNodes.insert(n);
    }

    QSet<quint64> socketIds;
    for (GraphicsNode* n : selNodes) {
        nodesArr.append(n->toJson());
        for (GraphicsSocket* s : n->inputSockets())
            socketIds.insert(s->objectId());
        for (GraphicsSocket* s : n->outputSockets())
            socketIds.insert(s->objectId());
    }

    for (GraphicsEdge* e : scene_->edges()) {
        if (!e->startSocket() || !e->endSocket())
            continue;
        const quint64 a = e->startSocket()->objectId();
        const quint64 b = e->endSocket()->objectId();
        if (socketIds.contains(a) && socketIds.contains(b))
            edgesArr.append(e->toJson());
    }

    out.insert(QStringLiteral("nodes"), nodesArr);
    out.insert(QStringLiteral("edges"), edgesArr);

    if (deleteAfter && scene_->view()) {
        scene_->view()->deleteSelectedItems();
        if (scene_->history())
            scene_->history()->storeHistory(QStringLiteral("Cut"), true);
    }

    return out;
}

void SceneClipboard::deserializeFromClipboard(const QJsonObject& data)
{
    if (!scene_)
        return;

    QHash<quint64, GraphicsSocket*> socketMap;
    const QJsonArray nodes = data.value(QStringLiteral("nodes")).toArray();

    QPointF mousePos(0, 0);
    if (scene_->view())
        mousePos = scene_->view()->lastSceneMousePosition();

    qreal minx = 1e9, maxx = -1e9, miny = 1e9, maxy = -1e9;
    for (const QJsonValue& v : nodes) {
        const QJsonObject o = v.toObject();
        const qreal x = o.value(QStringLiteral("pos_x")).toDouble();
        const qreal y = o.value(QStringLiteral("pos_y")).toDouble();
        minx = std::min(minx, x);
        maxx = std::max(maxx, x);
        miny = std::min(miny, y);
        maxy = std::max(maxy, y);
    }
    if (nodes.isEmpty())
        return;

    if (scene_->graphicsScene())
        scene_->graphicsScene()->clearSelection();

    for (const QJsonValue& v : nodes) {
        GraphicsNode* nn = GraphicsNode::fromJson(scene_, v.toObject(), false, socketMap);
        const QJsonObject o = v.toObject();
        const qreal px = o.value(QStringLiteral("pos_x")).toDouble();
        const qreal py = o.value(QStringLiteral("pos_y")).toDouble();
        const qreal newx = mousePos.x() + px - minx;
        const qreal newy = mousePos.y() + py - miny;
        nn->setPos(newx, newy);
        nn->setSelected(true);
    }

    const QJsonArray edges = data.value(QStringLiteral("edges")).toArray();
    for (const QJsonValue& v : edges) {
        const QJsonObject eo = v.toObject();
        const quint64 sid = static_cast<quint64>(eo.value(QStringLiteral("start")).toVariant().toULongLong());
        const quint64 eid = static_cast<quint64>(eo.value(QStringLiteral("end")).toVariant().toULongLong());
        GraphicsSocket* a = socketMap.value(sid, nullptr);
        GraphicsSocket* b = socketMap.value(eid, nullptr);
        if (!a || !b)
            continue;
        GraphicsSocket* outS = a->isOutput() ? a : (b->isOutput() ? b : nullptr);
        GraphicsSocket* inS = a->isInput() ? a : (b->isInput() ? b : nullptr);
        if (!outS || !inS)
            continue;
        new GraphicsEdge(scene_, outS, inS);
    }

    if (scene_->history())
        scene_->history()->storeHistory(QStringLiteral("Paste"), true);
}
