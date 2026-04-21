#pragma once

#include <QGraphicsRectItem>
#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QVector>

#include "ObjectId.hpp"

class EditorScene;
class GraphicsSocket;
class QGraphicsTextItem;

class GraphicsNode : public QGraphicsRectItem {
public:
    explicit GraphicsNode(EditorScene* scene, const QString& title,
                          const QString& nodeType = QStringLiteral("Node"), bool addDefaultSockets = true);
    ~GraphicsNode() override;

    [[nodiscard]] quint64 objectId() const { return objectId_; }
    void assignObjectId(quint64 id) { objectId_ = id; }

    [[nodiscard]] EditorScene* document() const { return scene_; }
    [[nodiscard]] QString title() const { return title_; }
    void setTitle(const QString& t);

    [[nodiscard]] QString nodeType() const { return nodeType_; }
    void setNodeType(const QString& t) { nodeType_ = t; }

    [[nodiscard]] QVector<GraphicsSocket*> inputSockets() const { return inputs_; }
    [[nodiscard]] QVector<GraphicsSocket*> outputSockets() const { return outputs_; }

    void addInputSocket();
    void addOutputSocket();

    /** Remove node and all connected edges from the document (delete self). */
    void removeFromDocument();

    [[nodiscard]] QJsonObject toJson() const;
    static GraphicsNode* fromJson(EditorScene* scene, const QJsonObject& o, bool restoreId,
                                  QHash<quint64, GraphicsSocket*>& socketMap);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    void layoutTitle();
    void clearSockets();
    void detachFromDocument();

    quint64 objectId_{allocateObjectId()};
    EditorScene* scene_{nullptr};
    QString title_;
    QString nodeType_;
    QGraphicsTextItem* titleItem_{nullptr};
    QVector<GraphicsSocket*> inputs_;
    QVector<GraphicsSocket*> outputs_;
    bool detached_{false};
};
