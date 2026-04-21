#include "FlowEditorWidget.h"

#include "FlowEditorScene.h"
#include "FlowGraphicsView.h"
#include "NodeTypeRegistry.h"

#include <QVBoxLayout>

static const QString kNodeMime = QStringLiteral("application/x-flow-node-type");

FlowEditorWidget::FlowEditorWidget(NodeTypeRegistry *registry, QWidget *parent)
    : QWidget(parent)
{
    m_scene = new FlowEditorScene(this);
    m_scene->setRegistry(registry);

    m_view = new FlowGraphicsView(m_scene, this);
    auto *lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->addWidget(m_view);
}

bool FlowEditorWidget::isModified() const
{
    return m_scene && m_scene->isModified();
}

void FlowEditorWidget::newDocument()
{
    m_scene->clearDocument();
}

bool FlowEditorWidget::openDocument(const QString &path)
{
    return m_scene->loadSceneFile(path);
}

bool FlowEditorWidget::saveDocument(const QString &path)
{
    return m_scene->saveSceneFile(path);
}

bool FlowEditorWidget::saveGraphDocument(const QString &path)
{
    return m_scene->saveGraphFile(path);
}
