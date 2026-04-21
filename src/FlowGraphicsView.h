#pragma once

#include <QGraphicsPathItem>
#include <QGraphicsView>
#include <QPointF>
#include <QVector>

class FlowEditorScene;
class FlowSocketItem;
class FlowNodeItem;
class FlowEdgeItem;

class FlowGraphicsView : public QGraphicsView
{
    Q_OBJECT

public:
    explicit FlowGraphicsView(FlowEditorScene *scene, QWidget *parent = nullptr);

    QPointF visibleSceneCenter() const;
    QPointF lastSceneMousePos() const
    {
        return m_lastSceneMousePos;
    }

signals:
    void sceneCursorPosChanged(int x, int y);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    enum class Mode
    {
        Noop,
        EdgeDrag,
        EdgeCut,
        EdgeReroute
    };

    FlowSocketItem *socketAt(const QPoint &viewPos) const;
    FlowSocketItem *nearestSocketSnap(const QPointF &scenePos, FlowSocketItem *ignore) const;
    static QPainterPath bezierPreview(const QPointF &from, const QPointF &to);

    void finishCutMode();
    void trySnapEdgeTarget(const QPoint &viewPos);
    bool tryDropNodeOnEdgeSplit(FlowNodeItem *node);

    void startEdgeReroute(FlowSocketItem *startSocket);
    void updateEdgeReroutePreview(const QPoint &viewPos);
    void finishEdgeReroute(FlowSocketItem *targetSocket);

    FlowEditorScene *m_flowScene = nullptr;
    FlowSocketItem *m_dragOrigin = nullptr;
    QGraphicsPathItem *m_dragLine = nullptr;
    Mode m_mode = Mode::Noop;
    QVector<QPointF> m_cutPoints;
    QGraphicsPathItem *m_cutRubber = nullptr;
    QPointF m_lastSceneMousePos;
    bool m_maybeDragNode = false;
    FlowNodeItem *m_draggedNode = nullptr;
    QPoint m_pressViewPos;
    qreal m_zoom = 1.0;

    FlowSocketItem *m_rerouteStart = nullptr;
    QVector<FlowEdgeItem *> m_rerouteAffectedEdges;
    QVector<FlowSocketItem *> m_rerouteOtherSockets;
    QVector<QGraphicsPathItem *> m_reroutePreview;
};
