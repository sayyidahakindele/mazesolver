#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
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

    auto* statsLabel = new QLabel;
    auto showStats = [statsLabel](int moves, qint64 ms) {
        statsLabel->setText(QString("Time: %1     Moves: %2").arg(MazeWidget::formatTime(ms)).arg(moves));
    };
    showStats(0, 0);

    auto* topRow = new QHBoxLayout;
    topRow->addWidget(solveButton);
    topRow->addSpacing(16);
    topRow->addWidget(statsLabel);
    topRow->addStretch();

    auto* layout = new QVBoxLayout(&window);
    layout->addLayout(topRow);
    layout->addWidget(mazeWidget, 1);

    QObject::connect(solveButton, &QPushButton::clicked, mazeWidget, [mazeWidget] {
        mazeWidget->solve();
    });
    QObject::connect(mazeWidget, &MazeWidget::statsChanged, statsLabel, showStats);

    window.resize(600, 650);
    window.show();
    mazeWidget->setFocus(); // arrow keys move the player right away

    return app.exec();
}
