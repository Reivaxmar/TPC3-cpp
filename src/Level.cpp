#include "Level.hpp"
#include <fstream>
#include <stdexcept>

Level::Level(const std::string& file_path) {
    read_level(file_path);
    set_tile_images();
}

void Level::make_rectangle(std::vector<std::vector<char>>& grid) {
    size_t max_w = 0;
    for (auto& row : grid)
        if (row.size() > max_w) max_w = row.size();
    for (auto& row : grid) {
        while (row.size() < max_w) row.push_back('_');
        row.insert(row.begin(), 'o');  // left border
        row.push_back('o');            // right border
    }
    max_w += 2;
    grid.insert(grid.begin(), std::vector<char>(max_w, 'o'));  // top border
    grid.push_back(std::vector<char>(max_w, 'o'));             // bottom border
    level  = grid;
    size_x = (int)max_w;
    size_y = (int)level.size();
}

std::pair<int,int> Level::locate_tile(char ch) const {
    for (int i = 0; i < size_y; ++i)
        for (int j = 0; j < size_x; ++j)
            if (level[i][j] == ch)
                return {j * 32, i * 32};
    return {0, 0};
}

std::vector<std::pair<int,int>> Level::locate_multiple(char ch) const {
    std::vector<std::pair<int,int>> out;
    for (int i = 0; i < size_y; ++i)
        for (int j = 0; j < size_x; ++j)
            if (level[i][j] == ch)
                out.push_back({j * 32, i * 32});
    return out;
}

void Level::read_level(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("Cannot open level file: " + path);

    std::vector<std::vector<char>> grid;
    std::string line;
    while (std::getline(f, line)) {
        // strip \r in case of Windows line endings
        if (!line.empty() && line.back() == '\r') line.pop_back();
        grid.push_back(std::vector<char>(line.begin(), line.end()));
    }
    make_rectangle(grid);
    a_pos    = locate_tile('A');
    b_pos    = locate_tile('B');
    fish_pos = locate_multiple('F');
}

// Autotiling – exact translation of the Python set_tile_images()
void Level::set_tile_images() {
    tile_indexes.assign(size_y, std::vector<int>(size_x, 0));

    auto is = [&](int row, int col) -> bool {
        if (row < 0 || row >= size_y || col < 0 || col >= size_x) return false;
        return level[row][col] == 'o';
    };

    // Top border row
    for (int j = 0; j < size_x; ++j)
        tile_indexes[0][j] = is(1, j) ? 8 : 14;
    // Bottom border row
    for (int j = 0; j < size_x; ++j)
        tile_indexes[size_y - 1][j] = is(size_y - 2, j) ? 8 : 2;
    // Left border column
    for (int i = 1; i < size_y - 1; ++i)
        tile_indexes[i][0] = is(i, 1) ? 8 : 9;
    // Right border column
    for (int i = 1; i < size_y - 1; ++i)
        tile_indexes[i][size_x - 1] = is(i, size_x - 2) ? 8 : 7;

    // Interior tiles
    for (int i = 1; i < size_y - 1; ++i) {
        for (int j = 1; j < size_x - 1; ++j) {
            if (level[i][j] != 'o') continue;
            bool u = is(i-1,j), d = is(i+1,j), r = is(i,j+1), l = is(i,j-1);
            if (u) {
                if (d) {
                    if (r) tile_indexes[i][j] = l ? 8 : 7;
                    else   tile_indexes[i][j] = l ? 9 : 10;
                } else {
                    if (r) tile_indexes[i][j] = l ? 14 : 13;
                    else   tile_indexes[i][j] = l ? 15 : 16;
                }
            } else {
                if (d) {
                    if (r) tile_indexes[i][j] = l ? 2 : 1;
                    else   tile_indexes[i][j] = l ? 3 : 4;
                } else {
                    if (r) tile_indexes[i][j] = l ? 11 : 5;
                    else   tile_indexes[i][j] = l ? 17 : 6;
                }
            }
        }
    }
}

void Level::draw(const sf::Texture&              tileset,
                 const std::vector<sf::IntRect>& frames,
                 sf::RenderTexture&              target,
                 float scale, float xo, float yo) const {
    sf::Sprite sp(tileset);
    for (int i = 0; i < size_y; ++i)
        for (int j = 0; j < size_x; ++j) {
            int idx = tile_indexes[i][j];
            sp.setTextureRect(frames[idx]);
            sp.setPosition(j * 32.f * scale + xo, i * 32.f * scale + yo);
            sp.setScale(scale, scale);
            target.draw(sp);
        }
}

void Level::draw_background(const sf::Texture&              bgtex,
                             const std::vector<sf::IntRect>& bgframes,
                             sf::RenderTexture&              target,
                             float scale, float xo, float yo) const {
    sf::Sprite sp(bgtex);
    // Upper row (row 1 in visible area)
    sp.setTextureRect(bgframes[0]);
    sp.setScale(scale, scale);
    for (int j = 1; j < size_x - 1; ++j) {
        sp.setPosition(j * 32.f * scale + xo, 32.f * scale + yo);
        target.draw(sp);
    }
    // Lower row
    sp.setTextureRect(bgframes[2]);
    for (int j = 1; j < size_x - 1; ++j) {
        sp.setPosition(j * 32.f * scale + xo, (size_y - 2) * 32.f * scale + yo);
        target.draw(sp);
    }
    // Middle rows
    sp.setTextureRect(bgframes[1]);
    for (int i = 2; i < size_y - 2; ++i)
        for (int j = 1; j < size_x - 1; ++j) {
            sp.setPosition(j * 32.f * scale + xo, i * 32.f * scale + yo);
            target.draw(sp);
        }
}
