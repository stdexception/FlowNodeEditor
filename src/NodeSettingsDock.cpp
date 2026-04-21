#include "NodeSettingsDock.h"

#include "FlowEditorScene.h"
#include "FlowEditorSceneHistory.h"
#include "FlowNodeItem.h"

#include <QGraphicsItem>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

NodeSettingsDock::NodeSettingsDock(QWidget *parent)
    : QWidget(parent)
{
    auto *lay = new QVBoxLayout(this);
    lay->addWidget(new QLabel(tr("Node settings (JSON):"), this));
    m_edit = new QPlainTextEdit(this);
    m_edit->setPlaceholderText(tr("{ }"));
    lay->addWidget(m_edit, 1);
    auto *btnRow = new QHBoxLayout;
    auto *apply = new QPushButton(tr("Apply"), this);
    connect(apply, &QPushButton::clicked, this, &NodeSettingsDock::onApply);
    btnRow->addStretch();
    btnRow->addWidget(apply);
    lay->addLayout(btnRow);
}

void NodeSettingsDock::setScene(FlowEditorScene *scene)
{
    if (m_scene)
    {
        disconnect(m_scene, nullptr, this, nullptr);
    }
    m_scene = scene;
    m_target = nullptr;
    m_edit->clear();
    if (m_scene)
    {
        connect(m_scene, &FlowEditorScene::selectionChangedInScene, this, &NodeSettingsDock::refreshFromSelection);
    }
}

void NodeSettingsDock::refreshFromSelection()
{
    m_target = nullptr;
    m_edit->clear();
    if (!m_scene)
    {
        return;
    }
    const QList<QGraphicsItem *> sel = m_scene->selectedItems();
    if (sel.size() != 1)
    {
        return;
    }
    QGraphicsItem *gi = sel.first();
    if (gi->type() != FlowNodeItem::Type)
    {
        return;
    }
    m_target = static_cast<FlowNodeItem *>(gi);
    const QJsonDocument doc(m_target->settings());
    m_edit->setPlainText(QString::fromUtf8(doc.toJson(QJsonDocument::Indented)));
}

void NodeSettingsDock::onApply()
{
    if (!m_scene || !m_target)
    {
        return;
    }
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(m_edit->toPlainText().toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
    {
        QMessageBox::warning(this, tr("Settings"), tr("Invalid JSON: %1").arg(err.errorString()));
        return;
    }
    m_target->setSettings(doc.object());
    m_scene->history()->storeHistory(QStringLiteral("Edit node settings"), true);
}
