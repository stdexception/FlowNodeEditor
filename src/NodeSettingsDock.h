#pragma once

#include <QWidget>

class FlowEditorScene;
class FlowNodeItem;
class QPlainTextEdit;

class NodeSettingsDock : public QWidget
{
    Q_OBJECT

public:
    explicit NodeSettingsDock(QWidget *parent = nullptr);

    void setScene(FlowEditorScene *scene);
    FlowEditorScene *scene() const
    {
        return m_scene;
    }

public slots:
    void refreshFromSelection();

private slots:
    void onApply();

private:
    FlowEditorScene *m_scene = nullptr;
    QPlainTextEdit *m_edit = nullptr;
    FlowNodeItem *m_target = nullptr;
};
