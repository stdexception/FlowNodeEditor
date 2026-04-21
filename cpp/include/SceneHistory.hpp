#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <functional>
#include <vector>

class EditorScene;

/**
 * C++ analogue of nodeeditor.node_scene_history.SceneHistory (subset).
 */
class SceneHistory {
public:
    explicit SceneHistory(EditorScene* scene);

    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;

    void undo();
    void redo();

    void clear();
    void storeInitialHistoryStamp();
    void storeHistory(const QString& desc, bool setModified = false);

    void addHistoryModifiedListener(std::function<void()> cb);
    void addHistoryStoredListener(std::function<void()> cb);
    void addHistoryRestoredListener(std::function<void()> cb);

private:
    struct Stamp {
        QString desc;
        QJsonObject snapshot;
        QJsonObject selection;
    };

    [[nodiscard]] QJsonObject captureCurrentSelection() const;
    [[nodiscard]] Stamp createHistoryStamp(const QString& desc) const;
    void restoreHistoryStamp(const Stamp& stamp);

    void notifyModified();
    void notifyStored();
    void notifyRestored();

    EditorScene* scene_{nullptr};
    std::vector<Stamp> stack_;
    int currentStep_{-1};
    int historyLimit_{32};

    std::vector<std::function<void()>> modifiedListeners_;
    std::vector<std::function<void()>> storedListeners_;
    std::vector<std::function<void()>> restoredListeners_;
};
