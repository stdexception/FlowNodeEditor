#include "FlowEditorScene.h"

#include "FlowEdgeItem.h"
#include "FlowEditorSceneHistory.h"
#include "FlowNodeItem.h"
#include "FlowSocketItem.h"
#include "NodeTypeRegistry.h"

#include <QFile>
#include <QFileInfo>
#include <QBrush>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>
#include <QPainterPathStroker>
#include <QTemporaryFile>
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

QJsonObject nodeToJson(const FlowNodeItem *node)
{
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
    return n;
}
} // namespace

FlowEditorScene::FlowEditorScene(QObject *parent)
    : QGraphicsScene(parent)
    , m_history(new FlowEditorSceneHistory(this, this))
{
    setSceneRect(-kSceneW / 2, -kSceneH / 2, kSceneW, kSceneH);
    setBackgroundBrush(QBrush(QColor(QStringLiteral("#0f131c"))));
    connect(this, &QGraphicsScene::selectionChanged, this, [this]() {
        emit selectionChangedInScene();
    });
}

FlowEditorScene::~FlowEditorScene() = default;

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
    if (m_loadSilent == 0)
    {
        setModified(true);
    }
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
    if (m_loadSilent == 0)
    {
        setModified(true);
    }
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
    if (m_loadSilent == 0)
    {
        setModified(true);
    }
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

    // Avoid creating duplicate edges between the exact same two sockets when multi-edges are allowed.
    if (start->multiEdges && end->multiEdges)
    {
        for (FlowEdgeItem *ex : start->edges)
        {
            if (!ex)
            {
                continue;
            }
            if ((ex->startSocket() == start && ex->endSocket() == end)
                || (ex->startSocket() == end && ex->endSocket() == start))
            {
                return ex;
            }
        }
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
    if (m_loadSilent == 0)
    {
        setModified(true);
    }
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

void FlowEditorScene::removeAllGraphItems()
{
    const QList<QGraphicsItem *> copy = items();
    for (QGraphicsItem *gi : copy)
    {
        if (gi->type() == FlowNodeItem::Type)
        {
            removeNode(static_cast<FlowNodeItem *>(gi));
        }
    }
    m_typeOccurrence.clear();
}

void FlowEditorScene::clearDocument()
{
    removeAllGraphItems();
    m_filePath.clear();
    m_nextEntityId = 1;
    m_sceneId = 1;
    m_history->clear();
    setModified(false);
}

QJsonObject FlowEditorScene::snapshotJson() const
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
        nodesArr.append(nodeToJson(static_cast<FlowNodeItem *>(gi)));
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

bool FlowEditorScene::importGraphJson(const QJsonObject &o)
{
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
            const QJsonObject jo = inObjs.at(i);
            s->multiEdges = jo.contains(QStringLiteral("multi_edges")) ? jo.value(QStringLiteral("multi_edges")).toBool()
                                                                       : true;
            socketById.insert(s->socketId(), s);
        }
        for (int i = 0; i < outObjs.size() && i < node->outputs().size(); ++i)
        {
            FlowSocketItem *s = node->outputs().at(i);
            const QJsonObject jo = outObjs.at(i);
            s->multiEdges = jo.contains(QStringLiteral("multi_edges")) ? jo.value(QStringLiteral("multi_edges")).toBool()
                                                                       : true;
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
    return true;
}

void FlowEditorScene::restoreFromSnapshotJson(const QJsonObject &sceneJson, const QJsonObject &selectionJson)
{
    ++m_loadSilent;
    removeAllGraphItems();
    importGraphJson(sceneJson);

    deselectAll();
    const QJsonArray nids = selectionJson.value(QStringLiteral("nodes")).toArray();
    for (const QJsonValue &v : nids)
    {
        FlowNodeItem *n = findNodeById(qint64(v.toDouble()));
        if (n)
        {
            n->setSelected(true);
        }
    }
    const QJsonArray eids = selectionJson.value(QStringLiteral("edges")).toArray();
    for (const QJsonValue &v : eids)
    {
        FlowEdgeItem *e = findEdgeById(qint64(v.toDouble()));
        if (e)
        {
            e->setSelected(true);
        }
    }

    --m_loadSilent;
    emit selectionChangedInScene();
}

FlowNodeItem *FlowEditorScene::findNodeById(qint64 nodeId) const
{
    const QList<QGraphicsItem *> all = items();
    for (QGraphicsItem *gi : all)
    {
        if (gi->type() == FlowNodeItem::Type)
        {
            auto *n = static_cast<FlowNodeItem *>(gi);
            if (n->nodeId() == nodeId)
            {
                return n;
            }
        }
    }
    return nullptr;
}

FlowEdgeItem *FlowEditorScene::findEdgeById(qint64 edgeId) const
{
    const QList<QGraphicsItem *> all = items();
    for (QGraphicsItem *gi : all)
    {
        if (gi->type() == FlowEdgeItem::Type)
        {
            auto *e = static_cast<FlowEdgeItem *>(gi);
            if (e->edgeId() == edgeId)
            {
                return e;
            }
        }
    }
    return nullptr;
}

void FlowEditorScene::deselectAll()
{
    for (QGraphicsItem *gi : selectedItems())
    {
        gi->setSelected(false);
    }
}

bool FlowEditorScene::deserializeScene(const QJsonObject &o)
{
    clearDocument();

    ++m_loadSilent;
    importGraphJson(o);
    --m_loadSilent;
    setModified(false);
    m_history->storeInitialHistoryStamp();
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

bool FlowEditorScene::saveSceneFile(const QString &path) const
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        return false;
    }
    const QJsonDocument doc(const_cast<FlowEditorScene *>(this)->snapshotJson());
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

bool FlowEditorScene::saveGraphToTempFile(QString *outPath) const
{
    QTemporaryFile tf(QDir::tempPath() + QStringLiteral("/flownode_graph_XXXXXX.graph.json"));
    tf.setAutoRemove(false);
    if (!tf.open())
    {
        return false;
    }
    const QString path = tf.fileName();
    tf.close();
    if (!saveGraphFile(path))
    {
        QFile::remove(path);
        return false;
    }
    if (outPath)
    {
        *outPath = path;
    }
    return true;
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

QJsonObject FlowEditorScene::serializeClipboardSelection(bool deleteAfter)
{
    QJsonObject out;
    QJsonArray nodesArr;
    QJsonArray edgesArr;

    QHash<qint64, FlowSocketItem *> socketInSelection;

    const QList<QGraphicsItem *> sel = selectedItems();
    for (QGraphicsItem *gi : sel)
    {
        if (gi->type() == FlowNodeItem::Type)
        {
            auto *node = static_cast<FlowNodeItem *>(gi);
            nodesArr.append(nodeToJson(node));
            for (FlowSocketItem *s : node->inputs() + node->outputs())
            {
                socketInSelection.insert(s->socketId(), s);
            }
        }
    }

    for (QGraphicsItem *gi : sel)
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
        if (!socketInSelection.contains(edge->startSocket()->socketId())
            || !socketInSelection.contains(edge->endSocket()->socketId()))
        {
            continue;
        }
        QJsonObject eo;
        eo.insert(QStringLiteral("id"), double(edge->edgeId()));
        eo.insert(QStringLiteral("edge_type"), edge->edgeType());
        eo.insert(QStringLiteral("start"), double(edge->startSocket()->socketId()));
        eo.insert(QStringLiteral("end"), double(edge->endSocket()->socketId()));
        edgesArr.append(eo);
    }

    out.insert(QStringLiteral("nodes"), nodesArr);
    out.insert(QStringLiteral("edges"), edgesArr);

    if (deleteAfter)
    {
        QList<QGraphicsItem *> edgesToDelete;
        QList<QGraphicsItem *> nodesToDelete;
        for (QGraphicsItem *gi : sel)
        {
            if (gi->type() == FlowEdgeItem::Type)
            {
                edgesToDelete.append(gi);
            }
            else if (gi->type() == FlowNodeItem::Type)
            {
                nodesToDelete.append(gi);
            }
        }
        deselectAll();
        ++m_loadSilent;
        for (QGraphicsItem *gi : edgesToDelete)
        {
            removeEdge(static_cast<FlowEdgeItem *>(gi));
        }
        for (QGraphicsItem *gi : nodesToDelete)
        {
            removeNode(static_cast<FlowNodeItem *>(gi));
        }
        --m_loadSilent;
        m_history->storeHistory(QStringLiteral("Cut"), true);
    }

    return out;
}

static bool segmentCutsPath(const QPainterPath &edgePath, const QPointF &p1, const QPointF &p2)
{
    QPainterPath seg;
    seg.moveTo(p1);
    seg.lineTo(p2);
    QPainterPathStroker stroker;
    stroker.setWidth(10.0);
    const QPainterPath thick = stroker.createStroke(seg);
    return edgePath.intersects(thick);
}

void FlowEditorScene::cutEdgesAlongPolyline(const QVector<QPointF> &points)
{
    if (points.size() < 2)
    {
        return;
    }
    QList<FlowEdgeItem *> toRemove;
    const QList<QGraphicsItem *> all = items();
    for (QGraphicsItem *gi : all)
    {
        if (gi->type() != FlowEdgeItem::Type)
        {
            continue;
        }
        auto *edge = static_cast<FlowEdgeItem *>(gi);
        const QPainterPath ep = edge->path();
        for (int i = 0; i + 1 < points.size(); ++i)
        {
            if (segmentCutsPath(ep, points.at(i), points.at(i + 1)))
            {
                toRemove.append(edge);
                break;
            }
        }
    }
    if (toRemove.isEmpty())
    {
        return;
    }
    ++m_loadSilent;
    for (FlowEdgeItem *e : toRemove)
    {
        removeEdge(e);
    }
    --m_loadSilent;
    m_history->storeHistory(QStringLiteral("Delete cut edges"), true);
}

bool FlowEditorScene::pasteFromClipboardJson(const QJsonObject &data, const QPointF &mouseScenePos)
{
    const QJsonArray nodes = data.value(QStringLiteral("nodes")).toArray();
    if (nodes.isEmpty())
    {
        return false;
    }

    qreal minx = 1e15;
    qreal maxx = -1e15;
    qreal miny = 1e15;
    qreal maxy = -1e15;
    for (const QJsonValue &v : nodes)
    {
        const QJsonObject nd = v.toObject();
        const qreal x = nd.value(QStringLiteral("pos_x")).toDouble();
        const qreal y = nd.value(QStringLiteral("pos_y")).toDouble();
        minx = qMin(minx, x);
        maxx = qMax(maxx, x);
        miny = qMin(miny, y);
        maxy = qMax(maxy, y);
    }

    const qreal mousex = mouseScenePos.x();
    const qreal mousey = mouseScenePos.y();

    ++m_loadSilent;
    deselectAll();

    QHash<qint64, FlowSocketItem *> oldIdToSocket;

    for (const QJsonValue &v : nodes)
    {
        const QJsonObject nd = v.toObject();
        const QString nodeType = nd.value(QStringLiteral("node_type")).toString();
        const QString title = nd.value(QStringLiteral("title")).toString();
        const qreal px = nd.value(QStringLiteral("pos_x")).toDouble();
        const qreal py = nd.value(QStringLiteral("pos_y")).toDouble();
        const qreal newx = mousex + px - minx;
        const qreal newy = mousey + py - miny;

        FlowNodeItem *node = spawnNode(nodeType, QPointF(newx, newy), title);
        if (!node)
        {
            continue;
        }
        if (nd.contains(QStringLiteral("node_settings")))
        {
            node->setSettings(nd.value(QStringLiteral("node_settings")).toObject());
        }

        QJsonArray inArr = nd.value(QStringLiteral("inputs")).toArray();
        QJsonArray outArr = nd.value(QStringLiteral("outputs")).toArray();
        QVector<QJsonObject> inObjs;
        QVector<QJsonObject> outObjs;
        for (const QJsonValue &iv : inArr)
        {
            inObjs.append(iv.toObject());
        }
        for (const QJsonValue &iv : outArr)
        {
            outObjs.append(iv.toObject());
        }
        std::sort(inObjs.begin(), inObjs.end(), socketLess);
        std::sort(outObjs.begin(), outObjs.end(), socketLess);

        for (int i = 0; i < inObjs.size() && i < node->inputs().size(); ++i)
        {
            const qint64 oldId = qint64(inObjs.at(i).value(QStringLiteral("id")).toDouble());
            oldIdToSocket.insert(oldId, node->inputs().at(i));
        }
        for (int i = 0; i < outObjs.size() && i < node->outputs().size(); ++i)
        {
            const qint64 oldId = qint64(outObjs.at(i).value(QStringLiteral("id")).toDouble());
            oldIdToSocket.insert(oldId, node->outputs().at(i));
        }
        node->setSelected(true);
    }

    const QJsonArray edges = data.value(QStringLiteral("edges")).toArray();
    for (const QJsonValue &v : edges)
    {
        const QJsonObject ed = v.toObject();
        const qint64 sidStart = qint64(ed.value(QStringLiteral("start")).toDouble());
        const qint64 sidEnd = qint64(ed.value(QStringLiteral("end")).toDouble());
        FlowSocketItem *sa = oldIdToSocket.value(sidStart, nullptr);
        FlowSocketItem *sb = oldIdToSocket.value(sidEnd, nullptr);
        if (!sa || !sb)
        {
            continue;
        }
        const int et = ed.value(QStringLiteral("edge_type")).toInt(2);
        makeEdge(sa, sb, -1, et);
    }

    --m_loadSilent;
    resumeIdCounterFromScene();
    m_history->storeHistory(QStringLiteral("Paste"), true);
    emit selectionChangedInScene();
    return true;
}
