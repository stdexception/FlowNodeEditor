#pragma once

#include <memory>
#include <vector>

#include <QJsonObject>
#include <QString>

#include "ObjectId.hpp"
#include "nodeeditor/Serializable.hpp"

class GraphicsScene;
class GraphicsNode;
class GraphicsEdge;
class GraphicsSocket;
class GraphicsView;
class SceneHistory;
class SceneClipboard;

class EditorScene : public nodeeditor::Serializable {
public:
    EditorScene();
    ~EditorScene() override;

    void setView(GraphicsView* v) { view_ = v; }
    [[nodiscard]] GraphicsView* view() const { return view_; }

    [[nodiscard]] GraphicsScene* graphicsScene() const { return grScene_.get(); }

    void addNode(GraphicsNode* node);
    void removeNode(GraphicsNode* node);
    void addEdge(GraphicsEdge* edge);
    void removeEdge(GraphicsEdge* edge);

    [[nodiscard]] const std::vector<GraphicsNode*>& nodes() const { return nodes_; }
    [[nodiscard]] const std::vector<GraphicsEdge*>& edges() const { return edges_; }

    void setModified(bool value);
    [[nodiscard]] bool isModified() const { return modified_; }

    [[nodiscard]] SceneHistory* history() const { return history_.get(); }
    [[nodiscard]] SceneClipboard* clipboard() const { return clipboard_.get(); }

    /** Full scene JSON (Python Scene.serialize shape). */
    [[nodiscard]] QJsonObject toJson() const;
    /** Replace scene content from JSON (no undo entry). */
    void fromJson(const QJsonObject& o);

    void clearContent();

    /** Python saveSceneToFile / loadFromFile subset (.nes). */
    bool saveToFile(const QString& path) const;
    bool loadFromFile(const QString& path);

    /** Python saveGraphToFile subset (execution exchange format). */
    bool saveGraphJson(const QString& path) const;

private:
    std::unique_ptr<GraphicsScene> grScene_;
    std::unique_ptr<SceneHistory> history_;
    std::unique_ptr<SceneClipboard> clipboard_;
    GraphicsView* view_{nullptr};

    std::vector<GraphicsNode*> nodes_;
    std::vector<GraphicsEdge*> edges_;
    bool modified_{false};
    int sceneWidth_{64000};
    int sceneHeight_{64000};
    quint64 sceneObjectId_{allocateObjectId()};
    bool inLoadBatch_{false};
};
