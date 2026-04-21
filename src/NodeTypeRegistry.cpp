#include "NodeTypeRegistry.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

NodeTypeRegistry::NodeTypeRegistry(QObject *parent)
    : QObject(parent)
{
}

void NodeTypeRegistry::loadFromDirectory(const QString &rootPath)
{
    m_byType.clear();
    m_categoryOrder.clear();
    m_typesByCategory.clear();

    QDir root(rootPath);
    if (!root.exists())
    {
        return;
    }

    const QFileInfoList entries = root.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &catFi : entries)
    {
        if (!catFi.isDir())
        {
            continue;
        }

        const QString category = catFi.fileName();
        m_categoryOrder.append(category);

        QDir catDir(catFi.absoluteFilePath());
        const QFileInfoList jsonFiles = catDir.entryInfoList(QStringList() << "*.json", QDir::Files, QDir::Name);
        for (const QFileInfo &jf : jsonFiles)
        {
            QFile f(jf.absoluteFilePath());
            if (!f.open(QIODevice::ReadOnly))
            {
                continue;
            }

            const QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
            f.close();
            if (!doc.isObject())
            {
                continue;
            }

            const QJsonObject o = doc.object();
            NodeTypeInfo info;
            info.category = category;
            info.nodeType = o.value(QStringLiteral("node_type")).toString();
            if (info.nodeType.isEmpty())
            {
                continue;
            }

            const QJsonArray inPorts = o.value(QStringLiteral("input_ports")).toArray();
            for (const QJsonValue &v : inPorts)
            {
                const QJsonObject p = v.toObject();
                PortInfo pi;
                pi.name = p.value(QStringLiteral("port_name")).toString();
                pi.dataType = p.value(QStringLiteral("data_type")).toString();
                if (p.contains(QStringLiteral("multi_edges")))
                {
                    pi.multiEdges = p.value(QStringLiteral("multi_edges")).toBool();
                }
                info.inputs.append(pi);
            }

            const QJsonArray outPorts = o.value(QStringLiteral("output_ports")).toArray();
            for (const QJsonValue &v : outPorts)
            {
                const QJsonObject p = v.toObject();
                PortInfo pi;
                pi.name = p.value(QStringLiteral("port_name")).toString();
                pi.dataType = p.value(QStringLiteral("data_type")).toString();
                if (p.contains(QStringLiteral("multi_edges")))
                {
                    pi.multiEdges = p.value(QStringLiteral("multi_edges")).toBool();
                }
                info.outputs.append(pi);
            }

            info.defaultSettings = o.value(QStringLiteral("default_settings")).toObject();

            m_byType.insert(info.nodeType, info);
            m_typesByCategory[category].append(info.nodeType);
        }
    }
}

QVector<NodeTypeInfo> NodeTypeRegistry::typesInCategory(const QString &category) const
{
    QVector<NodeTypeInfo> out;
    const QVector<QString> ids = m_typesByCategory.value(category);
    for (const QString &id : ids)
    {
        const NodeTypeInfo *p = findType(id);
        if (p)
        {
            out.append(*p);
        }
    }
    return out;
}

QStringList NodeTypeRegistry::categories() const
{
    return m_categoryOrder;
}

const NodeTypeInfo *NodeTypeRegistry::findType(const QString &nodeType) const
{
    auto it = m_byType.constFind(nodeType);
    if (it == m_byType.cend())
    {
        return nullptr;
    }
    return &(*it);
}
