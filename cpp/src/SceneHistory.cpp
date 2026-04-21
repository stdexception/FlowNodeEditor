#include "SceneHistory.hpp"
#include "EditorScene.hpp"
#include "GraphicsEdge.hpp"
#include "GraphicsNode.hpp"
#include "GraphicsSocket.hpp"
#include "GraphicsScene.hpp"

#include <QGraphicsItem>
#include <QJsonArray>

SceneHistory::SceneHistory(EditorScene* scene)
    : scene_(scene)
{
    clear();
}

void SceneHistory::clear()
{
    stack_.clear();
    currentStep_ = -1;
}

void SceneHistory::addHistoryModifiedListener(std::function<void()> cb)
{
    modifiedListeners_.push_back(std::move(cb));
}

void SceneHistory::addHistoryStoredListener(std::function<void()> cb)
{
    storedListeners_.push_back(std::move(cb));
}

void SceneHistory::addHistoryRestoredListener(std::function<void()> cb)
{
    restoredListeners_.push_back(std::move(cb));
}

bool SceneHistory::canUndo() const
{
    return currentStep_ > 0;
}

bool SceneHistory::canRedo() const
{
    return currentStep_ + 1 < static_cast<int>(stack_.size());
}

void SceneHistory::notifyModified()
{
    for (const auto& cb : modifiedListeners_)
        cb();
}

void SceneHistory::notifyStored()
{
    for (const auto& cb : storedListeners_)
        cb();
}

void SceneHistory::notifyRestored()
{
    for (const auto& cb : restoredListeners_)
        cb();
}

QJsonObject SceneHistory::captureCurrentSelection() const
{
    QJsonArray nodeIds;
    QJsonArray edgeIds;
    if (!scene_ || !scene_->graphicsScene())
        return QJsonObject{{QStringLiteral("nodes"), nodeIds}, {QStringLiteral("edges"), edgeIds}};

    for (QGraphicsItem* it : scene_->graphicsScene()->selectedItems()) {
        if (auto* n = dynamic_cast<GraphicsNode*>(it))
            nodeIds.append(static_cast<qint64>(n->objectId()));
        else if (auto* e = dynamic_cast<GraphicsEdge*>(it))
            edgeIds.append(static_cast<qint64>(e->objectId()));
    }
    return QJsonObject{{QStringLiteral("nodes"), nodeIds}, {QStringLiteral("edges"), edgeIds}};
}

SceneHistory::Stamp SceneHistory::createHistoryStamp(const QString& desc) const
{
    Stamp s;
    s.desc = desc;
    s.snapshot = scene_ ? scene_->toJson() : QJsonObject{};
    s.selection = captureCurrentSelection();
    return s;
}

void SceneHistory::storeInitialHistoryStamp()
{
    storeHistory(QStringLiteral("Initial History Stamp"));
}

void SceneHistory::storeHistory(const QString& desc, bool setModified)
{
    if (!scene_)
        return;
    if (setModified)
        scene_->setModified(true);

    if (currentStep_ + 1 < static_cast<int>(stack_.size()))
        stack_.resize(static_cast<size_t>(currentStep_) + 1);

    if (currentStep_ + 1 >= historyLimit_) {
        if (!stack_.empty()) {
            stack_.erase(stack_.begin());
            --currentStep_;
        }
    }

    stack_.push_back(createHistoryStamp(desc));
    ++currentStep_;

    notifyModified();
    notifyStored();
}

void SceneHistory::restoreHistoryStamp(const Stamp& stamp)
{
    if (!scene_)
        return;
    scene_->fromJson(stamp.snapshot);

    if (scene_->graphicsScene()) {
        for (GraphicsEdge* e : scene_->edges())
            e->setSelected(false);
        for (GraphicsNode* n : scene_->nodes())
            n->setSelected(false);

        const QJsonArray selNodes = stamp.selection.value(QStringLiteral("nodes")).toArray();
        for (const QJsonValue& v : selNodes) {
            const quint64 id = static_cast<quint64>(v.toVariant().toULongLong());
            for (GraphicsNode* n : scene_->nodes()) {
                if (n->objectId() == id) {
                    n->setSelected(true);
                    break;
                }
            }
        }
        const QJsonArray selEdges = stamp.selection.value(QStringLiteral("edges")).toArray();
        for (const QJsonValue& v : selEdges) {
            const quint64 id = static_cast<quint64>(v.toVariant().toULongLong());
            for (GraphicsEdge* e : scene_->edges()) {
                if (e->objectId() == id) {
                    e->setSelected(true);
                    break;
                }
            }
        }
    }

    notifyModified();
    notifyRestored();
}

void SceneHistory::undo()
{
    if (!canUndo())
        return;
    --currentStep_;
    restoreHistoryStamp(stack_[static_cast<size_t>(currentStep_)]);
    scene_->setModified(true);
}

void SceneHistory::redo()
{
    if (!canRedo())
        return;
    ++currentStep_;
    restoreHistoryStamp(stack_[static_cast<size_t>(currentStep_)]);
    scene_->setModified(true);
}
