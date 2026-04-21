#pragma once

#include <QGraphicsPathItem>
#include <QGraphicsView>

class FlowEditorScene;
class FlowSocketItem;

class FlowGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit FlowGraphicsView(FlowEditorScene *scene, QWidget *parent = nullptr);

    QPointF visibleSceneCenter() const;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    FlowSocketItem *socketAt(const QPoint &viewPos) const;
    static QPainterPath bezierPreview(const QPointF &from, const QPointF &to);

    FlowEditorScene *m_flowScene = nullptr;
    FlowSocketItem *m_dragOrigin = nullptr;
    QGraphicsPathItem *m_dragLine = nullptr;
};
