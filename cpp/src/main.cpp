#include "MainWindow.hpp"

#include <QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("FlowNodeEditor"));
    QApplication::setOrganizationName(QStringLiteral("FlowNodeEditor"));

    MainWindow w;
    w.show();
    return app.exec();
}
