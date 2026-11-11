#include <QApplication>

#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Used by QSettings to store dark mode and recent games.
    QApplication::setOrganizationName("MazeSolver");
    QApplication::setApplicationName("MazeSolver");

    // Fusion follows the light/dark palettes set by MainWindow.
    QApplication::setStyle("Fusion");

    MainWindow window;
    window.show();

    return app.exec();
}
