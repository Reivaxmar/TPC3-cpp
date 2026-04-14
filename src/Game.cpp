#include "Game.hpp"
#include <iostream>
#include <filesystem>
#include <array>

// Font paths to try (common on Linux)
static const char* FONT_PATHS[] = {
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
    "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
    // Common Windows font fallback
    "C:\\Windows\\Fonts\\arial.ttf",
    // Local assets fallback (if you bundle a font into assets/)
    "./assets/DejaVuSans.ttf",
    nullptr
};

Game::Game() {
    for (int i = 0; FONT_PATHS[i]; ++i) {
        if (std::filesystem::exists(FONT_PATHS[i])) {
            if (hud_font.loadFromFile(FONT_PATHS[i])) {
                font_loaded = true;
                break;
            }
        }
    }
}

void Game::load_textures() {
    if (textures_loaded) return;
    tileset_frames = load_frames("./assets/tileset.png",    tileset_tex, 32, 32);
    bg_frames      = load_frames("./assets/background.png", bg_tex,      32, 32);
    fish_frames    = load_frames("./assets/Pufferfish.png", fish_tex,    32, 32);
    sd_frames      = load_frames("./assets/SuddenDeath.png",sd_tex,      32, 32);
    textures_loaded = true;
}

void Game::set_up(const std::string& map_file,
                  std::unique_ptr<Controller> p1,
                  std::unique_ptr<Controller> p2,
                  bool full_screen) {
    load_textures();

    current_level = std::make_unique<Level>(map_file);

    int lw = current_level->get_width()  * 32;
    int lh = current_level->get_height() * 32;
    base_surface.create(lw, lh);

    // Create or reuse window
    if (!window.isOpen()) {
        if (full_screen)
            window.create(sf::VideoMode::getDesktopMode(), "TPC3",
                          sf::Style::Fullscreen);
        else
            window.create(sf::VideoMode(800, 600), "TPC3", sf::Style::Resize);
        window.setFramerateLimit(60);
    }

    controller_1 = std::move(p1);
    controller_2 = std::move(p2);

    controller_1->set_is_first_controller(true);
    controller_2->set_is_first_controller(false);
    controller_1->set_level(current_level->get_matrix());
    controller_2->set_level(current_level->get_matrix());
    controller_1->make_first_time_pixel_map();
    controller_2->make_first_time_pixel_map();
    team_names[0] = controller_1->get_team_name();
    team_names[1] = controller_2->get_team_name();

    auto a_pos = current_level->get_a_starting_pos();
    auto b_pos = current_level->get_b_starting_pos();
    player_a = std::make_unique<Player>(a_pos.first, a_pos.second,
                                        controller_1->get_look(),
                                        controller_1->get_color(),
                                        current_level->get_matrix());
    player_b = std::make_unique<Player>(b_pos.first, b_pos.second,
                                        controller_2->get_look(),
                                        controller_2->get_color(),
                                        current_level->get_matrix());

    fish.clear();
    for (auto& fp : current_level->get_fishes_starting_pos())
        fish.emplace_back(fp, current_level->get_matrix());

    sudden_death = std::make_unique<SuddenDeath>(
        current_level->get_width(), current_level->get_height());

    a_holding  = false;
    b_holding  = false;
    death      = {false, false};
    game_time  = 0;
    frame_rate = 60;
}

void Game::loop() {
    sf::Clock clock;
    int photo_finish = 0;

    while (true) {
        // --- Events ---
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) {
                window.close();
                std::cout << "Final scores: [" << points[0] << ", " << points[1] << "]\n";
                return;
            }
        }

        // --- Speed control ---
        static constexpr int FPS_NORMAL = 60;
        static constexpr int FPS_X2     = 120;
        static constexpr int FPS_X4     = 240;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num1)) frame_rate = FPS_NORMAL;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num2)) frame_rate = FPS_X2;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Num3)) frame_rate = FPS_X4;

        // --- Logic ---
        if (!death[0] && !death[1]) {
            std::vector<std::pair<int,int>> fp;
            std::vector<int>               fs;
            for (auto& f : fish) { fp.push_back(f.get_pos()); fs.push_back(f.get_state()); }

            controller_1->clear();
            controller_2->clear();
            controller_1->update(fp, fs, player_a->get_pos(), player_b->get_pos(), game_time, player_a->get_y_speed());
            controller_2->update(fp, fs, player_b->get_pos(), player_a->get_pos(), game_time, player_b->get_y_speed());
            controller_1->behavior();
            controller_2->behavior();

            player_a->control(controller_1->get_go_left(),
                              controller_1->get_go_right(),
                              controller_1->get_jump());
            player_b->control(controller_2->get_go_left(),
                              controller_2->get_go_right(),
                              controller_2->get_jump());

            // Throws
            if (controller_1->get_throw_down()  && a_holding) { player_a->fish_jump(); a_holding = false; }
            if (controller_2->get_throw_down()  && b_holding) { player_b->fish_jump(); b_holding = false; }
            if ((controller_1->get_throw_right() || controller_1->get_throw_left()) && a_holding) a_holding = false;
            if ((controller_2->get_throw_right() || controller_2->get_throw_left()) && b_holding) b_holding = false;

            for (auto& f : fish) {
                bool tie = f.behave(
                    player_a->get_pos(), player_b->get_pos(),
                    player_a->get_facing(), player_b->get_facing(),
                    controller_1->get_grab() && !a_holding,
                    controller_2->get_grab() && !b_holding,
                    controller_1->get_throw_down(),
                    controller_1->get_throw_right(),
                    controller_1->get_throw_left(),
                    controller_2->get_throw_down(),
                    controller_2->get_throw_right(),
                    controller_2->get_throw_left());

                if (tie) { death[0] = death[1] = true; ++points[0]; ++points[1]; }
                if (f.is_grabbed_by_a()) a_holding = true;
                if (f.is_grabbed_by_b()) b_holding = true;
                if (f.is_hit(player_a->get_pos())) { death[0] = true; ++points[1]; }
                if (f.is_hit(player_b->get_pos())) { death[1] = true; ++points[0]; }
            }
            if (sudden_death->check_death(game_time, player_a->get_pos())) { death[0] = true; ++points[1]; }
            if (sudden_death->check_death(game_time, player_b->get_pos())) { death[1] = true; ++points[0]; }

            ++game_time;
        } else {
            ++photo_finish;
            if (photo_finish == 60) break;
        }

        // --- Draw ---
        draw_frame();
        scale_and_present();

        // --- FPS limit ---
        sf::Time elapsed = clock.restart();
        sf::Time target  = sf::seconds(1.f / frame_rate);
        if (elapsed < target) sf::sleep(target - elapsed);
    }
}

void Game::draw_frame() {
    base_surface.clear(sf::Color::Black);
    current_level->draw_background(bg_tex,      bg_frames,      base_surface, 1.f, 0.f, 0.f);
    current_level->draw           (tileset_tex,  tileset_frames, base_surface, 1.f, 0.f, 0.f);
    for (auto& f : fish)
        f.draw(base_surface, fish_tex, fish_frames,
               player_a->get_facing(), player_b->get_facing());
    player_a->draw(base_surface);
    player_b->draw(base_surface);
    sudden_death->draw_spikes(base_surface, sd_tex, sd_frames, game_time);

    // HUD text
    if (font_loaded) {
        std::string hud = team_names[0] + ": " + std::to_string(points[0])
                        + " - " + team_names[1] + ": " + std::to_string(points[1]);
        sf::Text txt(hud, hud_font, 14u);
        txt.setFillColor(sf::Color::White);
        txt.setPosition(4.f, 2.f);
        base_surface.draw(txt);
    }

    base_surface.display();
}

void Game::scale_and_present() {
    sf::Vector2u ws  = window.getSize();
    sf::Vector2u bs  = base_surface.getSize();
    float w_ratio    = (float)ws.x / ws.y;
    float b_ratio    = (float)bs.x / bs.y;
    float scale;
    float xo, yo;
    if (w_ratio > b_ratio) {
        scale = (float)ws.y / bs.y;
        xo    = (ws.x - bs.x * scale) / 2.f;
        yo    = 0.f;
    } else {
        scale = (float)ws.x / bs.x;
        xo    = 0.f;
        yo    = (ws.y - bs.y * scale) / 2.f;
    }
    sf::Sprite sp(base_surface.getTexture());
    sp.setScale(scale, scale);
    sp.setPosition(xo, yo);
    window.clear(sf::Color::Black);
    window.draw(sp);
    window.display();
}
