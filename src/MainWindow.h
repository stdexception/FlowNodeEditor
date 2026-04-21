#pragma once

#include <QMainWindow>

class QCloseEvent;

class FlowEditorWidget;
class NodeTypeRegistry;
class QTreeWidget;
class QTreeWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    void openDocumentPath(const QString &path);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onNew();
    void onOpen();
    void onSave();
    void onSaveAs();
    void onSaveGraph();
    void onAbout();

private:
    void buildPalette();
    void updateWindowTitle();
    bool maybeSave();
    QString definitionsRoot() const;

    FlowEditorWidget *m_editor = nullptr;
    NodeTypeRegistry *m_registry = nullptr;
    QTreeWidget *m_palette = nullptr;
};
