#pragma once

#include <QMainWindow>

class QCloseEvent;
class QMdiArea;
class QMdiSubWindow;
class FlowEditorWidget;
class NodeTypeRegistry;
class QTreeWidget;
class NodeSettingsDock;
class QLabel;
class QAction;
class QSplitter;
class QTabWidget;

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
    void onRunGraph();
    void onAbout();
    void onUndo();
    void onRedo();
    void onCut();
    void onCopy();
    void onPaste();
    void onDelete();
    void onSubWindowActivated(QMdiSubWindow *w);
    void updateMenus();
    void updateWindowTitle();
    void updateSettingsDock();
    void onSceneCursor(int x, int y);

private:
    FlowEditorWidget *activeEditor() const;
    bool maybeSaveEditor(FlowEditorWidget *ed);
    void createMdiChild(FlowEditorWidget *reuse = nullptr);
    void buildPalette();
    QString definitionsRoot() const;
    QString executionRunnerPath() const;
    void readSettings();
    void writeSettings();

    QMdiArea *m_mdi = nullptr;
    NodeTypeRegistry *m_registry = nullptr;
    QTreeWidget *m_palette = nullptr;
    NodeSettingsDock *m_settingsDock = nullptr;
    QSplitter *m_mainSplitter = nullptr;
    QTabWidget *m_sideTabs = nullptr;
    QLabel *m_scenePosLabel = nullptr;

    QAction *m_actUndo = nullptr;
    QAction *m_actRedo = nullptr;
    QAction *m_actCut = nullptr;
    QAction *m_actCopy = nullptr;
    QAction *m_actPaste = nullptr;
    QAction *m_actDelete = nullptr;
};
