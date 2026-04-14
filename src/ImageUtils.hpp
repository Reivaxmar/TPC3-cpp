#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>

// Chop a texture into equal-sized frames (left-to-right, top-to-bottom)
std::vector<sf::IntRect> chop_into_frames(const sf::Texture& texture,
                                           int frame_w, int frame_h);

// Recolor up to three exact RGB colors in a pixel buffer
void recolor_three(sf::Image& img,
                   sf::Color srcA, sf::Color dstA,
                   sf::Color srcB, sf::Color dstB,
                   sf::Color srcC, sf::Color dstC);

// Apply the fixed palette swap for the given color_index (1-3)
void fixed_recolor(sf::Image& img, int color_index);

// Load the walk-sheet for char_index (1-5) and apply color_index (0-3)
sf::Texture load_player_texture(int char_index, int color_index);

// Load and chop a spritesheet into frames
std::vector<sf::IntRect> load_frames(const std::string& path,
                                      sf::Texture& tex_out,
                                      int frame_w, int frame_h);
