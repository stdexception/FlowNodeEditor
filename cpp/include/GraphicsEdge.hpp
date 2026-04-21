#pragma once

#include <QGraphicsPathItem>

class EditorScene;
class GraphicsSocket;

class GraphicsEdge : public QGraphicsPathItem {
public:
    GraphicsEdge(EditorScene* scene, GraphicsSocket* start, GraphicsSocket* end);

    [[nodiscard]] GraphicsSocket* startSocket() const { return start_; }
    [[nodiscard]] GraphicsSocket* endSocket() const { return end_; }

    void updatePath();

private:
    EditorScene* scene_;
    GraphicsSocket* start_;
    GraphicsSocket* end_;
};
