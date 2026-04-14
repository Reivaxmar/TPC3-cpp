#pragma once
#include "src/Controller.hpp"
#include <SFML/Window/Keyboard.hpp>

// Keyboard-controlled bot: WASD to move, S to grab, G/H/F to throw
class KeyboardBot : public Controller {
public:
    void info() override {
        team_name = "Teclado P1";
        look  = 1;
        color = 2;
    }
    void behavior() override {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) go_left();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) go_right();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) jump();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) grab();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::G)) throw_down();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::H)) throw_right();
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::F)) throw_left();
    }
};

REGISTER_BOT("keyboard", KeyboardBot)
