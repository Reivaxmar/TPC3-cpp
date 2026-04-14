#pragma once
#include "src/Controller.hpp"
#include <SFML/Window/Keyboard.hpp>

// Second keyboard-controlled bot: arrow keys + numpad
class KeyboardBot2 : public Controller {
public:
    void info() override {
        team_name = "Teclado P2";
        look  = 2;
        color = 3;
    }
    void behavior() override {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  go_left();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) go_right();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    jump();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  grab();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad1)) throw_down();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad2)) throw_right();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Numpad3)) throw_left();
    }
};

REGISTER_BOT("keyboard2", KeyboardBot2)
