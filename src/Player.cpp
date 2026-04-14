#include "Player.hpp"
#include "ImageUtils.hpp"

Player::Player(int x, int y, int char_index, int color_index,
               const std::vector<std::vector<char>>& lvl)
    : x(x), y(y), level(lvl)
{
    texture = load_player_texture(char_index, color_index);
    frames  = chop_into_frames(texture, 32, 32);
    num_frames = (int)frames.size();
}

bool Player::get_tile(int px, int py) const {
    int tx = px / 32;
    int ty = py / 32;
    if (ty < 0 || ty >= (int)level.size())    return true;
    if (tx < 0 || tx >= (int)level[0].size()) return true;
    return level[ty][tx] == 'o';
}

void Player::x_collision() {
    // Right wall
    if ((y_speed <= 0 && get_tile(x + 32, y)) ||
        (y_speed >= 0 && get_tile(x + 32, y + 31)))
        x = x - (x % 32);
    // Left wall
    else if ((y_speed <= 0 && get_tile(x, y)) ||
             (y_speed >= 0 && get_tile(x, y + 31)))
        x = x + 32 - (x % 32);
}

void Player::y_collision() {
    // Falling / landing
    if (y_speed >= 0 && (get_tile(x, y + 32 + y_speed) ||
                          get_tile(x + 31, y + 32 + y_speed))) {
        int tile_y = ((y + 32 + y_speed) / 32) * 32;
        y = tile_y - 32;
        y_speed   = 0;
        in_ground = true;
    }
    // Hitting ceiling
    else if (y_speed <= 0 && (get_tile(x, y + y_speed) ||
                               get_tile(x + 31, y + y_speed))) {
        int tile_y = ((y + y_speed) / 32) * 32;
        y = tile_y + 32;
        y_speed = 0;
    }
    y += y_speed;
}

void Player::control(bool left, bool right, bool do_jump) {
    walked = false;
    if (right) { walked = true; x += walk_speed; facing = false; }
    if (left)  { walked = true; x -= walk_speed; facing = true;  }
    if (do_jump && in_ground) y_speed = jump_speed;

    if (y_speed < max_fall_speed) {
        if (sub_speed == 3 || in_ground) { ++y_speed; sub_speed = 0; }
        else                               ++sub_speed;
    }
    in_ground = false;
    x_collision();
    y_collision();
}

void Player::fish_jump() {
    y_speed = jump_speed;
}

void Player::draw(sf::RenderTexture& target) {
    // Determine which frame to show
    if (!in_ground) {
        current_frame = (y_speed < 0) ? 1 : 2;
        animation     = 0;
    } else if (walked) {
        int walk_frames = num_frames - 1;
        if (walk_frames > 0) {
            current_frame = (animation / 6) % walk_frames + 1;
            animation     = (animation + 1) % (walk_frames * 6);
        }
    } else {
        animation     = 0;
        current_frame = 0;
    }

    sf::Sprite sp(texture, frames[current_frame]);
    if (facing) {
        // Flip horizontally: set negative x-scale with offset
        sp.setScale(-1.f, 1.f);
        sp.setPosition((float)(x + 32), (float)y);
    } else {
        sp.setScale(1.f, 1.f);
        sp.setPosition((float)x, (float)y);
    }
    target.draw(sp);
}
