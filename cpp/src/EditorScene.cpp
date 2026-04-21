#include <algorithm>

#include "EditorScene.hpp"
#include "GraphicsScene.hpp"
#include "GraphicsNode.hpp"
#include "GraphicsEdge.hpp"

EditorScene::EditorScene()
{
    grScene_ = std::make_unique<GraphicsScene>(this);
    grScene_->setGridSceneSize(sceneWidth_, sceneHeight_);
}

EditorScene::~EditorScene()
{
    // Deleting an edge mutates edges_; copy before iterating.
    const std::vector<GraphicsEdge*> edgesCopy = edges_;
    for (GraphicsEdge* e : edgesCopy)
        delete e;
    edges_.clear();
    const std::vector<GraphicsNode*> nodesCopy = nodes_;
    for (GraphicsNode* n : nodesCopy)
        delete n;
    nodes_.clear();
}

void EditorScene::addNode(GraphicsNode* node)
{
    nodes_.push_back(node);
    setModified(true);
}

void EditorScene::removeNode(GraphicsNode* node)
{
    auto it = std::find(nodes_.begin(), nodes_.end(), node);
    if (it != nodes_.end())
        nodes_.erase(it);
    setModified(true);
}

void EditorScene::addEdge(GraphicsEdge* edge)
{
    edges_.push_back(edge);
    setModified(true);
}

void EditorScene::removeEdge(GraphicsEdge* edge)
{
    auto it = std::find(edges_.begin(), edges_.end(), edge);
    if (it != edges_.end())
        edges_.erase(it);
    setModified(true);
}

void EditorScene::setModified(bool value)
{
    modified_ = value;
}
