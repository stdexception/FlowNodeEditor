#pragma once

#include <QWidget>

class FlowEditorScene;
class FlowGraphicsView;
class NodeTypeRegistry;

class FlowEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FlowEditorWidget(NodeTypeRegistry *registry, QWidget *parent = nullptr);

    FlowEditorScene *flowScene() const
    {
        return m_scene;
    }

    FlowGraphicsView *graphicsView() const
    {
        return m_view;
    }

    bool isModified() const;

    bool hasSelectedItems() const;
    bool canUndo() const;
    bool canRedo() const;

    void newDocument();
    bool openDocument(const QString &path);
    bool saveDocument(const QString &path);
    bool saveGraphDocument(const QString &path);

    QString userFriendlyFileName() const;

    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void deleteSelected();

private:
    FlowEditorScene *m_scene = nullptr;
    FlowGraphicsView *m_view = nullptr;
};
