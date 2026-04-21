#pragma once

#include <QGraphicsEllipseItem>
#include <QString>
#include <QVector>

class FlowNodeItem;
class FlowEdgeItem;

class FlowSocketItem : public QGraphicsEllipseItem
{
public:
    enum
    {
        Type = UserType + 2
    };
    int type() const override
    {
        return Type;
    }

    FlowSocketItem(FlowNodeItem *node, int index, bool isInput, const QString &name, const QString &dataType,
                   int countOnSide, qint64 socketId);

    FlowNodeItem *flowNode() const
    {
        return m_node;
    }
    int socketIndex() const
    {
        return m_index;
    }
    bool isInput() const
    {
        return m_isInput;
    }
    const QString &socketName() const
    {
        return m_name;
    }
    const QString &dataType() const
    {
        return m_dataType;
    }
    qint64 socketId() const
    {
        return m_id;
    }
    void setSocketId(qint64 id)
    {
        m_id = id;
    }

    QPointF pinScenePos() const;

    QVector<FlowEdgeItem *> edges;
    bool multiEdges = false;

private:
    FlowNodeItem *m_node;
    int m_index;
    bool m_isInput;
    QString m_name;
    QString m_dataType;
    qint64 m_id;
};
