#pragma once
#include "src/Controller.hpp"

// A bot that simply runs left and jumps continuously
class ExampleBot : public Controller {
public:
    void info() override {
        team_name = "Nombre de ejemplo";
        look  = 5;
        color = 2;
    }
    void behavior() override {
        go_left();
        jump();
    }
};

REGISTER_BOT("example", ExampleBot)
