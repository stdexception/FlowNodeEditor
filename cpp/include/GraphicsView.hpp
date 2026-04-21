#pragma once

#include <QGraphicsView>

class GraphicsScene;

class GraphicsView : public QGraphicsView {
    Q_OBJECT
public:
    explicit GraphicsView(GraphicsScene* grScene, QWidget* parent = nullptr);

signals:
    void scenePosChanged(int x, int y);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    double zoomFactor_{1.25};
    int zoom_{10};
    const int zoomMin_{1};
    const int zoomMax_{20};
};
