#include "FlowEditorSceneHistory.h"

#include "FlowEdgeItem.h"
#include "FlowEditorScene.h"
#include "FlowNodeItem.h"

#include <QGraphicsItem>
#include <QJsonArray>

FlowEditorSceneHistory::FlowEditorSceneHistory(FlowEditorScene *scene, QObject *parent)
    : QObject(parent)
    , m_scene(scene)
{
    clear();
}

void FlowEditorSceneHistory::clear()
{
    m_stack.clear();
    m_currentStep = -1;
}

void FlowEditorSceneHistory::storeInitialHistoryStamp()
{
    storeHistory(QStringLiteral("Initial History Stamp"));
}

void FlowEditorSceneHistory::storeHistory(const QString &desc, bool setModified)
{
    if (!m_scene || m_scene->loadSilentDepth() > 0 || m_restoring)
    {
        return;
    }

    if (setModified)
    {
        m_scene->setModified(true);
    }

    if (m_currentStep + 1 < m_stack.size())
    {
        m_stack.resize(m_currentStep + 1);
    }

    if (m_currentStep + 1 >= kHistoryLimit)
    {
        m_stack.removeFirst();
        --m_currentStep;
    }

    m_stack.append(createStamp(desc));
    ++m_currentStep;

    emit historyModified();
}

bool FlowEditorSceneHistory::canUndo() const
{
    return m_currentStep > 0;
}

bool FlowEditorSceneHistory::canRedo() const
{
    return m_currentStep + 1 < m_stack.size();
}

void FlowEditorSceneHistory::undo()
{
    if (!canUndo())
    {
        return;
    }
    --m_currentStep;
    m_restoring = true;
    restoreStamp(m_stack.at(m_currentStep));
    m_restoring = false;
    m_scene->setModified(true);
    emit historyModified();
    emit historyRestored();
}

void FlowEditorSceneHistory::redo()
{
    if (!canRedo())
    {
        return;
    }
    ++m_currentStep;
    m_restoring = true;
    restoreStamp(m_stack.at(m_currentStep));
    m_restoring = false;
    m_scene->setModified(true);
    emit historyModified();
    emit historyRestored();
}

QJsonObject FlowEditorSceneHistory::captureSelection() const
{
    QJsonArray nodeIds;
    QJsonArray edgeIds;
    const QList<QGraphicsItem *> sel = m_scene->selectedItems();
    for (QGraphicsItem *gi : sel)
    {
        if (gi->type() == FlowNodeItem::Type)
        {
            nodeIds.append(double(static_cast<FlowNodeItem *>(gi)->nodeId()));
        }
        else if (gi->type() == FlowEdgeItem::Type)
        {
            edgeIds.append(double(static_cast<FlowEdgeItem *>(gi)->edgeId()));
        }
    }
    QJsonObject o;
    o.insert(QStringLiteral("nodes"), nodeIds);
    o.insert(QStringLiteral("edges"), edgeIds);
    return o;
}

FlowEditorSceneHistory::Stamp FlowEditorSceneHistory::createStamp(const QString &desc)
{
    Stamp s;
    s.desc = desc;
    s.snapshot = m_scene->snapshotJson();
    s.selection = captureSelection();
    return s;
}

void FlowEditorSceneHistory::restoreStamp(const Stamp &stamp)
{
    m_scene->restoreFromSnapshotJson(stamp.snapshot, stamp.selection);
}
