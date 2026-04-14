#pragma once
#include <vector>
#include <utility>
#include <SFML/Graphics.hpp>
#include "Controller.hpp"  // for TILE_SIZE

enum class FishState {
    Rest    = 0,
    Right   = 1,
    Left    = 2,
    Falling = 3,
    HeldA   = 4,
    HeldB   = 5,
};

class Fish {
public:
    Fish(std::pair<int,int> pos, const std::vector<std::vector<char>>& level);

    // Returns true if both players were simultaneously hit (tie)
    bool behave(std::pair<int,int> a_pos, std::pair<int,int> b_pos,
                bool a_facing, bool b_facing,
                bool a_grab, bool b_grab,
                bool a_down, bool a_right, bool a_left,
                bool b_down, bool b_right, bool b_left);

    void draw(sf::RenderTexture& target,
              const sf::Texture& sprites,
              const std::vector<sf::IntRect>& frames,
              bool a_facing, bool b_facing);

    bool is_hit(std::pair<int,int> pos) const;
    bool is_grabbed_by_a() const { return state == FishState::HeldA; }
    bool is_grabbed_by_b() const { return state == FishState::HeldB; }

    std::pair<int,int> get_pos()   const { return {x, y}; }
    int                get_state() const { return static_cast<int>(state); }

private:
    int       x, y;
    int       y_speed    = 0;
    FishState state      = FishState::Rest;
    int       animation  = 0;
    int       walk_speed = 4;

    const std::vector<std::vector<char>>& level;

    bool get_tile(int px, int py) const;
    void go_right();
    void go_left();
    void x_collision();
    void y_collision();
    bool small_col(std::pair<int,int> pos) const;
    void kick(std::pair<int,int> pos, bool facing);
};
