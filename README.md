# MazeSolver

A C++ desktop app built with Qt 6 Widgets. It generates a random square maze that you walk through from the top-left entrance to the green exit at the bottom-right, or that it solves for you along the shortest path.

## Controls

- **Move:** click a cell next to the player (or use the arrow keys / WASD). A line shows the route you've walked.
- **Drag:** hold the mouse button and drag; the player follows the cursor through open passages and stops at walls.
- **Undo a step:** click (or drag back to) the cell you just came from; the last part of the line disappears.
- **Solve:** walks the player along the shortest path from where it stands to the exit.

## Quick start (PowerShell / VS Code terminal)

If the project has been built before, this one line builds any changes and runs a 13×13 maze:

```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"; cmake --build build; .\build\MazeSolver.exe 13
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
.\build\MazeSolver.exe 20
```

### MSYS2 UCRT64 terminal or Git Bash

```bash
export PATH=/c/msys64/ucrt64/bin:$PATH
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=C:/msys64/ucrt64
cmake --build build
./build/MazeSolver.exe 20
```

What each line does:

| Command | Purpose | When to run it |
|---|---|---|
| PATH line | Lets the terminal find `cmake`, `g++` and the Qt DLLs the app needs | Once per new terminal |
| `cmake -S . -B build ...` | Configures the project into the `build` folder | Once, or after editing `CMakeLists.txt` |
| `cmake --build build` | Compiles the code into `build/MazeSolver.exe` | After every code change |
| `MazeSolver.exe 20` | Runs the app | Whenever you want to play |

The number after `MazeSolver.exe` is the maze size: `20` makes a 20×20 maze. It must be a whole number from 5 to 100, and it defaults to 15 if you leave it out. The maze is fixed once the window opens; to get a different one, close the app and run it again.

## How the code works

### Files

| File | What it does |
|---|---|
| [main.cpp](main.cpp) | Entry point. Reads the maze size from the command line, builds the window (Solve button + maze), and starts Qt. |
| [MazeGenerator.h](MazeGenerator.h) / [.cpp](MazeGenerator.cpp) | Holds the grid of `Cell`s and carves a random maze into it. |
| [MazeSolver.h](MazeSolver.h) / [.cpp](MazeSolver.cpp) | Finds the shortest path between two cells. |
| [Player.h](Player.h) / [.cpp](Player.cpp) | Tracks the player's position and checks whether a move is allowed. |
| [MazeWidget.h](MazeWidget.h) / [.cpp](MazeWidget.cpp) | The Qt widget: draws everything, handles mouse/keyboard input, keeps the route, and animates Solve. |
| [CMakeLists.txt](CMakeLists.txt) | Build setup: finds Qt 6 and lists the source files. |

### How the pieces fit together

```
main.cpp
  └─ MazeWidget            (drawing + input)
       ├─ MazeGenerator    (the maze: grid of cells and walls)
       ├─ Player           (where the player is, which moves are legal)
       └─ MazeSolver       (shortest path, used by the Solve button)
```

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
- **Input lock:** clicks, drags and keys are ignored while Solve is animating and after the player reaches the exit.
- **Drawing (`paintEvent`):** scales the grid to fit the window, then draws the green exit cell, the blue route line, the walls, the red player, and "You escaped!" when the player reaches the exit.

## Troubleshooting

- **"Usage: MazeSolver &lt;size&gt;" and the app closes:** the size wasn't a whole number from 5 to 100.
- **"Qt6Core.dll was not found" or the app exits right away:** `C:\msys64\ucrt64\bin` isn't on your PATH. Run the PATH line above in the same terminal before starting the app.
- **The term `\build\MazeSolver.exe` is not recognized:** the path needs a leading dot: `.\build\MazeSolver.exe`.
- **`cannot open output file MazeSolver.exe: Permission denied` when building:** the app is still running. Close the maze window and build again.
- **CMake finds the wrong Qt (for example a Qt 5 install in `C:\Qt`):** delete the `build` folder and configure again with `-DCMAKE_PREFIX_PATH=C:/msys64/ucrt64`.
- **`cmake` or `ninja` not found:** check that the PATH line ran, or install the packages listed under Requirements.
