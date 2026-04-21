#pragma once

#include <QGraphicsView>
#include <QPointF>

#include "EdgeDragging.hpp"

class GraphicsScene;

enum class ViewInteractionMode {
    NoOp = 1,
    EdgeDrag = 2,
};

class GraphicsView : public QGraphicsView {
    Q_OBJECT
public:
    explicit GraphicsView(GraphicsScene* grScene, QWidget* parent = nullptr);

    void resetMode();

signals:
    void scenePosChanged(int x, int y);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    [[nodiscard]] QGraphicsItem* itemAtClick(const QMouseEvent* event) const;
    [[nodiscard]] bool distanceClickReleaseExceedsThreshold(const QMouseEvent* event) const;

    EdgeDragging dragging_{this};
    ViewInteractionMode mode_{ViewInteractionMode::NoOp};
    QPointF lastLmbScenePos_;
    static constexpr int kEdgeDragThresholdPx = 50;

    double zoomFactor_{1.25};
    int zoom_{10};
    const int zoomMin_{1};
    const int zoomMax_{20};
};
