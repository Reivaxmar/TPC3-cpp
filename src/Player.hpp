#pragma once
#include <vector>
#include <SFML/Graphics.hpp>

class Player {
public:
    Player(int x, int y, int char_index, int color_index,
           const std::vector<std::vector<char>>& level);

    void control(bool left, bool right, bool do_jump);
    void fish_jump();

    int  getX()      const { return x; }
    int  getY()      const { return y; }
    std::pair<int,int> get_pos()    const { return {x, y}; }
    bool get_facing() const { return facing; }
    int  get_y_speed() const { return y_speed; }

    void draw(sf::RenderTexture& target);

private:
    int  x, y;
    int  walk_speed   = 2;
    int  jump_speed   = -7;
    int  max_fall_speed = 7;
    int  y_speed      = 0;
    int  sub_speed    = 0;
    bool in_ground    = false;
    bool facing       = true;   // true = left (flipped), false = right
    bool walked       = false;
    int  animation    = 0;
    int  current_frame = 0;

    const std::vector<std::vector<char>>& level;

    sf::Texture                  texture;
    std::vector<sf::IntRect>     frames;
    int                          num_frames = 0;

    bool get_tile(int px, int py) const;
    void x_collision();
    void y_collision();
};
