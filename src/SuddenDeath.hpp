#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include "Controller.hpp"  // for SUDDEN_DEATH_DELAY

class SuddenDeath {
public:
    SuddenDeath(int level_w_tiles, int level_h_tiles);

    void draw_spikes(sf::RenderTexture& target,
                     const sf::Texture& sprites,
                     const std::vector<sf::IntRect>& frames,
                     int time) const;

    bool check_death(int time, std::pair<int,int> pos) const;

private:
    int level_width;   // in pixels
    int level_height;  // in pixels
};
