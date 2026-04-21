#pragma once

#include <QGraphicsEllipseItem>
#include <QHash>
#include <QJsonObject>
#include <QString>
#include <QVector>

#include "ObjectId.hpp"

class GraphicsEdge;
class GraphicsNode;

/** Python LEFT_CENTER for serialized compatibility */
constexpr int kSocketPositionLeftCenter = 2;
constexpr int kSocketPositionRightCenter = 5;

class GraphicsSocket : public QGraphicsEllipseItem {
public:
    enum class SocketType { Input, Output };

    GraphicsSocket(GraphicsNode* node, SocketType type, int index, QGraphicsItem* parent = nullptr);

    [[nodiscard]] quint64 objectId() const { return objectId_; }
    void assignObjectId(quint64 id) { objectId_ = id; }

    [[nodiscard]] GraphicsNode* node() const { return node_; }
    [[nodiscard]] SocketType socketKind() const { return type_; }
    [[nodiscard]] int index() const { return index_; }
    [[nodiscard]] int position() const { return position_; }
    void setPosition(int p) { position_ = p; }

    [[nodiscard]] bool isInput() const { return type_ == SocketType::Input; }
    [[nodiscard]] bool isOutput() const { return type_ == SocketType::Output; }

    [[nodiscard]] QString dataType() const { return dataType_; }
    void setDataType(const QString& t) { dataType_ = t; }

    [[nodiscard]] bool multiEdges() const { return multiEdges_; }
    void setMultiEdges(bool v) { multiEdges_ = v; }

    void updateSocketPosition();

    void addEdge(GraphicsEdge* e);
    void removeEdge(GraphicsEdge* e);
    [[nodiscard]] const QVector<GraphicsEdge*>& edges() const { return edges_; }
    [[nodiscard]] bool hasEdges() const { return !edges_.isEmpty(); }

    void removeAllEdges();

    [[nodiscard]] QJsonObject toJson() const;
    static GraphicsSocket* fromJson(GraphicsNode* node, SocketType type, const QJsonObject& o, bool restoreId,
                                     QHash<quint64, GraphicsSocket*>& socketMap);

private:
    quint64 objectId_{allocateObjectId()};
    GraphicsNode* node_{nullptr};
    SocketType type_{SocketType::Input};
    int index_{0};
    int position_{kSocketPositionLeftCenter};
    QString dataType_{QStringLiteral("any")};
    bool multiEdges_{false};
    QVector<GraphicsEdge*> edges_;
    static constexpr qreal kRadius = 6.0;
};
