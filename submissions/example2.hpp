#pragma once
#include "src/Controller.hpp"

// A second example bot identical to ExampleBot but with a different name/color
class ExampleBot2 : public Controller {
public:
    void info() override {
        team_name = "Nombre de ejemplo 2";
        look  = 3;
        color = 1;
    }
    void behavior() override {
        go_left();
        jump();
    }
};

REGISTER_BOT("example2", ExampleBot2)
