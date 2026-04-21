#pragma once

#include <memory>
#include <vector>

#include "nodeeditor/Serializable.hpp"

class GraphicsScene;
class GraphicsNode;
class GraphicsEdge;
class GraphicsSocket;

class EditorScene : public nodeeditor::Serializable {
public:
    EditorScene();
    ~EditorScene() override;

    [[nodiscard]] GraphicsScene* graphicsScene() const { return grScene_.get(); }

    void addNode(GraphicsNode* node);
    void removeNode(GraphicsNode* node);
    void addEdge(GraphicsEdge* edge);
    void removeEdge(GraphicsEdge* edge);

    [[nodiscard]] const std::vector<GraphicsNode*>& nodes() const { return nodes_; }
    [[nodiscard]] const std::vector<GraphicsEdge*>& edges() const { return edges_; }

    void setModified(bool value);
    [[nodiscard]] bool isModified() const { return modified_; }

private:
    std::unique_ptr<GraphicsScene> grScene_;
    std::vector<GraphicsNode*> nodes_;
    std::vector<GraphicsEdge*> edges_;
    bool modified_{false};
    int sceneWidth_{64000};
    int sceneHeight_{64000};
};
