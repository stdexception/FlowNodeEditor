#include "MainWindow.h"

#include "FlowEditorScene.h"
#include "FlowEditorWidget.h"
#include "NodeTypeRegistry.h"

#include <QApplication>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDockWidget>
#include <QDrag>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QMouseEvent>
#include <QTextStream>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

namespace
{
const char *kNodeMime = "application/x-flow-node-type";

class PaletteTree : public QTreeWidget
{
public:
    explicit PaletteTree(QWidget *parent = nullptr)
        : QTreeWidget(parent)
    {
        setHeaderHidden(true);
        setRootIsDecorated(true);
        setMaximumWidth(260);
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

    m_editor = new FlowEditorWidget(m_registry, this);
    setCentralWidget(m_editor);

    m_palette = new PaletteTree(this);
    buildPalette();
    auto *dock = new QDockWidget(tr("Node library"), this);
    dock->setWidget(m_palette);
    addDockWidget(Qt::LeftDockWidgetArea, dock);

    auto *fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("&New"), this, &MainWindow::onNew, QKeySequence::New);
    fileMenu->addAction(tr("&Open..."), this, &MainWindow::onOpen, QKeySequence::Open);
    fileMenu->addAction(tr("&Save"), this, &MainWindow::onSave, QKeySequence::Save);
    fileMenu->addAction(tr("Save &As..."), this, &MainWindow::onSaveAs);
    fileMenu->addAction(tr("Save &Graph..."), this, &MainWindow::onSaveGraph);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("E&xit"), this, &QWidget::close, QKeySequence::Quit);

    auto *helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, &MainWindow::onAbout);

    const QString base = QCoreApplication::applicationDirPath();
    const QString qss = joinReadFiles(QStringList()
        << (base + QStringLiteral("/assets/qss/nodeeditor.qss"))
        << (base + QStringLiteral("/assets/qss/nodeeditor-light.qss")));
    if (!qss.isEmpty())
    {
        qApp->setStyleSheet(qss);
    }

    resize(1200, 800);
    updateWindowTitle();

    connect(m_editor->flowScene(), &FlowEditorScene::modificationChanged, this, [this](bool) {
        updateWindowTitle();
    });
}

QString MainWindow::definitionsRoot() const
{
    return QCoreApplication::applicationDirPath() + QStringLiteral("/execution_subsystem/node_type_definitions");
}

void MainWindow::buildPalette()
{
    m_palette->clear();
    for (const QString &cat : m_registry->categories())
    {
        auto *catItem = new QTreeWidgetItem(QStringList(cat));
        m_palette->addTopLevelItem(catItem);
        for (const NodeTypeInfo &info : m_registry->typesInCategory(cat))
        {
            auto *nodeItem = new QTreeWidgetItem(QStringList(info.nodeType));
            nodeItem->setData(0, Qt::UserRole, info.nodeType);
            catItem->addChild(nodeItem);
        }
        catItem->setExpanded(true);
    }
}

void MainWindow::updateWindowTitle()
{
    QString name = tr("Untitled");
    if (!m_editor->flowScene()->currentFilePath().isEmpty())
    {
        name = QFileInfo(m_editor->flowScene()->currentFilePath()).fileName();
    }
    const QString star = m_editor->isModified() ? QStringLiteral("*") : QString();
    setWindowTitle(star + name + QStringLiteral(" - FlowNodeEditor"));
}

bool MainWindow::maybeSave()
{
    if (!m_editor->isModified())
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
        onSave();
        return !m_editor->isModified();
    }
    if (ret == QMessageBox::Cancel)
    {
        return false;
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave())
    {
        event->accept();
    }
    else
    {
        event->ignore();
    }
}

void MainWindow::onNew()
{
    if (!maybeSave())
    {
        return;
    }
    m_editor->newDocument();
    updateWindowTitle();
}

void MainWindow::onOpen()
{
    if (!maybeSave())
    {
        return;
    }
    const QString path = QFileDialog::getOpenFileName(this, tr("Open scene"), QString(),
                                                      tr("Node editor scene (*.nes);;All files (*)"));
    if (path.isEmpty())
    {
        return;
    }
    if (m_editor->openDocument(path))
    {
        updateWindowTitle();
    }
    else
    {
        QMessageBox::warning(this, tr("FlowNodeEditor"), tr("Could not open the file."));
    }
}

void MainWindow::onSave()
{
    if (m_editor->flowScene()->currentFilePath().isEmpty())
    {
        onSaveAs();
        return;
    }
    if (m_editor->saveDocument(m_editor->flowScene()->currentFilePath()))
    {
        updateWindowTitle();
    }
}

void MainWindow::onSaveAs()
{
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
    if (m_editor->saveDocument(p))
    {
        updateWindowTitle();
    }
}

void MainWindow::onSaveGraph()
{
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
    if (!m_editor->saveGraphDocument(p))
    {
        QMessageBox::warning(this, tr("FlowNodeEditor"), tr("Could not save graph."));
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

void MainWindow::openDocumentPath(const QString &path)
{
    if (path.isEmpty())
    {
        return;
    }
    if (m_editor->openDocument(path))
    {
        updateWindowTitle();
    }
}
