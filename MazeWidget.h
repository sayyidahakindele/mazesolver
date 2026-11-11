#pragma once

#include <QElapsedTimer>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <vector>

#include "MazeGenerator.h"
#include "MazeSolver.h"
#include "Player.h"

// Draws the maze and a player you move by clicking an adjacent cell, or with
// the arrow keys / WASD. The route walked so far is drawn as a line; moving
// back onto the previous cell removes the last step of the route.
class MazeWidget : public QWidget {
    Q_OBJECT

public:
    // Generates a random size x size maze once; it can't be changed afterwards.
    explicit MazeWidget(int size, QWidget* parent = nullptr);

    // Finds the shortest path from the player to the exit and walks the
    // player along it.
    void solve();

    int moves() const;
    qint64 elapsedMs() const;

    // Formats milliseconds as m:ss.t, e.g. 1:05.3
    static QString formatTime(qint64 ms);

signals:
    // Emitted after every move and about 10 times a second while the clock runs.
    void statsChanged(int moves, qint64 elapsedMs);

protected:
    void paintEvent(QPaintEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    struct Geometry {
        double left;
        double top;
        double cellSize;
    };
    Geometry geometry() const;

    bool hasWon() const;
    bool canAcceptInput() const;

    // Moves the player to an adjacent open cell, extending the route, or
    // shortening it if the cell is the one the player just came from.
    void moveTo(int row, int col);
    void stepAlongSolution();

    // Cell under a point in widget coordinates (may be outside the maze).
    Position cellAt(const QPointF& point) const;

    // Moves the player toward a neighbouring cell, including diagonal
    // neighbours (by going round the corner) when walls allow.
    void stepToward(const Position& target);

    // Moves the player through every cell the cursor passed over between
    // two mouse positions, so fast drags don't skip cells.
    void followCursor(const QPointF& from, const QPointF& to);

    const MazeGenerator m_maze;
    Player m_player;
    std::vector<Position> m_route; // cells walked from the start to the player

    bool m_dragging = false;
    QPointF m_lastMousePos;

    // Timer and move counter. The clock starts on the first move and stops at the exit.
    int m_moves = 0;
    bool m_clockStarted = false;
    QElapsedTimer m_clock;
    qint64 m_finishMs = 0;
    QTimer m_clockTick; // refreshes the displayed time
    bool m_usedSolve = false;

    QTimer m_solveTimer;
    std::vector<Position> m_solution; // shortest path found by the solver
    size_t m_solutionStep = 0;
};
