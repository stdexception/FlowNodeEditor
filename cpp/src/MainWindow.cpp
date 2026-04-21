#include "MainWindow.hpp"
#include "EditorScene.hpp"
#include "GraphicsScene.hpp"
#include "GraphicsNode.hpp"
#include "GraphicsEdge.hpp"
#include "GraphicsView.hpp"
#include "SceneHistory.hpp"
#include "SceneClipboard.hpp"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QJsonDocument>
#include <QKeySequence>
#include <QMenuBar>
#include <QMessageBox>
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

    auto* menuFile = menuBar()->addMenu(tr("&File"));
    auto* actNew = menuFile->addAction(tr("&New"));
    actNew->setShortcut(QKeySequence::New);
    connect(actNew, &QAction::triggered, this, &MainWindow::onFileNew);

    auto* actOpen = menuFile->addAction(tr("&Open…"));
    actOpen->setShortcut(QKeySequence::Open);
    connect(actOpen, &QAction::triggered, this, &MainWindow::onFileOpen);

    auto* actSave = menuFile->addAction(tr("&Save"));
    actSave->setShortcut(QKeySequence::Save);
    connect(actSave, &QAction::triggered, this, &MainWindow::onFileSave);

    auto* actSaveGraph = menuFile->addAction(tr("Save &Graph JSON…"));
    connect(actSaveGraph, &QAction::triggered, this, &MainWindow::onSaveGraphJson);

    menuFile->addSeparator();
    auto* actQuit = menuFile->addAction(tr("E&xit"));
    actQuit->setShortcut(QKeySequence::Quit);
    connect(actQuit, &QAction::triggered, this, &QWidget::close);

    auto* menuEdit = menuBar()->addMenu(tr("&Edit"));
    actUndo_ = menuEdit->addAction(tr("&Undo"));
    actUndo_->setShortcut(QKeySequence::Undo);
    connect(actUndo_, &QAction::triggered, this, &MainWindow::onUndo);

    actRedo_ = menuEdit->addAction(tr("&Redo"));
    actRedo_->setShortcut(QKeySequence::Redo);
    connect(actRedo_, &QAction::triggered, this, &MainWindow::onRedo);

    menuEdit->addSeparator();
    auto* actCopy = menuEdit->addAction(tr("&Copy"));
    actCopy->setShortcut(QKeySequence::Copy);
    connect(actCopy, &QAction::triggered, this, &MainWindow::onCopy);

    auto* actCut = menuEdit->addAction(tr("Cu&t"));
    actCut->setShortcut(QKeySequence::Cut);
    connect(actCut, &QAction::triggered, this, &MainWindow::onCut);

    auto* actPaste = menuEdit->addAction(tr("&Paste"));
    actPaste->setShortcut(QKeySequence::Paste);
    connect(actPaste, &QAction::triggered, this, &MainWindow::onPaste);

    menuEdit->addSeparator();
    auto* actDel = menuEdit->addAction(tr("&Delete"));
    actDel->setShortcut(QKeySequence::Delete);
    connect(actDel, &QAction::triggered, this, &MainWindow::onDelete);

    if (scene_->history()) {
        scene_->history()->addHistoryModifiedListener([this] { updateEditActions(); });
        scene_->history()->storeInitialHistoryStamp();
    }
    updateEditActions();
}

void MainWindow::updateEditActions()
{
    if (!scene_ || !scene_->history()) {
        actUndo_->setEnabled(false);
        actRedo_->setEnabled(false);
        return;
    }
    actUndo_->setEnabled(scene_->history()->canUndo());
    actRedo_->setEnabled(scene_->history()->canRedo());
}

void MainWindow::onFileNew()
{
    if (!scene_)
        return;
    if (scene_->history())
        scene_->history()->storeHistory(QStringLiteral("Before New"), false);
    scene_->clearContent();
    auto* a = new GraphicsNode(scene_, QStringLiteral("Node A"));
    a->setPos(-200, 0);
    auto* b = new GraphicsNode(scene_, QStringLiteral("Node B"));
    b->setPos(200, 40);
    if (!a->outputSockets().isEmpty() && !b->inputSockets().isEmpty())
        new GraphicsEdge(scene_, a->outputSockets().first(), b->inputSockets().first());
    if (scene_->history())
        scene_->history()->storeHistory(QStringLiteral("New"), true);
    currentFilePath_.clear();
    setWindowTitle(QStringLiteral("Flow Node Editor (C++) — Untitled"));
}

void MainWindow::onFileOpen()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Open scene"), QString(),
                                                      tr("Node Editor Scene (*.nes);;JSON (*.json);;All (*)"));
    if (path.isEmpty())
        return;
    if (!scene_->loadFromFile(path)) {
        QMessageBox::warning(this, tr("Open"), tr("Could not load file."));
        return;
    }
    currentFilePath_ = path;
    setWindowTitle(QStringLiteral("Flow Node Editor (C++) — %1").arg(path));
    if (scene_->history()) {
        scene_->history()->clear();
        scene_->history()->storeInitialHistoryStamp();
    }
    updateEditActions();
}

void MainWindow::onFileSave()
{
    QString path = currentFilePath_;
    if (path.isEmpty())
        path = QFileDialog::getSaveFileName(this, tr("Save scene"), QString(),
                                            tr("Node Editor Scene (*.nes);;JSON (*.json)"));
    if (path.isEmpty())
        return;
    if (!scene_->saveToFile(path)) {
        QMessageBox::warning(this, tr("Save"), tr("Could not save file."));
        return;
    }
    currentFilePath_ = path;
    scene_->setModified(false);
    setWindowTitle(QStringLiteral("Flow Node Editor (C++) — %1").arg(path));
}

void MainWindow::onSaveGraphJson()
{
    const QString path = QFileDialog::getSaveFileName(this, tr("Save graph JSON"), QString(),
                                                      tr("Graph (*.graph.json);;JSON (*.json)"));
    if (path.isEmpty())
        return;
    if (!scene_->saveGraphJson(path))
        QMessageBox::warning(this, tr("Save"), tr("Could not save graph."));
}

void MainWindow::onUndo()
{
    if (scene_->history())
        scene_->history()->undo();
}

void MainWindow::onRedo()
{
    if (scene_->history())
        scene_->history()->redo();
}

void MainWindow::onCopy()
{
    if (!scene_->clipboard())
        return;
    const QJsonObject data = scene_->clipboard()->serializeSelected(false);
    QApplication::clipboard()->setText(QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact)));
}

void MainWindow::onCut()
{
    if (!scene_->clipboard())
        return;
    const QJsonObject data = scene_->clipboard()->serializeSelected(true);
    QApplication::clipboard()->setText(QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact)));
    updateEditActions();
}

void MainWindow::onPaste()
{
    if (!scene_->clipboard())
        return;
    const QJsonObject data = QJsonDocument::fromJson(QApplication::clipboard()->text().toUtf8()).object();
    if (!data.isEmpty())
        scene_->clipboard()->deserializeFromClipboard(data);
    updateEditActions();
}

void MainWindow::onDelete()
{
    if (!view_)
        return;
    if (scene_->history())
        scene_->history()->storeHistory(QStringLiteral("Before delete"), false);
    view_->deleteSelectedItems();
    if (scene_->history())
        scene_->history()->storeHistory(QStringLiteral("Delete"), true);
    updateEditActions();
}

void MainWindow::setupEditor()
{
    scene_ = new EditorScene();
    view_ = new GraphicsView(scene_->graphicsScene(), this);
    view_->setEditorScene(scene_);
    scene_->setView(view_);
    view_->setScene(scene_->graphicsScene());

    auto* a = new GraphicsNode(scene_, QStringLiteral("Node A"));
    a->setPos(-200, 0);
    auto* b = new GraphicsNode(scene_, QStringLiteral("Node B"));
    b->setPos(200, 40);

    if (!a->outputSockets().isEmpty() && !b->inputSockets().isEmpty())
        new GraphicsEdge(scene_, a->outputSockets().first(), b->inputSockets().first());
}
