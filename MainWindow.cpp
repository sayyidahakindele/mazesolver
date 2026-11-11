#include "MainWindow.h"

#include <QApplication>
#include <algorithm>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QStyle>
#include <QTableWidget>
#include <QVBoxLayout>

#include "MazeWidget.h"

namespace {

constexpr int kMaxRecentGames = 10;

QPalette darkPalette()
{
    QPalette p;
    const QColor window(30, 31, 34);
    const QColor base(42, 43, 47);
    const QColor text(230, 230, 230);
    const QColor disabledText(120, 120, 120);

    p.setColor(QPalette::Window, window);
    p.setColor(QPalette::WindowText, text);
    p.setColor(QPalette::Base, base);
    p.setColor(QPalette::AlternateBase, QColor(48, 49, 54));
    p.setColor(QPalette::ToolTipBase, base);
    p.setColor(QPalette::ToolTipText, text);
    p.setColor(QPalette::Text, text);
    p.setColor(QPalette::PlaceholderText, QColor(140, 140, 140));
    p.setColor(QPalette::Button, QColor(47, 48, 53));
    p.setColor(QPalette::ButtonText, text);
    p.setColor(QPalette::BrightText, Qt::white);
    p.setColor(QPalette::Highlight, QColor(61, 111, 214));
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Link, QColor(110, 160, 255));
    p.setColor(QPalette::Mid, QColor(70, 71, 76));
    p.setColor(QPalette::Dark, QColor(20, 21, 23));

    p.setColor(QPalette::Disabled, QPalette::WindowText, disabledText);
    p.setColor(QPalette::Disabled, QPalette::Text, disabledText);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, disabledText);
    return p;
}

QString starsText(int stars)
{
    if (stars <= 0) {
        return "Used Solve";
    }
    return QString(stars, QChar(0x2605)) + QString(MazeWidget::kMaxStars - stars, QChar(0x2606)); // filled + empty
}

} // namespace

MainWindow::MainWindow()
{
    setWindowTitle("Maze Solver");

    QSettings settings;
    m_darkMode = settings.value("darkMode", false).toBool();
    loadRecentGames();

    m_homePage = buildHomePage();
    m_gamePage = buildGamePage();

    m_stack = new QStackedWidget;
    m_stack->addWidget(m_homePage);
    m_stack->addWidget(m_gamePage);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_stack);

    applyTheme(m_darkMode);
    refreshRecentTable();
    resize(600, 700);
}

QWidget* MainWindow::buildHomePage()
{
    auto* page = new QWidget;

    // Dark mode toggle, top-right.
    m_darkModeBox = new QCheckBox("Dark mode");
    m_darkModeBox->setChecked(m_darkMode);
    connect(m_darkModeBox, &QCheckBox::toggled, this, [this](bool on) {
        applyTheme(on);
        QSettings().setValue("darkMode", on);
    });
    auto* topBar = new QHBoxLayout;
    topBar->addStretch();
    topBar->addWidget(m_darkModeBox);

    auto* title = new QLabel("Maze Solver");
    QFont titleFont = title->font();
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setAlignment(Qt::AlignCenter);

    auto* subtitle = new QLabel("Find your way from the top-left entrance to the green exit.");
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setWordWrap(true);

    // Size picker.
    auto* sizeLabel = new QLabel("Maze size:");
    m_sizeBox = new QSpinBox;
    m_sizeBox->setRange(5, 100);
    m_sizeBox->setValue(15);
    m_sizeBox->setMinimumWidth(80);

    auto* sizeHint = new QLabel;
    auto updateHint = [sizeHint](int n) { sizeHint->setText(QString(" × %1 cells").arg(n)); };
    updateHint(m_sizeBox->value());
    connect(m_sizeBox, &QSpinBox::valueChanged, sizeHint, updateHint);

    auto* sizeRow = new QHBoxLayout;
    sizeRow->addStretch();
    sizeRow->addWidget(sizeLabel);
    sizeRow->addWidget(m_sizeBox);
    sizeRow->addWidget(sizeHint);
    sizeRow->addStretch();

    auto* startButton = new QPushButton("Start");
    QFont startFont = startButton->font();
    startFont.setPointSize(14);
    startFont.setBold(true);
    startButton->setFont(startFont);
    startButton->setMinimumSize(160, 48);
    connect(startButton, &QPushButton::clicked, this, [this] { startGame(); });

    // Recent games.
    auto* recentTitle = new QLabel("Recent games");
    QFont recentFont = recentTitle->font();
    recentFont.setPointSize(13);
    recentFont.setBold(true);
    recentTitle->setFont(recentFont);

    m_recentTable = new QTableWidget(0, 3);
    m_recentTable->setHorizontalHeaderLabels({"Size", "Time", "Stars"});
    m_recentTable->verticalHeader()->hide();
    m_recentTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_recentTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_recentTable->setSelectionMode(QAbstractItemView::NoSelection);
    m_recentTable->setFocusPolicy(Qt::NoFocus);
    m_recentTable->setAlternatingRowColors(true);
    m_recentTable->setMaximumWidth(460);

    m_noRecentLabel = new QLabel("No completed games yet.");
    m_noRecentLabel->setAlignment(Qt::AlignCenter);

    auto* layout = new QVBoxLayout(page);
    layout->addLayout(topBar);
    layout->addStretch(1);
    layout->addWidget(title);
    layout->addWidget(subtitle);
    layout->addSpacing(28);
    layout->addLayout(sizeRow);
    layout->addSpacing(20);
    layout->addWidget(startButton, 0, Qt::AlignHCenter);
    layout->addSpacing(32);
    layout->addWidget(recentTitle, 0, Qt::AlignHCenter);
    layout->addWidget(m_noRecentLabel);
    layout->addWidget(m_recentTable, 3, Qt::AlignHCenter);
    layout->addStretch(1);

    return page;
}

QWidget* MainWindow::buildGamePage()
{
    auto* page = new QWidget;

    auto* solveButton = new QPushButton("Solve");
    auto* restartButton = new QPushButton("Restart");
    auto* homeButton = new QPushButton("Home");
    for (QPushButton* b : {solveButton, restartButton, homeButton}) {
        b->setFocusPolicy(Qt::NoFocus); // keep arrow keys going to the maze
    }

    m_statsLabel = new QLabel;

    auto* topRow = new QHBoxLayout;
    topRow->addWidget(solveButton);
    topRow->addWidget(restartButton);
    topRow->addSpacing(16);
    topRow->addWidget(m_statsLabel);
    topRow->addStretch();
    topRow->addWidget(homeButton); // top-right corner

    m_gameLayout = new QVBoxLayout(page);
    m_gameLayout->addLayout(topRow);
    // The maze widget is added below the top row by startGame().

    connect(solveButton, &QPushButton::clicked, this, [this] {
        if (m_maze) m_maze->solve();
    });
    connect(restartButton, &QPushButton::clicked, this, [this] {
        if (m_maze) m_maze->restart();
    });
    connect(homeButton, &QPushButton::clicked, this, [this] { requestHome(); });

    return page;
}

void MainWindow::startGame()
{
    const int size = m_sizeBox->value();

    if (m_maze) {
        m_gameLayout->removeWidget(m_maze);
        m_maze->deleteLater();
    }
    m_maze = new MazeWidget(size);
    m_maze->setDarkMode(m_darkMode);
    m_gameLayout->addWidget(m_maze, 1);

    connect(m_maze, &MazeWidget::statsChanged, this, [this](int moves, qint64 ms) { showStats(moves, ms); });
    connect(m_maze, &MazeWidget::won, this, [this] { recordWin(); });

    showStats(0, 0);
    setWindowTitle(QString("Maze Solver - %1 x %1").arg(size));

    m_stack->setCurrentWidget(m_gamePage);
    m_maze->setFocus(); // arrow keys move the player right away
}

void MainWindow::requestHome()
{
    if (m_maze && m_maze->inProgress()) {
        const auto answer = QMessageBox::question(this, "End game?",
                                                  "End this game and go back to the home screen?");
        if (answer != QMessageBox::Yes) {
            m_maze->setFocus();
            return;
        }
    }
    goHome();
}

void MainWindow::goHome()
{
    if (m_maze) {
        m_gameLayout->removeWidget(m_maze);
        m_maze->deleteLater(); // also stops its timers
        m_maze = nullptr;
    }
    setWindowTitle("Maze Solver");
    m_stack->setCurrentWidget(m_homePage);
    m_sizeBox->setFocus();
}

void MainWindow::showStats(int moves, qint64 elapsedMs)
{
    const int shortest = m_maze ? m_maze->optimalMoves() : 0;
    m_statsLabel->setText(QString("Time: %1     Moves: %2     Shortest possible: %3")
                              .arg(MazeWidget::formatTime(elapsedMs))
                              .arg(moves)
                              .arg(shortest));
}

void MainWindow::applyTheme(bool dark)
{
    m_darkMode = dark;
    // Fusion draws consistently with custom palettes on every platform.
    qApp->setPalette(dark ? darkPalette() : qApp->style()->standardPalette());
    if (m_maze) {
        m_maze->setDarkMode(dark);
    }
}

void MainWindow::recordWin()
{
    if (!m_maze) {
        return;
    }
    const RecentGame game{m_maze->mazeSize(), m_maze->elapsedMs(), m_maze->usedSolve() ? 0 : m_maze->stars()};
    m_recentGames.insert(m_recentGames.begin(), game);
    if (m_recentGames.size() > kMaxRecentGames) {
        m_recentGames.resize(kMaxRecentGames);
    }
    saveRecentGames();
    refreshRecentTable();
}

void MainWindow::loadRecentGames()
{
    QSettings settings;
    const int count = settings.beginReadArray("recentGames");
    for (int i = 0; i < count && i < kMaxRecentGames; ++i) {
        settings.setArrayIndex(i);
        int stars = settings.value("stars").toInt();
        // Games saved before the 5-star system were rated out of 3.
        if (settings.value("outOf", 3).toInt() == 3) {
            const int fromThree[] = {0, 1, 3, 5};
            stars = fromThree[std::clamp(stars, 0, 3)];
        }
        m_recentGames.push_back({settings.value("size").toInt(),
                                 settings.value("timeMs").toLongLong(),
                                 stars});
    }
    settings.endArray();
}

void MainWindow::saveRecentGames() const
{
    QSettings settings;
    settings.remove("recentGames");
    settings.beginWriteArray("recentGames", static_cast<int>(m_recentGames.size()));
    for (int i = 0; i < static_cast<int>(m_recentGames.size()); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("size", m_recentGames[i].size);
        settings.setValue("timeMs", m_recentGames[i].timeMs);
        settings.setValue("stars", m_recentGames[i].stars);
        settings.setValue("outOf", MazeWidget::kMaxStars);
    }
    settings.endArray();
}

void MainWindow::refreshRecentTable()
{
    m_recentTable->setRowCount(static_cast<int>(m_recentGames.size()));
    for (int row = 0; row < static_cast<int>(m_recentGames.size()); ++row) {
        const RecentGame& g = m_recentGames[row];
        const QStringList cells = {QString("%1 × %1").arg(g.size), MazeWidget::formatTime(g.timeMs),
                                   starsText(g.stars)};
        for (int col = 0; col < 3; ++col) {
            auto* item = new QTableWidgetItem(cells[col]);
            item->setTextAlignment(Qt::AlignCenter);
            m_recentTable->setItem(row, col, item);
        }
    }

    const bool empty = m_recentGames.empty();
    m_recentTable->setVisible(!empty);
    m_noRecentLabel->setVisible(empty);
}
