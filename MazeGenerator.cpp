#include "MazeGenerator.h"

#include <stack>
#include <stdexcept>
#include <utility>

MazeGenerator::MazeGenerator(int size)
    : m_size(size)
    , m_rng(std::random_device{}())
{
    if (size < 5) {
        throw std::invalid_argument("Maze size must be at least 5");
    }
    reset();
}

void MazeGenerator::reset()
{
    m_grid.assign(m_size, std::vector<Cell>(m_size));
}

void MazeGenerator::generate()
{
    reset();

    std::stack<std::pair<int, int>> path;
    m_grid[0][0].visited = true;
    path.push({0, 0});

    const int dRow[] = {-1, 0, 1, 0};
    const int dCol[] = {0, 1, 0, -1};

    while (!path.empty()) {
        auto [row, col] = path.top();

        // Collect neighbours we haven't visited yet.
        std::vector<std::pair<int, int>> options;
        for (int i = 0; i < 4; ++i) {
            int r = row + dRow[i];
            int c = col + dCol[i];
            if (r >= 0 && r < m_size && c >= 0 && c < m_size && !m_grid[r][c].visited) {
                options.push_back({r, c});
            }
        }

        if (options.empty()) {
            path.pop(); // dead end: backtrack
            continue;
        }

        std::uniform_int_distribution<size_t> pick(0, options.size() - 1);
        auto [nextRow, nextCol] = options[pick(m_rng)];

        removeWallBetween(row, col, nextRow, nextCol);
        m_grid[nextRow][nextCol].visited = true;
        path.push({nextRow, nextCol});
    }

    m_grid[0][0].topWall = false;
    m_grid[m_size - 1][m_size - 1].bottomWall = false;
}

void MazeGenerator::removeWallBetween(int row, int col, int nextRow, int nextCol)
{
    Cell& a = m_grid[row][col];
    Cell& b = m_grid[nextRow][nextCol];

    if (nextRow < row) {        // b is above a
        a.topWall = false;
        b.bottomWall = false;
    } else if (nextRow > row) { // b is below a
        a.bottomWall = false;
        b.topWall = false;
    } else if (nextCol > col) { // b is right of a
        a.rightWall = false;
        b.leftWall = false;
    } else {                    // b is left of a
        a.leftWall = false;
        b.rightWall = false;
    }
}

int MazeGenerator::size() const
{
    return m_size;
}

Cell& MazeGenerator::cell(int row, int col)
{
    return m_grid.at(row).at(col);
}

const Cell& MazeGenerator::cell(int row, int col) const
{
    return m_grid.at(row).at(col);
}

const std::vector<std::vector<Cell>>& MazeGenerator::grid() const
{
    return m_grid;
}
