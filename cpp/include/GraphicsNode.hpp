#pragma once

#include <QString>
#include <QVector>

#include <QGraphicsRectItem>

class EditorScene;
class GraphicsSocket;
class QGraphicsTextItem;

class GraphicsNode : public QGraphicsRectItem {
public:
    explicit GraphicsNode(EditorScene* scene, const QString& title);

    [[nodiscard]] EditorScene* document() const { return scene_; }
    [[nodiscard]] QString title() const { return title_; }
    void setTitle(const QString& t);

    [[nodiscard]] QVector<GraphicsSocket*> inputSockets() const { return inputs_; }
    [[nodiscard]] QVector<GraphicsSocket*> outputSockets() const { return outputs_; }

    void addInputSocket();
    void addOutputSocket();

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    void layoutTitle();

    EditorScene* scene_;
    QString title_;
    QGraphicsTextItem* titleItem_{nullptr};
    QVector<GraphicsSocket*> inputs_;
    QVector<GraphicsSocket*> outputs_;
};
