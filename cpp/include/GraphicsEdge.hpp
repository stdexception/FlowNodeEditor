#pragma once

#include <optional>

#include <QGraphicsPathItem>
#include <QJsonObject>
#include <QPointF>

#include "ObjectId.hpp"

class EditorScene;
class GraphicsSocket;

/**
 * Logical + visual edge (Python Edge + QDMGraphicsEdge merged for the C++ port).
 */
class GraphicsEdge : public QGraphicsPathItem {
public:
    static constexpr int kEdgeTypeBezier = 2;

    GraphicsEdge(EditorScene* scene, GraphicsSocket* start, GraphicsSocket* end);

    ~GraphicsEdge() override;

    [[nodiscard]] quint64 objectId() const { return objectId_; }
    void assignObjectId(quint64 id) { objectId_ = id; }

    [[nodiscard]] EditorScene* document() const { return scene_; }
    [[nodiscard]] GraphicsSocket* startSocket() const { return start_; }
    [[nodiscard]] GraphicsSocket* endSocket() const { return end_; }

    [[nodiscard]] int edgeType() const { return edgeType_; }
    void setEdgeType(int t) { edgeType_ = t; }

    void setEndSocket(GraphicsSocket* end);
    void setFreeEndScene(const QPointF& scenePos);
    void clearFreeEnd();

    void updatePath();

    /** Unregister from sockets, scene, and editor list; does not delete this. */
    void removeFromDocument();

    [[nodiscard]] QJsonObject toJson() const;

private:
    void attachSockets();
    void detachSockets();

    quint64 objectId_{allocateObjectId()};
    EditorScene* scene_{nullptr};
    GraphicsSocket* start_{nullptr};
    GraphicsSocket* end_{nullptr};
    std::optional<QPointF> freeEndScene_;
    int edgeType_{kEdgeTypeBezier};
};
