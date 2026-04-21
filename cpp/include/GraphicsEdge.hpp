#pragma once

#include <optional>

#include <QGraphicsPathItem>
#include <QPointF>

class EditorScene;
class GraphicsSocket;

/**
 * Logical + visual edge (Python Edge + QDMGraphicsEdge merged for the C++ port).
 */
class GraphicsEdge : public QGraphicsPathItem {
public:
    GraphicsEdge(EditorScene* scene, GraphicsSocket* start, GraphicsSocket* end);

    ~GraphicsEdge() override;

    [[nodiscard]] EditorScene* document() const { return scene_; }
    [[nodiscard]] GraphicsSocket* startSocket() const { return start_; }
    [[nodiscard]] GraphicsSocket* endSocket() const { return end_; }

    void setEndSocket(GraphicsSocket* end);
    void setFreeEndScene(const QPointF& scenePos);
    void clearFreeEnd();

    void updatePath();

    /** Unregister from sockets, scene, and editor list; does not delete this. */
    void removeFromDocument();

private:
    void attachSockets();
    void detachSockets();

    EditorScene* scene_{nullptr};
    GraphicsSocket* start_{nullptr};
    GraphicsSocket* end_{nullptr};
    std::optional<QPointF> freeEndScene_;
};
