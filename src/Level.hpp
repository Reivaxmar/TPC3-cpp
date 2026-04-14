#pragma once
#include <string>
#include <vector>
#include <SFML/Graphics.hpp>

class Level {
public:
    explicit Level(const std::string& file_path);

    void draw(const sf::Texture&         tileset,
              const std::vector<sf::IntRect>& frames,
              sf::RenderTexture&        target,
              float scale, float xo, float yo) const;

    void draw_background(const sf::Texture&         bgtex,
                         const std::vector<sf::IntRect>& bgframes,
                         sf::RenderTexture&        target,
                         float scale, float xo, float yo) const;

    int  get_width()  const { return size_x; }
    int  get_height() const { return size_y; }

    std::pair<int,int>              get_a_starting_pos()     const { return a_pos; }
    std::pair<int,int>              get_b_starting_pos()     const { return b_pos; }
    const std::vector<std::pair<int,int>>& get_fishes_starting_pos() const { return fish_pos; }
    const std::vector<std::vector<char>>& get_matrix() const { return level; }

private:
    std::vector<std::vector<char>> level;
    int size_x = 0, size_y = 0;
    std::pair<int,int>              a_pos, b_pos;
    std::vector<std::pair<int,int>> fish_pos;
    std::vector<std::vector<int>>   tile_indexes;

    void make_rectangle(std::vector<std::vector<char>>& grid);
    std::pair<int,int>              locate_tile(char ch) const;
    std::vector<std::pair<int,int>> locate_multiple(char ch) const;
    void read_level(const std::string& path);
    void set_tile_images();
};
