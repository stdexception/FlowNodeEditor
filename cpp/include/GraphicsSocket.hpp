#pragma once

#include <QGraphicsEllipseItem>

class GraphicsNode;

class GraphicsSocket : public QGraphicsEllipseItem {
public:
    enum class SocketType { Input, Output };

    GraphicsSocket(GraphicsNode* node, SocketType type, int index, QGraphicsItem* parent = nullptr);

    [[nodiscard]] GraphicsNode* node() const { return node_; }
    [[nodiscard]] SocketType socketType() const { return type_; }
    [[nodiscard]] int index() const { return index_; }

    void updateSocketPosition();

private:
    GraphicsNode* node_;
    SocketType type_;
    int index_;
    static constexpr qreal kRadius = 6.0;
};
