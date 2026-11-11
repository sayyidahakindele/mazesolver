#pragma once

#include <random>
#include <vector>

// One square in the maze. All walls start up; maze generation knocks them down.
struct Cell {
    bool topWall = true;
    bool rightWall = true;
    bool bottomWall = true;
    bool leftWall = true;
    bool visited = false;
};

class MazeGenerator {
public:
    // Creates a size x size grid. Throws std::invalid_argument if size < 5.
    explicit MazeGenerator(int size);

    // Puts every wall back up and clears visited flags.
    void reset();

    // Carves a random perfect maze (exactly one path between any two cells)
    // using a depth-first "recursive backtracker". The entrance is the top of
    // the top-left cell and the exit is the bottom of the bottom-right cell.
    void generate();

    int size() const;

    Cell& cell(int row, int col);
    const Cell& cell(int row, int col) const;

    const std::vector<std::vector<Cell>>& grid() const;

private:
    void removeWallBetween(int row, int col, int nextRow, int nextCol);

    int m_size;
    std::vector<std::vector<Cell>> m_grid;
    std::mt19937 m_rng;
};
