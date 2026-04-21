#pragma once

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>

struct PortInfo
{
    QString name;
    QString dataType;
};

struct NodeTypeInfo
{
    QString nodeType;
    QString category;
    QVector<PortInfo> inputs;
    QVector<PortInfo> outputs;
    QJsonObject defaultSettings;
};

class NodeTypeRegistry : public QObject
{
public:
    explicit NodeTypeRegistry(QObject *parent = nullptr);

    void loadFromDirectory(const QString &rootPath);

    QVector<NodeTypeInfo> typesInCategory(const QString &category) const;
    QStringList categories() const;

    const NodeTypeInfo *findType(const QString &nodeType) const;

private:
    QHash<QString, NodeTypeInfo> m_byType;
    QStringList m_categoryOrder;
    QHash<QString, QVector<QString>> m_typesByCategory;
};
