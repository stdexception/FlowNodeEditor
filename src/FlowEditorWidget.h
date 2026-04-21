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

    bool isModified() const;

    void newDocument();
    bool openDocument(const QString &path);
    bool saveDocument(const QString &path);
    bool saveGraphDocument(const QString &path);

    FlowGraphicsView *graphicsView() const
    {
        return m_view;
    }

private:
    FlowEditorScene *m_scene = nullptr;
    FlowGraphicsView *m_view = nullptr;
};
