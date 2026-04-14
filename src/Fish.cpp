#include "Fish.hpp"

Fish::Fish(std::pair<int,int> pos, const std::vector<std::vector<char>>& lvl)
    : x(pos.first), y(pos.second), level(lvl) {}

bool Fish::get_tile(int px, int py) const {
    if (py > (int)level.size() * TILE_SIZE) return true;
    int tx = px / TILE_SIZE, ty = py / TILE_SIZE;
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
    if ((y_speed <= 0 && get_tile(x + TILE_SIZE, y)) ||
        (y_speed >= 0 && get_tile(x + TILE_SIZE, y + TILE_SIZE - 1))) {
        x     = x - (x % TILE_SIZE);
        state = FishState::Left;
    } else if ((y_speed <= 0 && get_tile(x, y)) ||
               (y_speed >= 0 && get_tile(x, y + TILE_SIZE - 1))) {
        x     = x + TILE_SIZE - (x % TILE_SIZE);
        state = FishState::Right;
    }
}

void Fish::y_collision() {
    y_speed += 1;
    if (y_speed >= 0 && (get_tile(x, y + TILE_SIZE + y_speed) ||
                          get_tile(x + TILE_SIZE - 1, y + TILE_SIZE + y_speed))) {
        int tile_y = ((y + TILE_SIZE + y_speed) / TILE_SIZE) * TILE_SIZE;
        y       = tile_y - TILE_SIZE;
        y_speed = 0;
        if (state == FishState::Falling) state = FishState::Rest;
    }
    y += y_speed;
}

bool Fish::small_col(std::pair<int,int> pos) const {
    int px = pos.first, py = pos.second;
    bool xok = ((x + 8  <= px + TILE_SIZE) && (x + 8  >= px)) ||
               ((x + 23 <= px + TILE_SIZE) && (x + 24 >= px));
    bool yok = ((y + 9  <= py + TILE_SIZE) && (y + 9  >= py)) ||
               ((y + 23 <= py + TILE_SIZE) && (y + 23 >= py));
    return xok && yok;
}

bool Fish::is_hit(std::pair<int,int> pos) const {
    if (state == FishState::Rest || state == FishState::HeldA || state == FishState::HeldB)
        return false;
    int px = pos.first, py = pos.second;
    bool xok = ((x + 2  <= px + TILE_SIZE) && (x + 2  >= px)) ||
               ((x + 29 <= px + TILE_SIZE) && (x + 30 >= px));
    bool yok = ((y + 3  <= py + TILE_SIZE) && (y + 3  >= py)) ||
               ((y + 23 <= py + TILE_SIZE) && (y + 23 >= py));
    return xok && yok;
}

void Fish::kick(std::pair<int,int> pos, bool facing) {
    if (facing) {  // player faces left - kick fish to the left
        if (!get_tile(pos.first - TILE_SIZE, y)) x = pos.first - TILE_SIZE;
        state = FishState::Left;
    } else {
        if (!get_tile(pos.first + TILE_SIZE, y)) x = pos.first + TILE_SIZE;
        state = FishState::Right;
    }
}

bool Fish::behave(std::pair<int,int> a_pos, std::pair<int,int> b_pos,
                  bool a_facing, bool b_facing,
                  bool a_grab, bool b_grab,
                  bool a_down, bool a_right, bool a_left,
                  bool b_down, bool b_right, bool b_left) {
    switch (state) {
        case FishState::Rest:
            if (small_col(a_pos) && small_col(b_pos)) return true;  // simultaneous (tie)
            if (small_col(a_pos)) {
                if (a_grab) state = FishState::HeldA; else kick(a_pos, a_facing);
            } else if (small_col(b_pos)) {
                if (b_grab) state = FishState::HeldB; else kick(b_pos, b_facing);
            }
            break;
        case FishState::Right:
            go_right();
            y_collision();
            break;
        case FishState::Left:
            go_left();
            y_collision();
            break;
        case FishState::Falling:
            y_collision();
            break;
        case FishState::HeldA:
            if (a_down) {
                x = a_pos.first; y = a_pos.second + TILE_SIZE; state = FishState::Falling;
            } else if (a_right) { kick(a_pos, false);
            } else if (a_left)  { kick(a_pos, true);
            } else {
                y = a_pos.second;
                x = a_facing ? a_pos.first - 20 : a_pos.first + 20;
            }
            break;
        case FishState::HeldB:
            if (b_down) {
                x = b_pos.first; y = b_pos.second + TILE_SIZE; state = FishState::Falling;
            } else if (b_right) { kick(b_pos, false);
            } else if (b_left)  { kick(b_pos, true);
            } else {
                y = b_pos.second;
                x = b_facing ? b_pos.first - 20 : b_pos.first + 20;
            }
            break;
    }
    return false;
}

void Fish::draw(sf::RenderTexture& target,
                const sf::Texture& tex,
                const std::vector<sf::IntRect>& frames,
                bool a_facing, bool b_facing) {
    sf::Sprite sp(tex);
    bool flip      = false;
    int  frame_idx = 0;

    switch (state) {
        case FishState::Rest:    frame_idx = 0; break;
        case FishState::Right:
        case FishState::Left:    frame_idx = animation / 10 + 1; break;
        case FishState::Falling: frame_idx = 5; break;
        case FishState::HeldA:   frame_idx = 0; flip = a_facing; break;
        case FishState::HeldB:   frame_idx = 0; flip = b_facing; break;
    }

    if (frame_idx >= (int)frames.size()) frame_idx = 0;
    sp.setTextureRect(frames[frame_idx]);

    if (flip) {
        sp.setScale(-1.f, 1.f);
        sp.setPosition((float)(x + TILE_SIZE), (float)y);
    } else {
        sp.setScale(1.f, 1.f);
        sp.setPosition((float)x, (float)y);
    }
    target.draw(sp);
}
