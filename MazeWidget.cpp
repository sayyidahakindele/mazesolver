#include "MazeWidget.h"

#include <QKeyEvent>
#include <QMouseEvent>
#include <QFontMetricsF>
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

    const int last = m_maze.size() - 1;
    m_optimalMoves = static_cast<int>(MazeSolver::shortestPath(m_maze, {0, 0}, {last, last}).size()) - 1;

    m_solveTimer.setInterval(60); // ms per step while auto-solving
    connect(&m_solveTimer, &QTimer::timeout, this, [this] { stepAlongSolution(); });

    m_clockTick.setInterval(100);
    connect(&m_clockTick, &QTimer::timeout, this, [this] { emit statsChanged(m_moves, elapsedMs()); });
}

int MazeWidget::moves() const
{
    return m_moves;
}

qint64 MazeWidget::elapsedMs() const
{
    return m_clockRunning ? m_pausedMs + m_clock.elapsed() : m_pausedMs;
}

bool MazeWidget::inProgress() const
{
    return elapsedMs() > 0 && !hasWon();
}

int MazeWidget::mazeSize() const
{
    return m_maze.size();
}

bool MazeWidget::usedSolve() const
{
    return m_usedSolve;
}

void MazeWidget::setDarkMode(bool dark)
{
    m_darkMode = dark;
    update();
}

int MazeWidget::optimalMoves() const
{
    return m_optimalMoves;
}

int MazeWidget::scorePercent() const
{
    if (m_moves == 0) {
        return 0;
    }
    return static_cast<int>(std::lround(100.0 * m_optimalMoves / m_moves));
}

int MazeWidget::stars() const
{
    const int score = scorePercent();
    if (score >= 100) return 5;
    if (score >= 90)  return 4;
    if (score >= 75)  return 3;
    if (score >= 50)  return 2;
    return 1;
}

QString MazeWidget::formatTime(qint64 ms)
{
    const qint64 tenths = ms / 100;
    const qint64 minutes = tenths / 600;
    const qint64 seconds = (tenths / 10) % 60;
    return QString("%1:%2.%3")
        .arg(minutes)
        .arg(seconds, 2, 10, QChar('0'))
        .arg(tenths % 10);
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

    if (!m_clockRunning) {
        m_clockRunning = true;
        m_clock.start();
        m_clockTick.start();
    }
    ++m_moves;
    if (hasWon()) {
        m_pausedMs += m_clock.elapsed(); // pause the clock at the exit
        m_clockRunning = false;
        m_clockTick.stop();
        emit won();
    }

    emit statsChanged(m_moves, elapsedMs());
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
        m_usedSolve = true;
        m_solveTimer.start();
    }
}

void MazeWidget::restart()
{
    m_solveTimer.stop();
    m_solution.clear();
    m_solutionStep = 0;

    m_player.setPosition(0, 0);
    m_route = {{0, 0}};

    if (m_dragging) {
        m_dragging = false;
        unsetCursor();
    }

    m_moves = 0;
    m_usedSolve = false;
    // The session timer is deliberately not reset.

    setFocus();
    emit statsChanged(m_moves, elapsedMs());
    update();
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
    // Colours for the current theme.
    const QColor background = m_darkMode ? QColor(30, 31, 34)        : QColor(Qt::white);
    const QColor wallColor  = m_darkMode ? QColor(215, 215, 220)     : QColor(Qt::black);
    const QColor exitColor  = m_darkMode ? QColor(40, 90, 55)        : QColor(200, 240, 200);
    const QColor routeColor = m_darkMode ? QColor(90, 150, 255, 180) : QColor(50, 110, 230, 170);
    const QColor playerColor= m_darkMode ? QColor(240, 85, 85)       : QColor(220, 50, 50);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), background);

    const int n = m_maze.size();
    const Geometry g = geometry();

    auto centreOf = [&](int row, int col) {
        return QPointF(g.left + (col + 0.5) * g.cellSize, g.top + (row + 0.5) * g.cellSize);
    };

    // Exit cell highlight.
    painter.fillRect(QRectF(g.left + (n - 1) * g.cellSize, g.top + (n - 1) * g.cellSize, g.cellSize, g.cellSize),
                     exitColor);

    // Route walked so far.
    if (m_route.size() >= 2) {
        QPolygonF line;
        for (const Position& p : m_route) {
            line << centreOf(p.first, p.second);
        }
        painter.setPen(QPen(routeColor, g.cellSize * 0.25, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        painter.drawPolyline(line);
    }

    // Walls.
    painter.setPen(QPen(wallColor, 2, Qt::SolidLine, Qt::SquareCap));
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
    painter.setBrush(playerColor);
    painter.drawEllipse(centreOf(m_player.row(), m_player.col()), radius, radius);

    if (hasWon()) {
        drawWinPanel(painter);
    }
}

void MazeWidget::drawWinPanel(QPainter& painter)
{
    const QColor cardColor   = m_darkMode ? QColor(35, 120, 72) : QColor(30, 140, 80);
    const QColor white(255, 255, 255);
    const QColor softWhite(255, 255, 255, 215);
    const QColor gold(255, 205, 60);
    const QColor emptyStar(255, 255, 255, 80);

    // Dim the maze so the results are the focus.
    painter.fillRect(rect(), m_darkMode ? QColor(0, 0, 0, 150) : QColor(0, 0, 0, 110));

    auto makeFont = [this](int pointSize, bool bold) {
        QFont f = font();
        f.setPointSize(pointSize);
        f.setBold(bold);
        return f;
    };
    const QFont titleFont  = makeFont(24, true);
    const QFont starFont   = makeFont(30, false);
    const QFont scoreFont  = makeFont(18, true);
    const QFont detailFont = makeFont(12, false);

    const QString title = "You escaped!";
    const int filledCount = m_usedSolve ? 0 : stars();
    const QString filledStars(filledCount, QChar(0x2605));
    const QString emptyStars(m_usedSolve ? 0 : kMaxStars - filledCount, QChar(0x2605));
    const QString scoreText = m_usedSolve ? QString("No score (used Solve)")
                                          : QString("Score: %1%").arg(scorePercent());
    const QString movesText = QString("%1 moves  ·  shortest possible %2").arg(m_moves).arg(m_optimalMoves);
    const QString timeText = QString("Time: %1").arg(formatTime(elapsedMs()));

    const QFontMetricsF titleFm(titleFont), starFm(starFont), scoreFm(scoreFont), detailFm(detailFont);

    // Card size: widest line plus padding.
    const double padX = 36, padY = 26;
    double contentWidth = std::max({titleFm.horizontalAdvance(title),
                                    scoreFm.horizontalAdvance(scoreText),
                                    detailFm.horizontalAdvance(movesText),
                                    detailFm.horizontalAdvance(timeText)});
    if (!m_usedSolve) {
        contentWidth = std::max(contentWidth, starFm.horizontalAdvance(filledStars + emptyStars));
    }
    double contentHeight = titleFm.height() + 6
                         + (m_usedSolve ? 0 : starFm.height() + 2)
                         + scoreFm.height() + 12
                         + detailFm.height() * 2 + 2;

    const double cardWidth = std::min(contentWidth + 2 * padX, width() - 20.0);
    const double cardHeight = contentHeight + 2 * padY;
    const QRectF card((width() - cardWidth) / 2.0, (height() - cardHeight) / 2.0, cardWidth, cardHeight);

    // Shadow, then the card.
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 90));
    painter.drawRoundedRect(card.translated(0, 6), 18, 18);
    painter.setPen(QPen(QColor(255, 255, 255, 70), 1.5));
    painter.setBrush(cardColor);
    painter.drawRoundedRect(card, 18, 18);

    // Lines of text, centred, top to bottom.
    double y = card.top() + padY;
    auto drawLine = [&](const QString& text, const QFont& f, const QColor& color, double lineHeight) {
        painter.setFont(f);
        painter.setPen(color);
        painter.drawText(QRectF(card.left(), y, card.width(), lineHeight), Qt::AlignCenter, text);
        y += lineHeight;
    };

    drawLine(title, titleFont, white, titleFm.height());
    y += 6;

    if (!m_usedSolve) {
        // Filled stars in gold, the rest faded, as one centred row.
        const double filledWidth = starFm.horizontalAdvance(filledStars);
        const double rowWidth = starFm.horizontalAdvance(filledStars + emptyStars);
        const double x = card.center().x() - rowWidth / 2.0;
        painter.setFont(starFont);
        painter.setPen(gold);
        painter.drawText(QRectF(x, y, filledWidth, starFm.height()), Qt::AlignLeft | Qt::AlignVCenter, filledStars);
        painter.setPen(emptyStar);
        painter.drawText(QRectF(x + filledWidth, y, rowWidth - filledWidth, starFm.height()),
                         Qt::AlignLeft | Qt::AlignVCenter, emptyStars);
        y += starFm.height() + 2;
    }

    drawLine(scoreText, scoreFont, white, scoreFm.height());
    y += 12;
    drawLine(movesText, detailFont, softWhite, detailFm.height());
    y += 2;
    drawLine(timeText, detailFont, softWhite, detailFm.height());
}
