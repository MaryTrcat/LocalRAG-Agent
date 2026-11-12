#include "localrag/ui/MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("LocalRAG Desktop"));
    QApplication::setOrganizationName(QStringLiteral("LocalRAG"));
    localrag::MainWindow window;
    window.show();
    return app.exec();
}
