#include "MainWindow.h"

#include "FlowEditorScene.h"
#include "FlowEditorSceneHistory.h"
#include "FlowEditorWidget.h"
#include "FlowGraphicsView.h"
#include "NodeSettingsDock.h"
#include "NodeTypeRegistry.h"

#include <QAction>
#include <QClipboard>
#include <QGuiApplication>
#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDrag>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QProcess>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTextStream>
#include <QToolBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QVariant>
#include <QWidget>
#include <Qt>

namespace
{
const char *kNodeMime = "application/x-flow-node-type";

static QIcon standardIcon(QStyle::StandardPixmap which)
{
    if (QApplication *app = qobject_cast<QApplication *>(QCoreApplication::instance()))
    {
        if (QStyle *st = app->style())
        {
            return st->standardIcon(which);
        }
    }
    return QIcon();
}

class PaletteTree : public QTreeWidget
{
public:
    explicit PaletteTree(QWidget *parent = nullptr)
        : QTreeWidget(parent)
    {
        setHeaderHidden(true);
        setRootIsDecorated(true);
        setIndentation(18);
        setIconSize(QSize(18, 18));
        setMinimumWidth(200);
        setMaximumWidth(360);
        setUniformRowHeights(true);
        setAnimated(false);
    }

protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        m_pressPos = e->pos();
        QTreeWidget::mousePressEvent(e);
    }

    void mouseMoveEvent(QMouseEvent *e) override
    {
        if (!(e->buttons() & Qt::LeftButton))
        {
            QTreeWidget::mouseMoveEvent(e);
            return;
        }
        QTreeWidgetItem *it = itemAt(m_pressPos);
        if (!it || !it->parent())
        {
            QTreeWidget::mouseMoveEvent(e);
            return;
        }
        if ((e->pos() - m_pressPos).manhattanLength() < QApplication::startDragDistance())
        {
            QTreeWidget::mouseMoveEvent(e);
            return;
        }

        QDrag drag(this);
        auto *md = new QMimeData;
        md->setData(QString::fromLatin1(kNodeMime), it->data(0, Qt::UserRole).toString().toUtf8());
        drag.setMimeData(md);
        drag.exec(Qt::CopyAction);
    }

private:
    QPoint m_pressPos;
};

QString joinReadFiles(const QStringList &paths)
{
    QString combined;
    for (const QString &p : paths)
    {
        QFile f(p);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            continue;
        }
        QTextStream in(&f);
        combined += in.readAll();
        combined += QLatin1Char('\n');
    }
    return combined;
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_registry = new NodeTypeRegistry(this);
    m_registry->loadFromDirectory(definitionsRoot());

    m_mdi = new QMdiArea(this);
    m_mdi->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_mdi->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_mdi->setViewMode(QMdiArea::TabbedView);
    m_mdi->setDocumentMode(true);
    m_mdi->setTabsClosable(true);
    m_mdi->setTabsMovable(true);
    connect(m_mdi, &QMdiArea::subWindowActivated, this, &MainWindow::onSubWindowActivated);

    m_palette = new PaletteTree(this);
    m_palette->setFont(QFont(QStringLiteral("Roboto"), 10));

    m_settingsDock = new NodeSettingsDock(this);

    m_sideTabs = new QTabWidget(this);
    m_sideTabs->setFont(QFont(QStringLiteral("Roboto"), 10));
    m_sideTabs->addTab(m_palette, tr("NODES"));
    m_sideTabs->addTab(m_settingsDock, tr("Settings"));

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->setChildrenCollapsible(false);
    m_mainSplitter->addWidget(m_mdi);
    m_mainSplitter->addWidget(m_sideTabs);
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 0);
    m_mainSplitter->setSizes({900, 360});
    setCentralWidget(m_mainSplitter);

    buildPalette();

    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New"), this, &MainWindow::onNew, QKeySequence::New);
    fileMenu->addAction(tr("&Open..."), this, &MainWindow::onOpen, QKeySequence::Open);
    fileMenu->addAction(tr("&Save"), this, &MainWindow::onSave, QKeySequence::Save);
    fileMenu->addAction(tr("Save &As..."), this, &MainWindow::onSaveAs);
    fileMenu->addAction(tr("Save &Graph..."), this, &MainWindow::onSaveGraph);
    fileMenu->addSeparator();
    QAction *runAct = fileMenu->addAction(tr("&Run graph"), this, &MainWindow::onRunGraph);
    runAct->setShortcut(Qt::Key_F5);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), this, &QWidget::close, QKeySequence::Quit);

    auto *editMenu = menuBar()->addMenu(tr("&Edit"));
    m_actUndo = editMenu->addAction(tr("&Undo"), this, &MainWindow::onUndo, QKeySequence::Undo);
    m_actRedo = editMenu->addAction(tr("&Redo"), this, &MainWindow::onRedo, QKeySequence::Redo);
    editMenu->addSeparator();
    m_actCut = editMenu->addAction(tr("Cu&t"), this, &MainWindow::onCut, QKeySequence::Cut);
    m_actCopy = editMenu->addAction(tr("&Copy"), this, &MainWindow::onCopy, QKeySequence::Copy);
    m_actPaste = editMenu->addAction(tr("&Paste"), this, &MainWindow::onPaste, QKeySequence::Paste);
    editMenu->addSeparator();
    m_actDelete = editMenu->addAction(tr("&Delete"), this, &MainWindow::onDelete, QKeySequence::Delete);

    auto *windowMenu = menuBar()->addMenu(tr("&Window"));
    windowMenu->addAction(tr("&Tile"), m_mdi, &QMdiArea::tileSubWindows);
    windowMenu->addAction(tr("&Cascade"), m_mdi, &QMdiArea::cascadeSubWindows);
    windowMenu->addSeparator();
    windowMenu->addAction(tr("Close &All"), m_mdi, &QMdiArea::closeAllSubWindows);

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, &MainWindow::onAbout);

    m_scenePosLabel = new QLabel(this);
    statusBar()->addPermanentWidget(m_scenePosLabel);

    // Toolbar parity with the old Python UI (Run Graph button)
    auto *toolbar = new QToolBar(tr("Main Toolbar"), this);
    toolbar->setFloatable(false);
    toolbar->setMovable(false);
    const QString playIcon = QCoreApplication::applicationDirPath() + QStringLiteral("/assets/icons/play.png");
    QAction *runToolbar = toolbar->addAction(QIcon(playIcon), tr("Run Graph (F5)"));
    runToolbar->setShortcut(Qt::Key_F5);
    connect(runToolbar, &QAction::triggered, this, &MainWindow::onRunGraph);
    addToolBar(toolbar);

    const QString base = QCoreApplication::applicationDirPath();
    const QString qss = joinReadFiles(QStringList()
        << (base + QStringLiteral("/assets/qss/nodeeditor.qss"))
        << (base + QStringLiteral("/assets/qss/nodeeditor-light.qss")));
    if (!qss.isEmpty())
    {
        qApp->setStyleSheet(qss);
    }

    readSettings();
    createMdiChild();
    if (FlowEditorWidget *ed = activeEditor())
    {
        ed->newDocument();
    }
    resize(1200, 800);
    updateWindowTitle();
    updateMenus();
    updateSettingsDock();

    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::updateMenus);
}

QString MainWindow::definitionsRoot() const
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/execution_subsystem/node_type_definitions");
}

QString MainWindow::executionRunnerPath() const
{
    const QString base = QCoreApplication::applicationDirPath() + QStringLiteral("/execution_subsystem/");
#ifdef Q_OS_WIN
    return base + QStringLiteral("run_graph.exe");
#else
    return base + QStringLiteral("run_graph");
#endif
}

void MainWindow::readSettings()
{
    QSettings s;
    if (s.contains(QStringLiteral("geometry")))
    {
        restoreGeometry(s.value(QStringLiteral("geometry")).toByteArray());
    }
    if (s.contains(QStringLiteral("windowState")))
    {
        restoreState(s.value(QStringLiteral("windowState")).toByteArray());
    }
    if (m_mainSplitter && s.contains(QStringLiteral("mainSplitterSizes")))
    {
        const QList<QVariant> v = s.value(QStringLiteral("mainSplitterSizes")).toList();
        QList<int> sizes;
        sizes.reserve(v.size());
        for (const QVariant &x : v)
        {
            sizes.append(x.toInt());
        }
        if (sizes.size() == m_mainSplitter->count())
        {
            m_mainSplitter->setSizes(sizes);
        }
    }
}

void MainWindow::writeSettings()
{
    QSettings s;
    s.setValue(QStringLiteral("geometry"), saveGeometry());
    s.setValue(QStringLiteral("windowState"), saveState());
    if (m_mainSplitter)
    {
        QList<QVariant> v;
        for (int sz : m_mainSplitter->sizes())
        {
            v.append(sz);
        }
        s.setValue(QStringLiteral("mainSplitterSizes"), v);
    }
}

FlowEditorWidget *MainWindow::activeEditor() const
{
    QMdiSubWindow *sw = m_mdi->activeSubWindow();
    if (!sw)
    {
        return nullptr;
    }
    return qobject_cast<FlowEditorWidget *>(sw->widget());
}

void MainWindow::createMdiChild(FlowEditorWidget *reuse)
{
    auto *ed = reuse ? reuse : new FlowEditorWidget(m_registry, m_mdi);
    auto *sub = m_mdi->addSubWindow(ed);
    sub->setAttribute(Qt::WA_DeleteOnClose, true);
    sub->setWindowTitle(ed->userFriendlyFileName());
    // Remove the native OS caption/title bar on MDI children. Document switching stays on QMdiArea's tab bar.
    sub->setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);

    connect(ed->flowScene(), &FlowEditorScene::modificationChanged, this, &MainWindow::updateWindowTitle);
    connect(ed->flowScene(), &FlowEditorScene::modificationChanged, sub, [sub, ed]() {
        const QString star = ed->isModified() ? QStringLiteral("*") : QString();
        sub->setWindowTitle(star + QFileInfo(ed->userFriendlyFileName()).fileName());
    });
    connect(ed->flowScene()->history(), &FlowEditorSceneHistory::historyModified, this, &MainWindow::updateMenus);
    connect(ed->graphicsView(), &FlowGraphicsView::sceneCursorPosChanged, this, &MainWindow::onSceneCursor);

    m_mdi->setActiveSubWindow(sub);
    sub->show();
}

void MainWindow::onSubWindowActivated(QMdiSubWindow *w)
{
    Q_UNUSED(w);
    updateSettingsDock();
    updateMenus();
}

void MainWindow::buildPalette()
{
    m_palette->clear();
    const QIcon folderIcon = standardIcon(QStyle::SP_DirIcon);
    const QIcon nodeIcon = standardIcon(QStyle::SP_FileIcon);
    for (const QString &cat : m_registry->categories())
    {
        auto *catItem = new QTreeWidgetItem(QStringList(cat));
        if (!folderIcon.isNull())
        {
            catItem->setIcon(0, folderIcon);
        }
        m_palette->addTopLevelItem(catItem);
        for (const NodeTypeInfo &info : m_registry->typesInCategory(cat))
        {
            auto *nodeItem = new QTreeWidgetItem(QStringList(info.nodeType));
            nodeItem->setData(0, Qt::UserRole, info.nodeType);
            if (!nodeIcon.isNull())
            {
                nodeItem->setIcon(0, nodeIcon);
            }
            catItem->addChild(nodeItem);
        }
        catItem->setExpanded(true);
    }
}

void MainWindow::updateWindowTitle()
{
    setWindowTitle(QStringLiteral("FlowNodeEditor"));
}

void MainWindow::updateMenus()
{
    FlowEditorWidget *ed = activeEditor();
    const bool has = ed != nullptr;
    const QMimeData *md = QGuiApplication::clipboard()->mimeData();
    const bool canPaste =
        has && md && md->hasFormat(QStringLiteral("application/x-flownodeeditor-clipboard"));

    if (m_actUndo)
    {
        m_actUndo->setEnabled(has && ed->canUndo());
    }
    if (m_actRedo)
    {
        m_actRedo->setEnabled(has && ed->canRedo());
    }
    if (m_actCut)
    {
        m_actCut->setEnabled(has && ed->hasSelectedItems());
    }
    if (m_actCopy)
    {
        m_actCopy->setEnabled(has && ed->hasSelectedItems());
    }
    if (m_actPaste)
    {
        m_actPaste->setEnabled(canPaste);
    }
    if (m_actDelete)
    {
        m_actDelete->setEnabled(has && ed->hasSelectedItems());
    }
}

void MainWindow::updateSettingsDock()
{
    FlowEditorWidget *ed = activeEditor();
    FlowEditorScene *desired = ed ? ed->flowScene() : nullptr;
    if (m_settingsDock->scene() != desired)
    {
        m_settingsDock->setScene(desired);
    }
    m_settingsDock->refreshFromSelection();
}

void MainWindow::onSceneCursor(int x, int y)
{
    m_scenePosLabel->setText(tr("Scene: [%1, %2]").arg(x).arg(y));
}

bool MainWindow::maybeSaveEditor(FlowEditorWidget *ed)
{
    if (!ed || !ed->isModified())
    {
        return true;
    }
    const QMessageBox::StandardButton ret = QMessageBox::warning(
        this,
        tr("FlowNodeEditor"),
        tr("The document has been modified.\nDo you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save)
    {
        if (ed->flowScene()->currentFilePath().isEmpty())
        {
            const QString path = QFileDialog::getSaveFileName(this, tr("Save scene"), QString(),
                                                              tr("Node editor scene (*.nes);;All files (*)"));
            if (path.isEmpty())
            {
                return false;
            }
            QString p = path;
            if (!p.endsWith(QStringLiteral(".nes"), Qt::CaseInsensitive))
            {
                p += QStringLiteral(".nes");
            }
            return ed->saveDocument(p);
        }
        return ed->saveDocument(ed->flowScene()->currentFilePath());
    }
    if (ret == QMessageBox::Cancel)
    {
        return false;
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    const QList<QMdiSubWindow *> windows = m_mdi->subWindowList();
    for (QMdiSubWindow *sw : windows)
    {
        m_mdi->setActiveSubWindow(sw);
        auto *ed = qobject_cast<FlowEditorWidget *>(sw->widget());
        if (!maybeSaveEditor(ed))
        {
            event->ignore();
            return;
        }
    }
    writeSettings();
    event->accept();
}

void MainWindow::onNew()
{
    if (!maybeSaveEditor(activeEditor()))
    {
        return;
    }
    createMdiChild();
    if (FlowEditorWidget *ed = activeEditor())
    {
        ed->newDocument();
    }
}

void MainWindow::onOpen()
{
    if (!maybeSaveEditor(activeEditor()))
    {
        return;
    }
    const QString path = QFileDialog::getOpenFileName(this, tr("Open scene"), QString(),
                                                      tr("Node editor scene (*.nes);;All files (*)"));
    if (path.isEmpty())
    {
        return;
    }
    for (QMdiSubWindow *sw : m_mdi->subWindowList())
    {
        auto *ed = qobject_cast<FlowEditorWidget *>(sw->widget());
        if (ed && ed->flowScene()->currentFilePath() == path)
        {
            m_mdi->setActiveSubWindow(sw);
            return;
        }
    }
    createMdiChild();
    if (FlowEditorWidget *ed = activeEditor())
    {
        if (!ed->openDocument(path))
        {
            QMessageBox::warning(this, tr("FlowNodeEditor"), tr("Could not open the file."));
            ed->parentWidget()->close();
        }
    }
}

void MainWindow::onSave()
{
    FlowEditorWidget *ed = activeEditor();
    if (!ed)
    {
        return;
    }
    if (ed->flowScene()->currentFilePath().isEmpty())
    {
        onSaveAs();
        return;
    }
    ed->saveDocument(ed->flowScene()->currentFilePath());
}

void MainWindow::onSaveAs()
{
    FlowEditorWidget *ed = activeEditor();
    if (!ed)
    {
        return;
    }
    const QString path = QFileDialog::getSaveFileName(this, tr("Save scene"), QString(),
                                                      tr("Node editor scene (*.nes);;All files (*)"));
    if (path.isEmpty())
    {
        return;
    }
    QString p = path;
    if (!p.endsWith(QStringLiteral(".nes"), Qt::CaseInsensitive))
    {
        p += QStringLiteral(".nes");
    }
    ed->saveDocument(p);
}

void MainWindow::onSaveGraph()
{
    FlowEditorWidget *ed = activeEditor();
    if (!ed)
    {
        return;
    }
    const QString path = QFileDialog::getSaveFileName(this, tr("Save graph"), QString(),
                                                      tr("Graph JSON (*.graph.json);;All files (*)"));
    if (path.isEmpty())
    {
        return;
    }
    QString p = path;
    if (!p.endsWith(QStringLiteral(".graph.json"), Qt::CaseInsensitive))
    {
        p += QStringLiteral(".graph.json");
    }
    if (!ed->saveGraphDocument(p))
    {
        QMessageBox::warning(this, tr("FlowNodeEditor"), tr("Could not save graph."));
    }
}

void MainWindow::onRunGraph()
{
    FlowEditorWidget *ed = activeEditor();
    if (!ed)
    {
        return;
    }
    QString graphPath;
    if (!ed->flowScene()->saveGraphToTempFile(&graphPath))
    {
        QMessageBox::warning(this, tr("Run graph"), tr("Could not export graph."));
        return;
    }
    const QString runner = executionRunnerPath();
    if (!QFileInfo::exists(runner))
    {
        QMessageBox::warning(
            this,
            tr("Run graph"),
            tr("Runner not found:\n%1").arg(runner));
        return;
    }
    const bool ok = QProcess::startDetached(runner, QStringList() << graphPath);
    if (!ok)
    {
        QMessageBox::warning(this, tr("Run graph"), tr("Failed to start runner."));
    }
}

void MainWindow::onAbout()
{
    QMessageBox::about(
        this,
        tr("About FlowNodeEditor"),
        tr("FlowNodeEditor - C++/Qt node graph editor.\n"
           "Node definitions are loaded from JSON next to the executable."));
}

void MainWindow::onUndo()
{
    if (FlowEditorWidget *ed = activeEditor())
    {
        ed->undo();
    }
}

void MainWindow::onRedo()
{
    if (FlowEditorWidget *ed = activeEditor())
    {
        ed->redo();
    }
}

void MainWindow::onCut()
{
    if (FlowEditorWidget *ed = activeEditor())
    {
        ed->cut();
    }
}

void MainWindow::onCopy()
{
    if (FlowEditorWidget *ed = activeEditor())
    {
        ed->copy();
    }
}

void MainWindow::onPaste()
{
    if (FlowEditorWidget *ed = activeEditor())
    {
        ed->paste();
    }
}

void MainWindow::onDelete()
{
    if (FlowEditorWidget *ed = activeEditor())
    {
        ed->deleteSelected();
    }
}

void MainWindow::openDocumentPath(const QString &path)
{
    if (path.isEmpty())
    {
        return;
    }
    for (QMdiSubWindow *sw : m_mdi->subWindowList())
    {
        auto *ed = qobject_cast<FlowEditorWidget *>(sw->widget());
        if (ed && ed->flowScene()->currentFilePath() == path)
        {
            m_mdi->setActiveSubWindow(sw);
            return;
        }
    }
    const QList<QMdiSubWindow *> wlist = m_mdi->subWindowList();
    if (wlist.size() == 1)
    {
        auto *ed = qobject_cast<FlowEditorWidget *>(wlist.first()->widget());
        if (ed && ed->flowScene()->currentFilePath().isEmpty() && !ed->isModified())
        {
            if (ed->openDocument(path))
            {
                return;
            }
        }
    }
    createMdiChild();
    if (FlowEditorWidget *ed = activeEditor())
    {
        ed->openDocument(path);
    }
}
