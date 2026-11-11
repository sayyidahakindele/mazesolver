# MazeSolver

A C++ desktop maze game built with Qt 6 Widgets. Pick a maze size, find your way from the top-left entrance to the green exit, and get scored on how close you came to the shortest route. Stuck? Press Solve and it walks you there along the shortest path.

## Features

### Home screen

- **Maze size:** pick a size from 5 to 100 (a 15 means a 15×15 maze) and press **Start**. Every Start makes a new random maze.
- **Dark mode:** checkbox in the top-right. It recolours the whole app, including the maze, and is remembered next time you open the app.
- **Recent games:** your last 10 completed games, newest first, with size, time and stars (or "Used Solve"). The list is saved between launches.

### Game screen

- **Move:** click a cell next to the player, or use the arrow keys / WASD. A blue line shows the route you've walked.
- **Drag:** hold the mouse button and drag; the player follows the cursor through open passages and stops at walls.
- **Undo a step:** click, drag or move back to the cell you just came from; the last part of the line disappears.
- **Solve:** walks the player along the shortest path from where it stands to the exit. Using it means no score for that run.
- **Restart:** puts the player back at the start of the same maze and resets the route, moves and score. It also stops Solve partway through. The timer keeps going, so it measures the whole game.
- **Home (top-right):** ends the game and goes back to the home screen. If a game is in progress, it asks first.
- **Time and moves:** shown in the top bar with the shortest possible number of moves. The clock starts on your first move, pauses at the exit, and carries on after Restart. Every cell moved, including steps back, counts as a move.

### Score

When you reach the exit you get a score: shortest possible moves ÷ your moves, as a percentage. Wrong turns and backtracking lower it.

| Score | Stars |
|---|---|
| 100% (perfect run) | ★★★★★ |
| 90% or more | ★★★★☆ |
| 75% or more | ★★★☆☆ |
| 50% or more | ★★☆☆☆ |
| below 50% | ★☆☆☆☆ |

## Quick start (PowerShell / VS Code terminal)

If the project has been built before, this one line builds any changes and opens the app:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"; cmake --build build; .\build\MazeSolver.exe
```

On a fresh copy of the project (no `build` folder yet), follow [Build and run](#build-and-run) below first.

## Requirements (Windows)

This project builds with the [MSYS2](https://www.msys2.org/) UCRT64 toolchain. After installing MSYS2, open the **MSYS2 UCRT64** terminal and install the tools:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-ninja \
          mingw-w64-ucrt-x86_64-qt6-base
```

## Build and run

Run all commands from the project folder.

### PowerShell (including the VS Code terminal)

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
cmake --build build
.\build\MazeSolver.exe
```

### MSYS2 UCRT64 terminal or Git Bash

```bash
export PATH=/c/msys64/ucrt64/bin:$PATH
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
cmake --build build
./build/MazeSolver.exe
```

What each line does:

| Command | Purpose | When to run it |
|---|---|---|
| PATH line | Lets the terminal find `cmake`, `g++` and the Qt DLLs the app needs | Once per new terminal |
| `cmake -S . -B build ...` | Configures the project into the `build` folder | Once, or after editing `CMakeLists.txt` |
| `cmake --build build` | Compiles the code into `build/MazeSolver.exe` | After every code change |
| `MazeSolver.exe` | Runs the app | Whenever you want to play |

The app opens on the home screen, where you choose the maze size. Once a game starts the maze is fixed; for a different one, go back home and press Start again.

## How the code works

### Files

| File | What it does |
|---|---|
| [main.cpp](main.cpp) | Entry point. Creates the main window and starts Qt. |
| [MainWindow.h](MainWindow.h) / [.cpp](MainWindow.cpp) | The window: the home screen (size picker, Start, dark mode, recent games) and the game screen (Solve, Restart, Home, time/moves, maze). |
| [MazeGenerator.h](MazeGenerator.h) / [.cpp](MazeGenerator.cpp) | Holds the grid of `Cell`s and carves a random maze into it. |
| [MazeSolver.h](MazeSolver.h) / [.cpp](MazeSolver.cpp) | Finds the shortest path between two cells. |
| [Player.h](Player.h) / [.cpp](Player.cpp) | Tracks the player's position and checks whether a move is allowed. |
| [MazeWidget.h](MazeWidget.h) / [.cpp](MazeWidget.cpp) | The game itself: draws the maze, handles mouse/keyboard input, keeps the route, animates Solve, and tracks time, moves and score. |
| [CMakeLists.txt](CMakeLists.txt) | Build setup: finds Qt 6 and lists the source files. |

### How the pieces fit together

```
main.cpp
  └─ MainWindow                (home screen + game screen)
       └─ MazeWidget            (drawing + input, one per game)
            ├─ MazeGenerator    (the maze: grid of cells and walls)
            ├─ Player           (where the player is, which moves are legal)
            └─ MazeSolver       (shortest path, used by Solve and the score)
```

### The window (`MainWindow`)

`MainWindow` uses a `QStackedWidget` to switch between two pages:

- **Home page:** a `QSpinBox` for the size (5 to 100), a Start button, a Dark mode checkbox, and a Recent games table.
- **Game page:** Solve, Restart and Home buttons, the time/moves label, and the maze.

`startGame()` creates a new `MazeWidget` of the chosen size, so every game gets a new maze, timer and score. `goHome()` deletes it, which also stops its timers. The Home button calls `requestHome()`, which asks "End this game?" if `MazeWidget::inProgress()` is true. When the widget emits `won()`, `recordWin()` adds the game to the front of the recent list.

**Dark mode:** `main.cpp` sets Qt's Fusion style, and `applyTheme()` swaps the app-wide `QPalette` between a dark palette and the standard light one. It also calls `MazeWidget::setDarkMode()`, which picks the maze's colours (background, walls, exit, route, player, win panel).

**Saved settings:** dark mode and the recent games (size, time in ms, stars out of 5; 0 stars means Solve was used) are stored with `QSettings`. On Windows that's the registry key `HKEY_CURRENT_USER\Software\MazeSolver\MazeSolver`. Only the 10 newest games are kept. Games saved before the 5-star system (rated out of 3) are converted when loaded: 3 → 5, 2 → 3, 1 → 1.

### The maze (`MazeGenerator`)

The maze is an N×N grid of `Cell`s. Each cell has four walls (`topWall`, `rightWall`, `bottomWall`, `leftWall`), all up at the start. A wall between two cells is stored on both of them, so the two sides always agree.

`generate()` uses the **recursive backtracker** algorithm (a depth-first search):

1. Start at the top-left cell and mark it visited.
2. Pick a random unvisited neighbour, knock down the wall between, and move there.
3. If every neighbour is visited (a dead end), step back along the path and try again from there.
4. Stop when every cell has been visited.

This makes a *perfect maze*: every cell is reachable and there is exactly one route between any two cells. Finally, the top wall of the top-left cell (entrance) and the bottom wall of the bottom-right cell (exit) are removed.

The size must be at least 5; smaller sizes throw `std::invalid_argument`. The widget stores the maze as `const`, so it can't change after it's created.

### The solver (`MazeSolver`)

`MazeSolver::shortestPath(maze, start, goal)` uses **breadth-first search (BFS)**:

1. Start from `start` and explore outward one step at a time, only through open walls.
2. For each cell reached, remember which cell it was reached from (`cameFrom`).
3. Once `goal` is reached, follow `cameFrom` backwards to `start` and reverse the list.

BFS reaches every cell by the fewest possible steps, so the path is always the shortest. It returns the list of cells from start to goal, or an empty list if there's no path.

### The player (`Player`)

`Player` stores a row and column. `canMoveTo(maze, row, col)` returns true only if the target cell is directly up, down, left or right of the player **and** the wall between them is open. Clicks, drags, keys and the solver all go through this check, so nothing can move the player through a wall.

### The widget (`MazeWidget`)

- **Route:** `m_route` is the list of cells from the start to the player. `moveTo()` adds the new cell to the end of the list, or removes the last cell if the player stepped back onto the cell it just came from. That's what makes the line grow and shrink.
- **Clicks:** `cellAt()` converts the mouse position to a grid cell, and the player moves there if it's an open neighbour.
- **Dragging:** while the mouse button is held, `followCursor()` checks points along the cursor's path every quarter-cell, so fast drags don't skip cells. If the cursor cuts across a corner, `stepToward()` goes round it through whichever side is open. If the cursor is past a wall, the player waits.
- **Solve:** runs `MazeSolver` from the player's current position to the exit, then a `QTimer` moves the player one cell every 60 ms using the same `moveTo()`. If the player was in a dead end, it backs out and the route shrinks; at the exit, the route is exactly the shortest path.
- **Timer and moves:** every successful `moveTo()` adds one to `m_moves`. The first move starts a `QElapsedTimer`. Reaching the exit pauses it by adding the running time to `m_pausedMs`; the next move starts it again, so the total keeps growing across restarts. The widget emits a `statsChanged` signal after each move and every 100 ms, and [MainWindow.cpp](MainWindow.cpp) shows it in the label next to the buttons. Solve's steps count too.
- **Score:** when the widget is created it runs `MazeSolver` from the entrance to the exit once and stores the length as `m_optimalMoves`. `scorePercent()` is `m_optimalMoves / m_moves × 100`, and `stars()` turns that into 1–5 stars (`MazeWidget::kMaxStars`). The win message shows moves, time, shortest possible and the score, or "No score (used Solve)".
- **Restart:** `restart()` stops the Solve timer, moves the player to (0, 0), resets `m_route` to just the start cell, and clears the move count and "used Solve" flag. It doesn't touch the clock. The maze itself is `const`, so it stays the same.
- **Input lock:** clicks, drags and keys are ignored while Solve is animating and after the player reaches the exit.
- **Drawing (`paintEvent`):** scales the grid to fit the window, then draws the exit cell, the route line, the walls and the player, using light or dark colours depending on `m_darkMode`. At the exit it adds a panel with "You escaped!", moves, time, shortest possible and the score.

## Troubleshooting

- **"Qt6Core.dll was not found" or the app exits right away:** `C:\msys64\ucrt64\bin` isn't on your PATH. Run the PATH line above in the same terminal before starting the app.
- **The term `\build\MazeSolver.exe` is not recognized:** the path needs a leading dot: `.\build\MazeSolver.exe`.
- **`cannot open output file MazeSolver.exe: Permission denied` when building:** the app is still running. Close the maze window and build again.
- **CMake finds the wrong Qt (for example a Qt 5 install in `C:\Qt`):** delete the `build` folder and configure again with `-DCMAKE_PREFIX_PATH=C:/msys64/ucrt64`.
- **`cmake` or `ninja` not found:** check that the PATH line ran, or install the packages listed under Requirements.
