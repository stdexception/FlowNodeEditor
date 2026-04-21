#pragma once

#include <QString>
#include <QMainWindow>

class QAction;
class EditorScene;
class GraphicsView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();

private slots:
    void onFileNew();
    void onFileOpen();
    void onFileSave();
    void onSaveGraphJson();
    void onUndo();
    void onRedo();
    void onCopy();
    void onCut();
    void onPaste();
    void onDelete();

private:
    void setupEditor();
    void updateEditActions();

    EditorScene* scene_{nullptr};
    GraphicsView* view_{nullptr};
    QString currentFilePath_;
    QAction* actUndo_{nullptr};
    QAction* actRedo_{nullptr};
};
