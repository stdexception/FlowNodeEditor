#include <algorithm>

#include "EditorScene.hpp"
#include "GraphicsScene.hpp"
#include "GraphicsNode.hpp"
#include "GraphicsEdge.hpp"
#include "GraphicsSocket.hpp"
#include "GraphicsView.hpp"
#include "SceneHistory.hpp"
#include "SceneClipboard.hpp"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHash>

EditorScene::EditorScene()
{
    grScene_ = std::make_unique<GraphicsScene>(this);
    grScene_->setGridSceneSize(sceneWidth_, sceneHeight_);
    history_ = std::make_unique<SceneHistory>(this);
    clipboard_ = std::make_unique<SceneClipboard>(this);
}

EditorScene::~EditorScene()
{
    const std::vector<GraphicsEdge*> edgesCopy = edges_;
    for (GraphicsEdge* e : edgesCopy)
        delete e;
    edges_.clear();
    const std::vector<GraphicsNode*> nodesCopy = nodes_;
    for (GraphicsNode* n : nodesCopy)
        delete n;
    nodes_.clear();
}

void EditorScene::clearContent()
{
    const std::vector<GraphicsEdge*> ec = edges_;
    for (GraphicsEdge* e : ec)
        delete e;
    edges_.clear();
    const std::vector<GraphicsNode*> nc = nodes_;
    for (GraphicsNode* n : nc)
        delete n;
    nodes_.clear();
}

QJsonObject EditorScene::toJson() const
{
    QJsonArray nodesArr;
    QJsonArray edgesArr;
    for (GraphicsNode* n : nodes_)
        nodesArr.append(n->toJson());
    for (GraphicsEdge* e : edges_)
        edgesArr.append(e->toJson());

    QJsonObject o;
    o.insert(QStringLiteral("id"), static_cast<qint64>(sceneObjectId_));
    o.insert(QStringLiteral("scene_width"), sceneWidth_);
    o.insert(QStringLiteral("scene_height"), sceneHeight_);
    o.insert(QStringLiteral("nodes"), nodesArr);
    o.insert(QStringLiteral("edges"), edgesArr);
    return o;
}

void EditorScene::fromJson(const QJsonObject& data)
{
    inLoadBatch_ = true;
    clearContent();

    if (data.contains(QStringLiteral("id")))
        sceneObjectId_ = static_cast<quint64>(data.value(QStringLiteral("id")).toVariant().toULongLong());
    sceneWidth_ = data.value(QStringLiteral("scene_width")).toInt(64000);
    sceneHeight_ = data.value(QStringLiteral("scene_height")).toInt(64000);
    if (grScene_)
        grScene_->setGridSceneSize(sceneWidth_, sceneHeight_);

    QHash<quint64, GraphicsSocket*> socketMap;
    const QJsonArray nodes = data.value(QStringLiteral("nodes")).toArray();
    for (const QJsonValue& v : nodes)
        GraphicsNode::fromJson(this, v.toObject(), true, socketMap);

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
        auto* edge = new GraphicsEdge(this, outS, inS);
        if (eo.contains(QStringLiteral("edge_type")))
            edge->setEdgeType(eo.value(QStringLiteral("edge_type")).toInt(GraphicsEdge::kEdgeTypeBezier));
        if (eo.contains(QStringLiteral("id")))
            edge->assignObjectId(static_cast<quint64>(eo.value(QStringLiteral("id")).toVariant().toULongLong()));
    }

    inLoadBatch_ = false;
}

bool EditorScene::saveToFile(const QString& path) const
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    f.write(QJsonDocument(toJson()).toJson(QJsonDocument::Indented));
    return true;
}

bool EditorScene::loadFromFile(const QString& path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return false;
    fromJson(doc.object());
    setModified(false);
    return true;
}

bool EditorScene::saveGraphJson(const QString& path) const
{
    QJsonObject nodesDict;
    for (GraphicsNode* n : nodes_) {
        QJsonObject nodeObj;
        nodeObj.insert(QStringLiteral("type"), n->nodeType());
        nodeObj.insert(QStringLiteral("settings"), QJsonObject{});
        nodesDict.insert(n->title(), nodeObj);
    }

    QJsonArray connections;
    for (GraphicsEdge* e : edges_) {
        if (!e->startSocket() || !e->endSocket())
            continue;
        GraphicsSocket* outS = e->startSocket()->isOutput() ? e->startSocket() : e->endSocket();
        GraphicsSocket* inS = e->startSocket()->isInput() ? e->startSocket() : e->endSocket();
        if (!outS->isOutput() || !inS->isInput())
            continue;
        const QString start = QStringLiteral("%1:%2")
                                  .arg(outS->node()->title())
                                  .arg(outS->index());
        const QString end = QStringLiteral("%1:%2")
                                .arg(inS->node()->title())
                                .arg(inS->index());
        QJsonArray pair;
        pair.append(start);
        pair.append(end);
        connections.append(pair);
    }

    QFileInfo fi(path);
    QJsonObject graph;
    graph.insert(QStringLiteral("name"), fi.completeBaseName());
    graph.insert(QStringLiteral("nodes"), nodesDict);
    graph.insert(QStringLiteral("connections"), connections);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    f.write(QJsonDocument(graph).toJson(QJsonDocument::Indented));
    return true;
}

void EditorScene::addNode(GraphicsNode* node)
{
    nodes_.push_back(node);
    if (!inLoadBatch_)
        setModified(true);
}

void EditorScene::removeNode(GraphicsNode* node)
{
    auto it = std::find(nodes_.begin(), nodes_.end(), node);
    if (it != nodes_.end())
        nodes_.erase(it);
    if (!inLoadBatch_)
        setModified(true);
}

void EditorScene::addEdge(GraphicsEdge* edge)
{
    edges_.push_back(edge);
    if (!inLoadBatch_)
        setModified(true);
}

void EditorScene::removeEdge(GraphicsEdge* edge)
{
    auto it = std::find(edges_.begin(), edges_.end(), edge);
    if (it != edges_.end())
        edges_.erase(it);
    if (!inLoadBatch_)
        setModified(true);
}

void EditorScene::setModified(bool value)
{
    modified_ = value;
}
