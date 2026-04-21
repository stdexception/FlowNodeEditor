#pragma once

#include <QGraphicsEllipseItem>
#include <QString>
#include <QVector>

class GraphicsEdge;

class GraphicsNode;

class GraphicsSocket : public QGraphicsEllipseItem {
public:
    enum class SocketType { Input, Output };

    GraphicsSocket(GraphicsNode* node, SocketType type, int index, QGraphicsItem* parent = nullptr);

    [[nodiscard]] GraphicsNode* node() const { return node_; }
    [[nodiscard]] SocketType socketKind() const { return type_; }
    [[nodiscard]] int index() const { return index_; }

    [[nodiscard]] bool isInput() const { return type_ == SocketType::Input; }
    [[nodiscard]] bool isOutput() const { return type_ == SocketType::Output; }

    /** Data type string for validation (Python socket_type); default "any". */
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

private:
    GraphicsNode* node_{nullptr};
    SocketType type_{SocketType::Input};
    int index_{0};
    QString dataType_{QStringLiteral("any")};
    bool multiEdges_{false};
    QVector<GraphicsEdge*> edges_;
    static constexpr qreal kRadius = 6.0;
};
