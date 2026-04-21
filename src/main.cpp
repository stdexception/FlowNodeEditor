#include "MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QIcon>
#include <QStringList>


static void loadFontsFromAssets()
{
    const QString base = QApplication::applicationDirPath();
    const QDir fontDir(base + QStringLiteral("/assets/fonts"));
    if (!fontDir.exists())
    {
        return;
    }
    const QFileInfoList fonts = fontDir.entryInfoList(QStringList() << QStringLiteral("*.ttf") << QStringLiteral("*.otf"),
                                                      QDir::Files, QDir::Name);
    for (const QFileInfo &fi : fonts)
    {
        QFontDatabase::addApplicationFont(fi.absoluteFilePath());
    }
}

static void loadWindowIcons(QMainWindow *w)
{
    const QString base = QApplication::applicationDirPath() + QStringLiteral("/assets/icons");
    QIcon icon;
    const QList<QPair<QString, QSize>> sizes = {
        {QStringLiteral("16x16.png"), QSize(16, 16)},   {QStringLiteral("24x24.png"), QSize(24, 24)},
        {QStringLiteral("32x32.png"), QSize(32, 32)},   {QStringLiteral("48x48.png"), QSize(48, 48)},
        {QStringLiteral("64x64.png"), QSize(64, 64)},   {QStringLiteral("128x128.png"), QSize(128, 128)},
        {QStringLiteral("256x256.png"), QSize(256, 256)},
    };
    for (const auto &p : sizes)
    {
        const QString path = base + QLatin1Char('/') + p.first;
        if (QFileInfo::exists(path))
        {
            icon.addFile(path, p.second);
        }
    }
    if (!icon.isNull())
    {
        QApplication::setWindowIcon(icon);
        w->setWindowIcon(icon);
    }
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("FlowNodeEditor"));
    QApplication::setOrganizationName(QStringLiteral("FlowNodeEditor"));
    QApplication::setApplicationDisplayName(QStringLiteral("FlowNodeEditor"));
    QApplication::setStyle(QStringLiteral("Fusion"));

    loadFontsFromAssets();

    MainWindow w;
    loadWindowIcons(&w);
    w.show();

    const QStringList args = QCoreApplication::arguments();
    if (args.size() == 2)
    {
        w.openDocumentPath(args.at(1));
    }

    return app.exec();
}
