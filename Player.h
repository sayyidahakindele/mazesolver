#pragma once

#include "MazeGenerator.h"

// A position in the maze that can only move through open walls.
class Player {
public:
    Player(int row = 0, int col = 0);

    // True if (row, col) is right next to the player with no wall in between.
    bool canMoveTo(const MazeGenerator& maze, int row, int col) const;

    void setPosition(int row, int col);

    int row() const;
    int col() const;

private:
    int m_row;
    int m_col;
};
