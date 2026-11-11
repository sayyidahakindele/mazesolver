#include "MazeWidget.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPolygonF>
#include <algorithm>
#include <cmath>

static MazeGenerator makeMaze(int size)
{
    MazeGenerator maze(size);
    maze.generate();
    return maze;
}

MazeWidget::MazeWidget(int size, QWidget* parent)
    : QWidget(parent)
    , m_maze(makeMaze(size))
    , m_route{{0, 0}}
{
    setMinimumSize(200, 200);
    setFocusPolicy(Qt::StrongFocus); // so arrow keys reach this widget

    m_solveTimer.setInterval(60); // ms per step while auto-solving
    connect(&m_solveTimer, &QTimer::timeout, this, [this] { stepAlongSolution(); });
}

MazeWidget::Geometry MazeWidget::geometry() const
{
    const int n = m_maze.size();
    const double margin = 10.0;
    const double cellSize = (std::min(width(), height()) - 2 * margin) / n;
    return {(width() - cellSize * n) / 2.0, (height() - cellSize * n) / 2.0, cellSize};
}

bool MazeWidget::hasWon() const
{
    const int last = m_maze.size() - 1;
    return m_player.row() == last && m_player.col() == last;
}

bool MazeWidget::canAcceptInput() const
{
    return !hasWon() && !m_solveTimer.isActive();
}

void MazeWidget::moveTo(int row, int col)
{
    if (!m_player.canMoveTo(m_maze, row, col)) {
        return;
    }

    // Stepping back onto the previous cell undoes the last step.
    if (m_route.size() >= 2 && m_route[m_route.size() - 2] == Position{row, col}) {
        m_route.pop_back();
    } else {
        m_route.push_back({row, col});
    }

    m_player.setPosition(row, col);
    update();
}

void MazeWidget::solve()
{
    if (!canAcceptInput()) {
        return;
    }

    const int last = m_maze.size() - 1;
    m_solution = MazeSolver::shortestPath(m_maze, {m_player.row(), m_player.col()}, {last, last});
    m_solutionStep = 0;

    if (!m_solution.empty()) {
        m_solveTimer.start();
    }
}

void MazeWidget::stepAlongSolution()
{
    if (m_solutionStep + 1 >= m_solution.size()) {
        m_solveTimer.stop();
        return;
    }

    ++m_solutionStep;
    const Position& p = m_solution[m_solutionStep];
    moveTo(p.first, p.second);
}

Position MazeWidget::cellAt(const QPointF& point) const
{
    const Geometry g = geometry();
    const int col = static_cast<int>(std::floor((point.x() - g.left) / g.cellSize));
    const int row = static_cast<int>(std::floor((point.y() - g.top) / g.cellSize));
    return {row, col};
}

void MazeWidget::stepToward(const Position& target)
{
    const int row = m_player.row();
    const int col = m_player.col();
    const int dr = target.first - row;
    const int dc = target.second - col;

    if (std::abs(dr) + std::abs(dc) == 1) {
        moveTo(target.first, target.second); // ignored if a wall is in the way
    } else if (std::abs(dr) == 1 && std::abs(dc) == 1) {
        // Cursor cut across a corner: go round it through whichever side is open.
        if (m_player.canMoveTo(m_maze, row, target.second)) {
            moveTo(row, target.second);
        } else if (m_player.canMoveTo(m_maze, target.first, col)) {
            moveTo(target.first, col);
        } else {
            return;
        }
        moveTo(target.first, target.second);
    }
    // Further away: the cursor is past a wall, so the player waits.
}

void MazeWidget::followCursor(const QPointF& from, const QPointF& to)
{
    const double stepLength = geometry().cellSize / 4.0;
    const QPointF delta = to - from;
    const double distance = std::hypot(delta.x(), delta.y());
    const int samples = std::max(1, static_cast<int>(std::ceil(distance / stepLength)));

    for (int i = 1; i <= samples; ++i) {
        if (!canAcceptInput()) {
            return; // reached the exit mid-drag
        }
        stepToward(cellAt(from + delta * (static_cast<double>(i) / samples)));
    }
}

void MazeWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !canAcceptInput()) {
        QWidget::mousePressEvent(event);
        return;
    }

    const QPointF pos = event->position();
    m_dragging = true;
    m_lastMousePos = pos;
    setCursor(Qt::ClosedHandCursor);

    stepToward(cellAt(pos)); // a plain click on a neighbour moves there
}

void MazeWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_dragging || !canAcceptInput()) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPointF pos = event->position();
    followCursor(m_lastMousePos, pos);
    m_lastMousePos = pos;
}

void MazeWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        unsetCursor();
    }
    QWidget::mouseReleaseEvent(event);
}

void MazeWidget::keyPressEvent(QKeyEvent* event)
{
    if (!canAcceptInput()) {
        QWidget::keyPressEvent(event);
        return;
    }

    int row = m_player.row();
    int col = m_player.col();
    switch (event->key()) {
    case Qt::Key_Up:    case Qt::Key_W: --row; break;
    case Qt::Key_Down:  case Qt::Key_S: ++row; break;
    case Qt::Key_Left:  case Qt::Key_A: --col; break;
    case Qt::Key_Right: case Qt::Key_D: ++col; break;
    default:
        QWidget::keyPressEvent(event);
        return;
    }

    moveTo(row, col);
}

void MazeWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), Qt::white);

    const int n = m_maze.size();
    const Geometry g = geometry();

    auto centreOf = [&](int row, int col) {
        return QPointF(g.left + (col + 0.5) * g.cellSize, g.top + (row + 0.5) * g.cellSize);
    };

    // Exit cell highlight.
    painter.fillRect(QRectF(g.left + (n - 1) * g.cellSize, g.top + (n - 1) * g.cellSize, g.cellSize, g.cellSize),
                     QColor(200, 240, 200));

    // Route walked so far.
    if (m_route.size() >= 2) {
        QPolygonF line;
        for (const Position& p : m_route) {
            line << centreOf(p.first, p.second);
        }
        painter.setPen(QPen(QColor(50, 110, 230, 170), g.cellSize * 0.25, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPolyline(line);
    }

    // Walls.
    painter.setPen(QPen(Qt::black, 2, Qt::SolidLine, Qt::SquareCap));
    for (int row = 0; row < n; ++row) {
        for (int col = 0; col < n; ++col) {
            const Cell& c = m_maze.cell(row, col);
            const double x = g.left + col * g.cellSize;
            const double y = g.top + row * g.cellSize;
            const double s = g.cellSize;

            if (c.topWall)    painter.drawLine(QPointF(x, y), QPointF(x + s, y));
            if (c.rightWall)  painter.drawLine(QPointF(x + s, y), QPointF(x + s, y + s));
            if (c.bottomWall) painter.drawLine(QPointF(x, y + s), QPointF(x + s, y + s));
            if (c.leftWall)   painter.drawLine(QPointF(x, y), QPointF(x, y + s));
        }
    }

    // Player.
    const double radius = g.cellSize * 0.3;
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(220, 50, 50));
    painter.drawEllipse(centreOf(m_player.row(), m_player.col()), radius, radius);

    // Win message.
    if (hasWon()) {
        QFont font = painter.font();
        font.setPointSize(24);
        font.setBold(true);
        painter.setFont(font);
        painter.setPen(QColor(30, 130, 30));
        painter.drawText(rect(), Qt::AlignCenter, "You escaped!");
    }
}
