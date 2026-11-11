#pragma once

#include <QWidget>
#include <QtGlobal>
#include <vector>

class QCheckBox;
class QLabel;
class QSpinBox;
class QStackedWidget;
class QTableWidget;
class QVBoxLayout;
class MazeWidget;

// A finished game, shown in the Recent games list on the home screen.
struct RecentGame {
    int size;
    qint64 timeMs;
    int stars; // 1-5, or 0 if Solve was used
};

// The app window. Holds two screens:
//  - Home: pick the maze size, press Start, toggle dark mode, see recent games.
//  - Game: the maze with Solve / Restart / Home buttons and the time and moves.
// Dark mode and recent games are saved with QSettings, so they survive restarts.
class MainWindow : public QWidget {
public:
    MainWindow();

private:
    QWidget* buildHomePage();
    QWidget* buildGamePage();

    void startGame();   // makes a new maze of the chosen size and shows the game screen
    void requestHome(); // asks before leaving a game in progress
    void goHome();      // throws away the current game and shows the home screen
    void discardMaze(); // disconnects and deletes the current maze, if any
    void showStats(int moves, qint64 elapsedMs);

    void applyTheme(bool dark);
    void recordWin();
    void loadRecentGames();
    void saveRecentGames() const;
    void refreshRecentTable();

    QStackedWidget* m_stack = nullptr;
    QWidget* m_homePage = nullptr;
    QWidget* m_gamePage = nullptr;

    QSpinBox* m_sizeBox = nullptr;
    QCheckBox* m_darkModeBox = nullptr;
    QTableWidget* m_recentTable = nullptr;
    QLabel* m_noRecentLabel = nullptr;

    QVBoxLayout* m_gameLayout = nullptr;
    MazeWidget* m_maze = nullptr; // replaced on every Start
    QLabel* m_statsLabel = nullptr;

    bool m_darkMode = false;
    std::vector<RecentGame> m_recentGames; // newest first
};
