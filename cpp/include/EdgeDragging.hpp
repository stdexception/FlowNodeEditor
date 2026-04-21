#pragma once

#include <QPointF>

class GraphicsView;
class GraphicsSocket;
class GraphicsEdge;
class EditorScene;

/**
 * C++ analogue of node_edge_dragging.EdgeDragging — temporary edge while LMB drags from a socket.
 */
class EdgeDragging {
public:
    explicit EdgeDragging(GraphicsView* view);

    void edgeDragStart(GraphicsSocket* itemSocket);
    /** @return true if caller should skip further mouse processing (Python edgeDragEnd) */
    bool edgeDragEnd(GraphicsSocket* releaseSocket);

    void updateDestination(qreal x, qreal y);
    void cancelDrag();

    [[nodiscard]] bool isDragging() const { return dragEdge_ != nullptr; }

private:
    GraphicsView* view_{nullptr};
    GraphicsEdge* dragEdge_{nullptr};
    GraphicsSocket* dragStart_{nullptr};
    QPointF dragCursorScene_;
};
