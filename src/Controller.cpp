#include "Controller.hpp"
#include <algorithm>

void Controller::clear() {
    fgo_right    = false;
    fgo_left     = false;
    fjump        = false;
    fgrab        = false;
    fthrow_right = false;
    fthrow_left  = false;
    fthrow_down  = false;
}

void Controller::update(const std::vector<std::pair<int,int>>& fp,
                         const std::vector<int>&                fs,
                         std::pair<int,int>                     my,
                         std::pair<int,int>                     other,
                         int time, int y_speed) {
    fish_pos         = fp;
    fish_state       = fs;
    my_pos           = my;
    other_player_pos = other;
    game_time        = time;
    my_y_speed       = y_speed;
}

void Controller::set_level(const std::vector<std::vector<char>>& lvl) {
    level = lvl;  // deep copy
    for (auto& row : level)
        for (auto& c : row)
            if (c != 'o')
                c = ' ';
}

void Controller::make_first_time_pixel_map() {
    int cols = (int)level[0].size() * 32;
    int rows = (int)level.size() * 32;
    pixeled_level.assign(rows, std::vector<char>(cols, ' '));
    for (int i = 0; i < (int)level.size(); ++i)
        for (int j = 0; j < (int)level[i].size(); ++j)
            if (level[i][j] == 'o')
                for (int dy = 0; dy < 32; ++dy)
                    for (int dx = 0; dx < 32; ++dx)
                        pixeled_level[i * 32 + dy][j * 32 + dx] = 'o';
}

// ---------------------------------------------------------------------------

std::vector<int> Controller::get_list_fish_state() const {
    std::vector<int> out(fish_state.size(), -1);
    for (int i = 0; i < (int)fish_state.size(); ++i) {
        int s = fish_state[i];
        if (s == 0 || s == 1 || s == 2 || s == 3)
            out[i] = s;
    }
    return out;
}

bool Controller::is_grabbing_fish() const {
    int target = is_first_controller ? 4 : 5;
    for (int s : fish_state)
        if (s == target) return true;
    return false;
}

bool Controller::is_enemy_grabbing_fish() const {
    int target = is_first_controller ? 5 : 4;
    for (int s : fish_state)
        if (s == target) return true;
    return false;
}

bool Controller::is_pixel_ground(int x, int y) const {
    if (x < 0 || y < 0) return true;
    int tx = x / TILE_SIZE, ty = y / TILE_SIZE;
    if (tx >= (int)level[0].size() || ty >= (int)level.size()) return true;
    return level[ty][tx] == 'o';
}

int Controller::get_level_x_pixel_size() const {
    return (int)level[0].size() * TILE_SIZE;
}

int Controller::get_level_y_pixel_size() const {
    return (int)level.size() * TILE_SIZE;
}

std::vector<std::vector<char>> Controller::get_tile_level_matrix() const {
    auto lv = level;  // copy
    int my_tx    = (my_pos.first  + TILE_SIZE / 2) / TILE_SIZE;
    int my_ty    = (my_pos.second + TILE_SIZE / 2) / TILE_SIZE;
    int oth_tx   = (other_player_pos.first  + TILE_SIZE / 2) / TILE_SIZE;
    int oth_ty   = (other_player_pos.second + TILE_SIZE / 2) / TILE_SIZE;

    auto clampX = [&](int x){ return std::clamp(x, 0, (int)lv[0].size()-1); };
    auto clampY = [&](int y){ return std::clamp(y, 0, (int)lv.size()-1); };

    lv[clampY(my_ty)][clampX(my_tx)]   = 'A';
    lv[clampY(oth_ty)][clampX(oth_tx)] = 'B';

    for (int i = 0; i < (int)fish_pos.size(); ++i) {
        int fx = (fish_pos[i].first  + TILE_SIZE / 2) / TILE_SIZE;
        int fy = (fish_pos[i].second + TILE_SIZE / 2) / TILE_SIZE;
        int s  = fish_state[i];
        if (s == 0 || s == 1 || s == 2 || s == 3)
            lv[clampY(fy)][clampX(fx)] = (char)('0' + s);
    }
    return lv;
}

std::vector<std::vector<char>> Controller::get_pixel_level_matrix() const {
    auto lv = pixeled_level;
    int W = (int)lv[0].size();
    int H = (int)lv.size();

    auto safeSet = [&](int px, int py, char c) {
        if (px >= 0 && py >= 0 && px < W && py < H) lv[py][px] = c;
    };

    for (int dy = 0; dy < 32; ++dy)
        for (int dx = 0; dx < 32; ++dx) {
            safeSet(my_pos.first + dx,           my_pos.second + dy,           'A');
            safeSet(other_player_pos.first + dx, other_player_pos.second + dy, 'B');
        }

    for (int i = 0; i < (int)fish_pos.size(); ++i) {
        int s  = fish_state[i];
        int fx = fish_pos[i].first;
        int fy = fish_pos[i].second;
        if (s == 0) {
            for (int dx = 8; dx < 24; ++dx)
                for (int dy = 0; dy < 32; ++dy)
                    safeSet(fx + dx, fy + dy, '0');
        } else if (s == 1 || s == 2 || s == 3) {
            for (int dx = 2; dx < 31; ++dx)
                for (int dy = 3; dy < 28; ++dy)
                    safeSet(fx + dx, fy + dy, (char)('0'+s));
        }
    }
    return lv;
}

int Controller::get_left_sudden_death() const {
    return game_time - SUDDEN_DEATH_DELAY + 31 - 8;
}

int Controller::get_right_sudden_death() const {
    return get_level_x_pixel_size() - game_time + SUDDEN_DEATH_DELAY + 8 - 31;
}

bool Controller::is_sudden_death_active() const {
    return (game_time - SUDDEN_DEATH_DELAY + 31) > 0;
}
