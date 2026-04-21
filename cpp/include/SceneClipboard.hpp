#pragma once

#include <QJsonObject>

class EditorScene;

/**
 * C++ analogue of nodeeditor.node_scene_clipboard.SceneClipboard (subset).
 */
class SceneClipboard {
public:
    explicit SceneClipboard(EditorScene* scene);

    /** Serialize selected nodes and edges between them (Python serializeSelected). */
    [[nodiscard]] QJsonObject serializeSelected(bool deleteAfter = false);

    /** Paste clipboard JSON at last mouse scene position (Python deserializeFromClipboard subset). */
    void deserializeFromClipboard(const QJsonObject& data);

private:
    EditorScene* scene_{nullptr};
};
