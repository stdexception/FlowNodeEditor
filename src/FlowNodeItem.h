#pragma once

#include <QGraphicsItem>
#include <QJsonObject>
#include <QString>
#include <QVector>

#include <QtGlobal>

class FlowSocketItem;
class FlowEditorScene;
struct NodeTypeInfo;

class FlowNodeItem : public QGraphicsItem
{
public:
    enum
    {
        Type = UserType + 1
    };
    int type() const override
    {
        return Type;
    }

    FlowNodeItem(FlowEditorScene *editorScene, const NodeTypeInfo &info, qint64 nodeId,
                 const QString &titleOverride = QString(), bool registerTypeOccurrence = true,
                 const QVector<qint64> &presetInputSocketIds = QVector<qint64>(),
                 const QVector<qint64> &presetOutputSocketIds = QVector<qint64>());

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

    qint64 nodeId() const
    {
        return m_nodeId;
    }
    void setNodeId(qint64 id)
    {
        m_nodeId = id;
    }

    QString title() const
    {
        return m_title;
    }
    void setTitle(const QString &t);

    QString nodeType() const
    {
        return m_nodeType;
    }

    QJsonObject settings() const
    {
        return m_settings;
    }
    void setSettings(const QJsonObject &s)
    {
        m_settings = s;
    }

    const QVector<FlowSocketItem *> &inputs() const
    {
        return m_inputs;
    }
    const QVector<FlowSocketItem *> &outputs() const
    {
        return m_outputs;
    }

    FlowSocketItem *findSocketById(qint64 socketId) const;

    QPointF socketLocalPos(int index, bool isInput, int countOnSide) const;

private:
    void layoutSockets();
    QString makeUniqueTitle(const QString &base) const;

    FlowEditorScene *m_editorScene;
    qint64 m_nodeId = 0;
    QString m_title;
    QString m_nodeType;
    QJsonObject m_settings;

    qreal m_width = 150.0;
    qreal m_height = 120.0;

    QVector<FlowSocketItem *> m_inputs;
    QVector<FlowSocketItem *> m_outputs;
};
