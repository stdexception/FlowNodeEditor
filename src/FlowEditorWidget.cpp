#include "FlowEditorWidget.h"

#include "FlowEdgeItem.h"
#include "FlowEditorScene.h"
#include "FlowEditorSceneHistory.h"
#include "FlowGraphicsView.h"
#include "FlowNodeItem.h"
#include "NodeTypeRegistry.h"

#include <QClipboard>
#include <QGraphicsItem>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QMimeData>
#include <QVBoxLayout>

static const char kClipboardMime[] = "application/x-flownodeeditor-clipboard";

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

bool FlowEditorWidget::hasSelectedItems() const
{
    if (!m_scene)
    {
        return false;
    }
    const QList<QGraphicsItem *> sel = m_scene->selectedItems();
    return !sel.isEmpty();
}

bool FlowEditorWidget::canUndo() const
{
    return m_scene && m_scene->history() && m_scene->history()->canUndo();
}

bool FlowEditorWidget::canRedo() const
{
    return m_scene && m_scene->history() && m_scene->history()->canRedo();
}

void FlowEditorWidget::newDocument()
{
    m_scene->clearDocument();
    m_scene->history()->storeInitialHistoryStamp();
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

QString FlowEditorWidget::userFriendlyFileName() const
{
    if (!m_scene->currentFilePath().isEmpty())
    {
        return m_scene->currentFilePath();
    }
    return tr("Untitled");
}

void FlowEditorWidget::undo()
{
    if (m_scene->history())
    {
        m_scene->history()->undo();
    }
}

void FlowEditorWidget::redo()
{
    if (m_scene->history())
    {
        m_scene->history()->redo();
    }
}

void FlowEditorWidget::cut()
{
    const QJsonObject data = m_scene->serializeClipboardSelection(true);
    auto *mime = new QMimeData;
    mime->setData(QString::fromLatin1(kClipboardMime), QJsonDocument(data).toJson(QJsonDocument::Compact));
    QGuiApplication::clipboard()->setMimeData(mime);
}

void FlowEditorWidget::copy()
{
    const QJsonObject data = m_scene->serializeClipboardSelection(false);
    auto *mime = new QMimeData;
    mime->setData(QString::fromLatin1(kClipboardMime), QJsonDocument(data).toJson(QJsonDocument::Compact));
    QGuiApplication::clipboard()->setMimeData(mime);
}

void FlowEditorWidget::paste()
{
    const QMimeData *md = QGuiApplication::clipboard()->mimeData();
    if (!md || !md->hasFormat(QString::fromLatin1(kClipboardMime)))
    {
        return;
    }
    const QJsonDocument doc = QJsonDocument::fromJson(md->data(QString::fromLatin1(kClipboardMime)));
    if (!doc.isObject())
    {
        return;
    }
    m_scene->pasteFromClipboardJson(doc.object(), m_view->lastSceneMousePos());
}

void FlowEditorWidget::deleteSelected()
{
    if (!hasSelectedItems())
    {
        return;
    }
    m_scene->beginBulkEdit();
    const QList<QGraphicsItem *> sel = m_scene->selectedItems();
    for (QGraphicsItem *gi : sel)
    {
        if (gi->type() == FlowEdgeItem::Type)
        {
            m_scene->removeEdge(static_cast<FlowEdgeItem *>(gi));
        }
    }
    for (QGraphicsItem *gi : sel)
    {
        if (gi->type() == FlowNodeItem::Type)
        {
            m_scene->removeNode(static_cast<FlowNodeItem *>(gi));
        }
    }
    m_scene->endBulkEdit();
    m_scene->history()->storeHistory(QStringLiteral("Delete"), true);
}
