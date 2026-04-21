#pragma once

#include <QGraphicsPathItem>

class FlowSocketItem;

class FlowEdgeItem : public QGraphicsPathItem
{
public:
    enum
    {
        Type = UserType + 3
    };
    int type() const override
    {
        return Type;
    }

    FlowEdgeItem(FlowSocketItem *startSocket, FlowSocketItem *endSocket, qint64 edgeId, int edgeType);

    qint64 edgeId() const
    {
        return m_edgeId;
    }
    void setEdgeId(qint64 id)
    {
        m_edgeId = id;
    }

    FlowSocketItem *startSocket() const
    {
        return m_start;
    }
    FlowSocketItem *endSocket() const
    {
        return m_end;
    }

    int edgeType() const
    {
        return m_edgeType;
    }
    void setEdgeType(int t)
    {
        m_edgeType = t;
    }

    void updatePath();
    void setDragEndPoint(const QPointF &scenePos);

    static bool validateConnection(FlowSocketItem *a, FlowSocketItem *b);

private:
    QPainterPath computeBezier(const QPointF &s, const QPointF &d) const;

    FlowSocketItem *m_start = nullptr;
    FlowSocketItem *m_end = nullptr;
    qint64 m_edgeId = 0;
    int m_edgeType = 2;
    bool m_dragging = false;
    QPointF m_dragEndScene;
};
