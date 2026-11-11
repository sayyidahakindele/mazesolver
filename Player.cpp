#include "Player.h"

Player::Player(int row, int col)
    : m_row(row)
    , m_col(col)
{
}

bool Player::canMoveTo(const MazeGenerator& maze, int row, int col) const
{
    const int n = maze.size();
    if (row < 0 || row >= n || col < 0 || col >= n) {
        return false;
    }

    const Cell& here = maze.cell(m_row, m_col);

    if (row == m_row - 1 && col == m_col) return !here.topWall;
    if (row == m_row + 1 && col == m_col) return !here.bottomWall;
    if (row == m_row && col == m_col - 1) return !here.leftWall;
    if (row == m_row && col == m_col + 1) return !here.rightWall;
    return false; // not an adjacent cell
}

void Player::setPosition(int row, int col)
{
    m_row = row;
    m_col = col;
}

int Player::row() const
{
    return m_row;
}

int Player::col() const
{
    return m_col;
}
