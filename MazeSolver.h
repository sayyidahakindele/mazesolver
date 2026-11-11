#pragma once

#include <utility>
#include <vector>

#include "MazeGenerator.h"

using Position = std::pair<int, int>; // (row, col)

class MazeSolver {
public:
    // Finds the shortest path from start to goal using breadth-first search,
    // moving only through open walls. The result includes both start and goal.
    // Returns an empty vector if there is no path.
    static std::vector<Position> shortestPath(const MazeGenerator& maze, Position start, Position goal);
};
