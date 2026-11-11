#include <QApplication>
#include <QPushButton>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

#include <iostream>

#include "MazeWidget.h"

// Usage: MazeSolver <size>   (size 5-100, default 15)
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    int size = 15;
    if (argc > 1) {
        bool ok = false;
        size = QString(argv[1]).toInt(&ok);
        if (!ok || size < 5 || size > 100) {
            std::cerr << "Usage: MazeSolver <size>\n"
                      << "  size must be a whole number from 5 to 100\n";
            return 1;
        }
    }

    QWidget window;
    window.setWindowTitle(QString("Maze Solver - %1 x %1").arg(size));

    auto* solveButton = new QPushButton("Solve");
    solveButton->setFocusPolicy(Qt::NoFocus); // keep arrow keys going to the maze
    auto* mazeWidget = new MazeWidget(size);

    auto* layout = new QVBoxLayout(&window);
    layout->addWidget(solveButton, 0, Qt::AlignLeft);
    layout->addWidget(mazeWidget, 1);

    QObject::connect(solveButton, &QPushButton::clicked, mazeWidget, [mazeWidget] {
        mazeWidget->solve();
    });

    window.resize(600, 650);
    window.show();
    mazeWidget->setFocus(); // arrow keys move the player right away

    return app.exec();
}
