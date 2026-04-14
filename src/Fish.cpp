#include "Fish.hpp"

Fish::Fish(std::pair<int,int> pos, const std::vector<std::vector<char>>& lvl)
    : x(pos.first), y(pos.second), level(lvl) {}

bool Fish::get_tile(int px, int py) const {
    if (py > (int)level.size() * 32) return true;
    int tx = px / 32, ty = py / 32;
    if (ty < 0 || ty >= (int)level.size())    return true;
    if (tx < 0 || tx >= (int)level[0].size()) return true;
    return level[ty][tx] == 'o';
}

void Fish::go_right() {
    x += walk_speed;
    animation = (animation - 1 + 40) % 40;
    x_collision();
}

void Fish::go_left() {
    x -= walk_speed;
    animation = (animation + 1) % 40;
    x_collision();
}

void Fish::x_collision() {
    if ((y_speed <= 0 && get_tile(x + 32, y)) ||
        (y_speed >= 0 && get_tile(x + 32, y + 31))) {
        x     = x - (x % 32);
        state = 2;
    } else if ((y_speed <= 0 && get_tile(x, y)) ||
               (y_speed >= 0 && get_tile(x, y + 31))) {
        x     = x + 32 - (x % 32);
        state = 1;
    }
}

void Fish::y_collision() {
    y_speed += 1;
    if (y_speed >= 0 && (get_tile(x, y + 32 + y_speed) ||
                          get_tile(x + 31, y + 32 + y_speed))) {
        int tile_y = ((y + 32 + y_speed) / 32) * 32;
        y       = tile_y - 32;
        y_speed = 0;
        if (state == 3) state = 0;
    }
    y += y_speed;
}

bool Fish::small_col(std::pair<int,int> pos) const {
    int px = pos.first, py = pos.second;
    bool xok = ((x + 8  <= px + 32) && (x + 8  >= px)) ||
               ((x + 23 <= px + 32) && (x + 24 >= px));
    bool yok = ((y + 9  <= py + 32) && (y + 9  >= py)) ||
               ((y + 23 <= py + 32) && (y + 23 >= py));
    return xok && yok;
}

bool Fish::is_hit(std::pair<int,int> pos) const {
    if (state == 0 || state == 4 || state == 5) return false;
    int px = pos.first, py = pos.second;
    bool xok = ((x + 2  <= px + 32) && (x + 2  >= px)) ||
               ((x + 29 <= px + 32) && (x + 30 >= px));
    bool yok = ((y + 3  <= py + 32) && (y + 3  >= py)) ||
               ((y + 23 <= py + 32) && (y + 23 >= py));
    return xok && yok;
}

void Fish::kick(std::pair<int,int> pos, bool facing) {
    if (facing) {  // player faces left, kick fish to left
        if (!get_tile(pos.first - 32, y)) x = pos.first - 32;
        state = 2;
    } else {
        if (!get_tile(pos.first + 32, y)) x = pos.first + 32;
        state = 1;
    }
}

bool Fish::behave(std::pair<int,int> a_pos, std::pair<int,int> b_pos,
                  bool a_facing, bool b_facing,
                  bool a_grab, bool b_grab,
                  bool a_down, bool a_right, bool a_left,
                  bool b_down, bool b_right, bool b_left) {
    if (state == 0) {
        if (small_col(a_pos) && small_col(b_pos)) return true;  // simultaneous hit
        if (small_col(a_pos)) {
            if (a_grab) state = 4; else kick(a_pos, a_facing);
        } else if (small_col(b_pos)) {
            if (b_grab) state = 5; else kick(b_pos, b_facing);
        }
    } else if (state == 1) {
        go_right();
        y_collision();
    } else if (state == 2) {
        go_left();
        y_collision();
    } else if (state == 3) {
        y_collision();
    } else if (state == 4) {
        if (a_down) {
            x = a_pos.first;
            y = a_pos.second + 32;
            state = 3;
        } else if (a_right) {
            kick(a_pos, false);
        } else if (a_left) {
            kick(a_pos, true);
        } else {
            y = a_pos.second;
            x = a_facing ? a_pos.first - 20 : a_pos.first + 20;
        }
    } else if (state == 5) {
        if (b_down) {
            x = b_pos.first;
            y = b_pos.second + 32;
            state = 3;
        } else if (b_right) {
            kick(b_pos, false);
        } else if (b_left) {
            kick(b_pos, true);
        } else {
            y = b_pos.second;
            x = b_facing ? b_pos.first - 20 : b_pos.first + 20;
        }
    }
    return false;
}

void Fish::draw(sf::RenderTexture& target,
                const sf::Texture& tex,
                const std::vector<sf::IntRect>& frames,
                bool a_facing, bool b_facing) {
    sf::Sprite sp(tex);
    bool flip = false;
    int  frame_idx = 0;

    if (state == 0) {
        frame_idx = 0;
    } else if (state == 1 || state == 2) {
        frame_idx = animation / 10 + 1;
    } else if (state == 3) {
        frame_idx = 5;
    } else if (state == 4) {
        frame_idx = 0;
        flip      = a_facing;
    } else if (state == 5) {
        frame_idx = 0;
        flip      = b_facing;
    }

    if (frame_idx >= (int)frames.size()) frame_idx = 0;
    sp.setTextureRect(frames[frame_idx]);

    if (flip) {
        sp.setScale(-1.f, 1.f);
        sp.setPosition((float)(x + 32), (float)y);
    } else {
        sp.setScale(1.f, 1.f);
        sp.setPosition((float)x, (float)y);
    }
    target.draw(sp);
}
