#pragma once

#include <QMainWindow>

class EditorScene;
class GraphicsView;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow();

private:
    void setupEditor();

    EditorScene* scene_{nullptr};
    GraphicsView* view_{nullptr};
};
