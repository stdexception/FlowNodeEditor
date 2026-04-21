#include "MainWindow.hpp"
#include "EditorScene.hpp"
#include "GraphicsScene.hpp"
#include "GraphicsNode.hpp"
#include "GraphicsEdge.hpp"
#include "GraphicsView.hpp"

#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow()
{
    setWindowTitle(QStringLiteral("Flow Node Editor (C++)"));
    resize(1200, 800);
    setupEditor();

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(view_);
    setCentralWidget(central);

    connect(view_, &GraphicsView::scenePosChanged, this, [this](int x, int y) {
        statusBar()->showMessage(QStringLiteral("Scene: %1, %2").arg(x).arg(y));
    });
}

void MainWindow::setupEditor()
{
    scene_ = new EditorScene();
    view_ = new GraphicsView(scene_->graphicsScene(), this);
    view_->setScene(scene_->graphicsScene());

    auto* a = new GraphicsNode(scene_, QStringLiteral("Node A"));
    a->setPos(-200, 0);
    auto* b = new GraphicsNode(scene_, QStringLiteral("Node B"));
    b->setPos(200, 40);

    if (!a->outputSockets().isEmpty() && !b->inputSockets().isEmpty())
        new GraphicsEdge(scene_, a->outputSockets().first(), b->inputSockets().first());
}
