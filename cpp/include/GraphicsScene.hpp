#pragma once

#include <QColor>
#include <QGraphicsScene>
#include <QPen>

class EditorScene;

class GraphicsScene : public QGraphicsScene {
    Q_OBJECT
public:
    explicit GraphicsScene(EditorScene* document, QObject* parent = nullptr);

    void setGridSceneSize(int width, int height);

    EditorScene* document() const { return document_; }

signals:
    void itemSelected();
    void itemsDeselected();

protected:
    void drawBackground(QPainter* painter, const QRectF& rect) override;

private:
    EditorScene* document_;
    QColor colorBackground_;
    QColor colorLight_;
    QColor colorDark_;
    QPen penLight_;
    QPen penDark_;
    int gridSize_{50};
    int gridSquares_{20};
};
