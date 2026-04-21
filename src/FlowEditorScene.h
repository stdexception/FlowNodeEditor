#pragma once

#include <QGraphicsScene>
#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QVector>

class FlowNodeItem;
class FlowSocketItem;
class FlowEdgeItem;
class NodeTypeRegistry;
class FlowEditorSceneHistory;

class FlowEditorScene : public QGraphicsScene
{
    Q_OBJECT

public:
    explicit FlowEditorScene(QObject *parent = nullptr);
    ~FlowEditorScene() override;

    void setRegistry(NodeTypeRegistry *registry)
    {
        m_registry = registry;
    }
    NodeTypeRegistry *registry() const
    {
        return m_registry;
    }

    FlowEditorSceneHistory *history() const
    {
        return m_history;
    }

    int loadSilentDepth() const
    {
        return m_loadSilent;
    }

    void beginBulkEdit()
    {
        ++m_loadSilent;
    }
    void endBulkEdit()
    {
        --m_loadSilent;
    }

    qint64 takeNextId();
    void seedNextId(qint64 minValue);
    int registerNodeTypeOccurrence(const QString &nodeType);

    FlowNodeItem *spawnNode(const QString &nodeType, const QPointF &scenePos, const QString &titleOverride = QString(),
                            qint64 forcedNodeId = -1, const QVector<qint64> &presetInputSocketIds = QVector<qint64>(),
                            const QVector<qint64> &presetOutputSocketIds = QVector<qint64>());

    void removeNode(FlowNodeItem *node);
    void removeEdge(FlowEdgeItem *edge);

    FlowEdgeItem *makeEdge(FlowSocketItem *startSock, FlowSocketItem *endSock, qint64 edgeId, int edgeType);

    void refreshEdgesForNode(FlowNodeItem *node);

    void clearDocument();

    QJsonObject snapshotJson() const;

    bool importGraphJson(const QJsonObject &o);

    void removeAllGraphItems();

    void restoreFromSnapshotJson(const QJsonObject &sceneJson, const QJsonObject &selectionJson);

    FlowNodeItem *findNodeById(qint64 nodeId) const;
    FlowEdgeItem *findEdgeById(qint64 edgeId) const;

    void deselectAll();

    QJsonObject serializeClipboardSelection(bool deleteAfter);

    bool pasteFromClipboardJson(const QJsonObject &data, const QPointF &mouseScenePos);

    void cutEdgesAlongPolyline(const QVector<QPointF> &points);

    bool saveSceneFile(const QString &path) const;
    bool loadSceneFile(const QString &path);

    bool saveGraphFile(const QString &path) const;

    bool saveGraphToTempFile(QString *outPath) const;

    QString currentFilePath() const
    {
        return m_filePath;
    }
    void setCurrentFilePath(const QString &p)
    {
        m_filePath = p;
    }

    bool isModified() const
    {
        return m_modified;
    }
    void setModified(bool m);

signals:
    void modificationChanged(bool modified);
    void selectionChangedInScene();

private:
    bool deserializeScene(const QJsonObject &o);
    void resumeIdCounterFromScene();

    NodeTypeRegistry *m_registry = nullptr;
    FlowEditorSceneHistory *m_history = nullptr;
    qint64 m_sceneId = 1;
    qint64 m_nextEntityId = 1;
    QHash<QString, int> m_typeOccurrence;
    QString m_filePath;
    bool m_modified = false;
    int m_loadSilent = 0;
};
