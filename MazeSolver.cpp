#include "MazeSolver.h"

#include <algorithm>
#include <queue>

std::vector<Position> MazeSolver::shortestPath(const MazeGenerator& maze, Position start, Position goal)
{
    const int n = maze.size();

    // cameFrom[r][c] is the cell we stepped from to reach (r, c); (-1, -1) means not reached yet.
    std::vector<std::vector<Position>> cameFrom(n, std::vector<Position>(n, {-1, -1}));
    std::queue<Position> frontier;

    frontier.push(start);
    cameFrom[start.first][start.second] = start;

    while (!frontier.empty()) {
        auto [row, col] = frontier.front();
        frontier.pop();

        if (Position{row, col} == goal) {
            break;
        }

        const Cell& here = maze.cell(row, col);

        // Neighbours we can walk to (no wall between us and them).
        std::vector<Position> next;
        if (!here.topWall    && row > 0)     next.push_back({row - 1, col});
        if (!here.bottomWall && row < n - 1) next.push_back({row + 1, col});
        if (!here.leftWall   && col > 0)     next.push_back({row, col - 1});
        if (!here.rightWall  && col < n - 1) next.push_back({row, col + 1});

        for (const Position& p : next) {
            if (cameFrom[p.first][p.second].first == -1) {
                cameFrom[p.first][p.second] = {row, col};
                frontier.push(p);
            }
        }
    }

    if (cameFrom[goal.first][goal.second].first == -1) {
        return {}; // goal never reached
    }

    // Walk backwards from the goal to the start, then reverse.
    std::vector<Position> path;
    for (Position p = goal; p != start; p = cameFrom[p.first][p.second]) {
        path.push_back(p);
    }
    path.push_back(start);
    std::reverse(path.begin(), path.end());
    return path;
}
