#pragma once
#include <memory>
#include <vector>
#include <array>
#include <string>
#include <SFML/Graphics.hpp>
#include "Controller.hpp"
#include "Level.hpp"
#include "Player.hpp"
#include "Fish.hpp"
#include "SuddenDeath.hpp"
#include "ImageUtils.hpp"

class Game {
public:
    Game();

    // Load a level and initialize all entities.
    // player1/player2 ownership is taken; their info() must be called before passing here.
    void set_up(const std::string& map_file,
                std::unique_ptr<Controller> player1,
                std::unique_ptr<Controller> player2,
                bool full_screen = false);

    // Run the game loop for the current level.  Returns when the round ends.
    void loop();

    const std::array<int,2>& team_points() const { return points; }

private:
    // Window
    sf::RenderWindow window;
    sf::RenderTexture base_surface;

    // Textures (loaded once)
    sf::Texture tileset_tex;
    sf::Texture bg_tex;
    sf::Texture fish_tex;
    sf::Texture sd_tex;
    bool        textures_loaded = false;

    std::vector<sf::IntRect> tileset_frames;
    std::vector<sf::IntRect> bg_frames;
    std::vector<sf::IntRect> fish_frames;
    std::vector<sf::IntRect> sd_frames;

    // Level and entities
    std::unique_ptr<Level>       current_level;
    std::unique_ptr<Player>      player_a;
    std::unique_ptr<Player>      player_b;
    std::vector<Fish>            fish;
    std::unique_ptr<SuddenDeath> sudden_death;

    // Controllers
    std::unique_ptr<Controller> controller_1;
    std::unique_ptr<Controller> controller_2;

    // State
    std::array<int,2> points    = {0, 0};
    std::array<bool,2> death    = {false, false};
    std::array<std::string,2> team_names;
    bool a_holding = false;
    bool b_holding = false;
    int  frame_rate = 60;
    int  game_time  = 0;

    // Font for HUD
    sf::Font  hud_font;
    bool      font_loaded = false;

    void load_textures();
    void draw_frame();
    void scale_and_present();
};
