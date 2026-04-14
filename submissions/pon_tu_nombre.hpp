#pragma once
#include "src/Controller.hpp"

// Template bot: copy this file, rename the class and the REGISTER_BOT name,
// then implement info() and behavior().
class PonTuNombre : public Controller {
public:
    void info() override {
        team_name = "Nombre de ejemplo";
        look  = 5;
        color = 3;
    }
    void behavior() override {
        go_left();
        jump();
    }
};

REGISTER_BOT("pon_tu_nombre", PonTuNombre)
