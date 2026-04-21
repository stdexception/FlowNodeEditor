#include "FlowEditorScene.h"

#include "FlowEdgeItem.h"
#include "FlowNodeItem.h"
#include "FlowSocketItem.h"
#include "NodeTypeRegistry.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <algorithm>

namespace
{
constexpr qreal kSceneW = 64000.0;
constexpr qreal kSceneH = 64000.0;
constexpr int kLeftCenter = 2;
constexpr int kRightCenter = 5;

struct SocketSortKey
{
    QJsonObject o;
    double key() const
    {
        return double(o.value(QStringLiteral("index")).toInt())
            + double(o.value(QStringLiteral("position")).toInt()) * 10000.0;
    }
};

bool socketLess(const QJsonObject &a, const QJsonObject &b)
{
    SocketSortKey ka;
    ka.o = a;
    SocketSortKey kb;
    kb.o = b;
    return ka.key() < kb.key();
}
} // namespace

FlowEditorScene::FlowEditorScene(QObject *parent)
    : QGraphicsScene(parent)
{
    setSceneRect(-kSceneW / 2, -kSceneH / 2, kSceneW, kSceneH);
}

qint64 FlowEditorScene::takeNextId()
{
    return ++m_nextEntityId;
}

void FlowEditorScene::seedNextId(qint64 minValue)
{
    if (minValue >= m_nextEntityId)
    {
        m_nextEntityId = minValue;
    }
}

int FlowEditorScene::registerNodeTypeOccurrence(const QString &nodeType)
{
    return ++m_typeOccurrence[nodeType];
}

void FlowEditorScene::setModified(bool m)
{
    m_modified = m;
    if (m_loadSilent > 0)
    {
        return;
    }
    emit modificationChanged(m);
}

FlowNodeItem *FlowEditorScene::spawnNode(const QString &nodeType, const QPointF &scenePos,
                                         const QString &titleOverride, const qint64 forcedNodeId,
                                         const QVector<qint64> &presetInputSocketIds,
                                         const QVector<qint64> &presetOutputSocketIds)
{
    if (!m_registry)
    {
        return nullptr;
    }
    const NodeTypeInfo *info = m_registry->findType(nodeType);
    if (!info)
    {
        return nullptr;
    }

    const qint64 nid = forcedNodeId >= 0 ? forcedNodeId : takeNextId();
    const bool registerOcc = titleOverride.isEmpty();
    auto *node = new FlowNodeItem(this, *info, nid, titleOverride, registerOcc, presetInputSocketIds,
                                  presetOutputSocketIds);
    addItem(node);
    node->setPos(scenePos);
    setModified(true);
    return node;
}

void FlowEditorScene::removeEdge(FlowEdgeItem *edge)
{
    if (!edge)
    {
        return;
    }
    if (edge->startSocket())
    {
        edge->startSocket()->edges.removeAll(edge);
    }
    if (edge->endSocket())
    {
        edge->endSocket()->edges.removeAll(edge);
    }
    removeItem(edge);
    delete edge;
    setModified(true);
}

void FlowEditorScene::removeNode(FlowNodeItem *node)
{
    if (!node)
    {
        return;
    }

    const QVector<FlowSocketItem *> all = node->inputs() + node->outputs();
    for (FlowSocketItem *s : all)
    {
        const QVector<FlowEdgeItem *> copy = s->edges;
        for (FlowEdgeItem *e : copy)
        {
            removeEdge(e);
        }
    }

    removeItem(node);
    delete node;
    setModified(true);
}

FlowEdgeItem *FlowEditorScene::makeEdge(FlowSocketItem *a, FlowSocketItem *b, qint64 edgeId, int edgeType)
{
    FlowSocketItem *start = a;
    FlowSocketItem *end = b;
    if (!FlowEdgeItem::validateConnection(start, end))
    {
        if (!FlowEdgeItem::validateConnection(end, start))
        {
            return nullptr;
        }
        qSwap(start, end);
    }

    auto clearIfNeeded = [this](FlowSocketItem *sock) {
        if (!sock->multiEdges)
        {
            const QVector<FlowEdgeItem *> copy = sock->edges;
            for (FlowEdgeItem *e : copy)
            {
                removeEdge(e);
            }
        }
    };

    clearIfNeeded(start);
    clearIfNeeded(end);

    const qint64 eid = edgeId < 0 ? takeNextId() : edgeId;
    auto *edge = new FlowEdgeItem(start, end, eid, edgeType);
    addItem(edge);
    edge->updatePath();
    setModified(true);
    return edge;
}

void FlowEditorScene::refreshEdgesForNode(FlowNodeItem *node)
{
    if (!node)
    {
        return;
    }
    const QVector<FlowSocketItem *> all = node->inputs() + node->outputs();
    for (FlowSocketItem *s : all)
    {
        for (FlowEdgeItem *e : s->edges)
        {
            if (e)
            {
                e->updatePath();
            }
        }
    }
}

void FlowEditorScene::clearDocument()
{
    const QList<QGraphicsItem *> itemsCopy = items();
    for (QGraphicsItem *it : itemsCopy)
    {
        if (it->type() == FlowNodeItem::Type)
        {
            removeNode(static_cast<FlowNodeItem *>(it));
        }
    }
    m_typeOccurrence.clear();
    m_filePath.clear();
    m_nextEntityId = 1;
    m_sceneId = 1;
    setModified(false);
}

QJsonObject FlowEditorScene::serializeScene() const
{
    QJsonObject root;
    root.insert(QStringLiteral("id"), double(m_sceneId));
    root.insert(QStringLiteral("scene_width"), kSceneW);
    root.insert(QStringLiteral("scene_height"), kSceneH);

    QJsonArray nodesArr;
    const QList<QGraphicsItem *> all = items();
    for (QGraphicsItem *gi : all)
    {
        if (gi->type() != FlowNodeItem::Type)
        {
            continue;
        }
        auto *node = static_cast<FlowNodeItem *>(gi);

        QJsonArray inA;
        for (FlowSocketItem *s : node->inputs())
        {
            QJsonObject so;
            so.insert(QStringLiteral("id"), double(s->socketId()));
            so.insert(QStringLiteral("index"), s->socketIndex());
            so.insert(QStringLiteral("multi_edges"), s->multiEdges);
            so.insert(QStringLiteral("position"), kLeftCenter);
            so.insert(QStringLiteral("socket_type"), s->dataType());
            inA.append(so);
        }

        QJsonArray outA;
        for (FlowSocketItem *s : node->outputs())
        {
            QJsonObject so;
            so.insert(QStringLiteral("id"), double(s->socketId()));
            so.insert(QStringLiteral("index"), s->socketIndex());
            so.insert(QStringLiteral("multi_edges"), s->multiEdges);
            so.insert(QStringLiteral("position"), kRightCenter);
            so.insert(QStringLiteral("socket_type"), s->dataType());
            outA.append(so);
        }

        QJsonObject n;
        n.insert(QStringLiteral("id"), double(node->nodeId()));
        n.insert(QStringLiteral("title"), node->title());
        n.insert(QStringLiteral("pos_x"), node->pos().x());
        n.insert(QStringLiteral("pos_y"), node->pos().y());
        n.insert(QStringLiteral("inputs"), inA);
        n.insert(QStringLiteral("outputs"), outA);
        n.insert(QStringLiteral("content"), QJsonObject());
        n.insert(QStringLiteral("node_type"), node->nodeType());
        n.insert(QStringLiteral("node_settings"), node->settings());
        nodesArr.append(n);
    }

    QJsonArray edgesArr;
    for (QGraphicsItem *gi : all)
    {
        if (gi->type() != FlowEdgeItem::Type)
        {
            continue;
        }
        auto *edge = static_cast<FlowEdgeItem *>(gi);
        QJsonObject eo;
        eo.insert(QStringLiteral("id"), double(edge->edgeId()));
        eo.insert(QStringLiteral("edge_type"), edge->edgeType());
        eo.insert(QStringLiteral("start"), edge->startSocket() ? double(edge->startSocket()->socketId()) : QJsonValue());
        eo.insert(QStringLiteral("end"), edge->endSocket() ? double(edge->endSocket()->socketId()) : QJsonValue());
        edgesArr.append(eo);
    }

    root.insert(QStringLiteral("nodes"), nodesArr);
    root.insert(QStringLiteral("edges"), edgesArr);
    return root;
}

bool FlowEditorScene::saveSceneFile(const QString &path) const
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    const QJsonDocument doc(serializeScene());
    f.write(doc.toJson(QJsonDocument::Indented));
    f.close();
    const_cast<FlowEditorScene *>(this)->setCurrentFilePath(path);
    const_cast<FlowEditorScene *>(this)->setModified(false);
    return true;
}

bool FlowEditorScene::saveGraphFile(const QString &path) const
{
    QJsonObject graph;
    graph.insert(QStringLiteral("name"), QFileInfo(path).completeBaseName());

    QJsonObject nodesObj;
    const QList<QGraphicsItem *> all = items();
    for (QGraphicsItem *gi : all)
    {
        if (gi->type() != FlowNodeItem::Type)
        {
            continue;
        }
        auto *node = static_cast<FlowNodeItem *>(gi);
        QJsonObject n;
        n.insert(QStringLiteral("type"), node->nodeType());
        n.insert(QStringLiteral("settings"), node->settings());
        nodesObj.insert(node->title(), n);
    }

    QJsonArray connections;
    for (QGraphicsItem *gi : all)
    {
        if (gi->type() != FlowEdgeItem::Type)
        {
            continue;
        }
        auto *edge = static_cast<FlowEdgeItem *>(gi);
        if (!edge->startSocket() || !edge->endSocket())
        {
            continue;
        }
        const QString a = edge->startSocket()->flowNode()->title() + QLatin1Char(':')
            + edge->startSocket()->socketName();
        const QString b = edge->endSocket()->flowNode()->title() + QLatin1Char(':')
            + edge->endSocket()->socketName();
        QJsonArray pair;
        pair.append(a);
        pair.append(b);
        connections.append(pair);
    }

    graph.insert(QStringLiteral("nodes"), nodesObj);
    graph.insert(QStringLiteral("connections"), connections);

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    f.write(QJsonDocument(graph).toJson(QJsonDocument::Indented));
    f.close();
    return true;
}

bool FlowEditorScene::deserializeScene(const QJsonObject &o)
{
    clearDocument();

    ++m_loadSilent;

    m_sceneId = qint64(o.value(QStringLiteral("id")).toDouble());

    QHash<qint64, FlowSocketItem *> socketById;

    QJsonArray nodes = o.value(QStringLiteral("nodes")).toArray();
    QVector<QJsonObject> nodeObjs;
    nodeObjs.reserve(nodes.size());
    for (const QJsonValue &v : nodes)
    {
        nodeObjs.append(v.toObject());
    }

    for (const QJsonObject &nd : nodeObjs)
    {
        const QString nodeType = nd.value(QStringLiteral("node_type")).toString();
        const QString title = nd.value(QStringLiteral("title")).toString();
        const qreal px = nd.value(QStringLiteral("pos_x")).toDouble();
        const qreal py = nd.value(QStringLiteral("pos_y")).toDouble();
        const qint64 nodeId = qint64(nd.value(QStringLiteral("id")).toDouble());

        QJsonArray inArr = nd.value(QStringLiteral("inputs")).toArray();
        QJsonArray outArr = nd.value(QStringLiteral("outputs")).toArray();
        QVector<QJsonObject> inObjs;
        QVector<QJsonObject> outObjs;
        for (const QJsonValue &v : inArr)
        {
            inObjs.append(v.toObject());
        }
        for (const QJsonValue &v : outArr)
        {
            outObjs.append(v.toObject());
        }
        std::sort(inObjs.begin(), inObjs.end(), socketLess);
        std::sort(outObjs.begin(), outObjs.end(), socketLess);

        QVector<qint64> inIds;
        QVector<qint64> outIds;
        for (const QJsonObject &jo : inObjs)
        {
            inIds.append(qint64(jo.value(QStringLiteral("id")).toDouble()));
        }
        for (const QJsonObject &jo : outObjs)
        {
            outIds.append(qint64(jo.value(QStringLiteral("id")).toDouble()));
        }

        FlowNodeItem *node = spawnNode(nodeType, QPointF(px, py), title, nodeId, inIds, outIds);
        if (!node)
        {
            continue;
        }

        if (nd.contains(QStringLiteral("node_settings")))
        {
            node->setSettings(nd.value(QStringLiteral("node_settings")).toObject());
        }

        for (int i = 0; i < inObjs.size() && i < node->inputs().size(); ++i)
        {
            FlowSocketItem *s = node->inputs().at(i);
            s->multiEdges = inObjs.at(i).value(QStringLiteral("multi_edges")).toBool();
            socketById.insert(s->socketId(), s);
        }
        for (int i = 0; i < outObjs.size() && i < node->outputs().size(); ++i)
        {
            FlowSocketItem *s = node->outputs().at(i);
            s->multiEdges = outObjs.at(i).value(QStringLiteral("multi_edges")).toBool();
            socketById.insert(s->socketId(), s);
        }
    }

    QJsonArray edges = o.value(QStringLiteral("edges")).toArray();
    for (const QJsonValue &v : edges)
    {
        const QJsonObject ed = v.toObject();
        const qint64 eid = qint64(ed.value(QStringLiteral("id")).toDouble());
        const int et = ed.value(QStringLiteral("edge_type")).toInt(2);
        const qint64 sidStart = qint64(ed.value(QStringLiteral("start")).toDouble());
        const qint64 sidEnd = qint64(ed.value(QStringLiteral("end")).toDouble());
        FlowSocketItem *sa = socketById.value(sidStart, nullptr);
        FlowSocketItem *sb = socketById.value(sidEnd, nullptr);
        if (!sa || !sb)
        {
            continue;
        }
        makeEdge(sa, sb, eid, et);
    }

    resumeIdCounterFromScene();

    --m_loadSilent;
    setModified(false);
    return true;
}

void FlowEditorScene::resumeIdCounterFromScene()
{
    qint64 m = m_sceneId;
    const QList<QGraphicsItem *> all = items();
    for (QGraphicsItem *gi : all)
    {
        if (gi->type() == FlowNodeItem::Type)
        {
            auto *n = static_cast<FlowNodeItem *>(gi);
            m = qMax(m, n->nodeId());
            for (FlowSocketItem *s : n->inputs() + n->outputs())
            {
                m = qMax(m, s->socketId());
            }
        }
        else if (gi->type() == FlowEdgeItem::Type)
        {
            m = qMax(m, static_cast<FlowEdgeItem *>(gi)->edgeId());
        }
    }
    m_nextEntityId = m;
}

bool FlowEditorScene::loadSceneFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
    {
        return false;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject())
    {
        return false;
    }
    const bool ok = deserializeScene(doc.object());
    if (ok)
    {
        setCurrentFilePath(path);
    }
    return ok;
}
