<img width="2560" height="650" alt="TPC3_banner" src="https://github.com/user-attachments/assets/c77d00b4-6bc2-4495-808a-ef3ba451ed7a" />
Por las fiestas de la escuela, desde el GDGC UVa organizamos el Torneo de Programación de Comportamientos 3, una competición en forma de torneo en la que los equipos participantes tendrán que programar el comportamiento para un bot de un juego, que se revelará el mismo día del evento. 

El bot programado se enfrentará a los bots de otros equipos por ver quién continua en el torneo y termina alzandose como el ganador!!!

Subid en el siguiente enlace vuestro fichero de comportamiento en Python antes de que empiece el torneo: <a href="https://forms.gle/vrbXj5bVzmg46sWu9" target="_blank">https://forms.gle/vrbXj5bVzmg46sWu9</a>

📍Sala Hedy Lamarr de la Escuela de Ingeniería Informática de la Universidad de Valladolid.
📆 Viernes 13 de Marzo
⏰ 16:00

Evento organizado por Daniel López Martínez y Pablo Cabrera Vara.

---

## C++ Version

This repository now includes a full C++ rewrite of the game using **CMake** and **SFML 2**.

### Requirements

- CMake >= 3.16
- SFML 2.5+ (`libsfml-dev` on Debian/Ubuntu: `sudo apt install libsfml-dev`)
- A C++17 compiler

### Build

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### Run

```bash
# Play one game between two bots (run from the project root)
./build/TPC3 example example2

# Play with a specific level set
./build/TPC3 keyboard keyboard2 --levels-dir semifinales

# Full-screen
./build/TPC3 example keyboard --full-screen

# Tournament mode
./build/TPC3 --tournament etc/test.json
./build/TPC3 --tournament etc/test.json --auto
```

Press **1 / 2 / 3** during a game to change speed (1x, 2x, 4x).

### Writing a Bot (C++)

Create a header file in `submissions/` (e.g. `submissions/mybot.hpp`):

```cpp
#pragma once
#include "src/Controller.hpp"

class MyBot : public Controller {
public:
    void info() override {
        team_name = "Mi equipo";
        look  = 1;   // sprite 1-5
        color = 2;   // colour variant 0-3
    }
    void behavior() override {
        // Actions: go_left(), go_right(), jump(), grab(),
        //          throw_left(), throw_right(), throw_down()
        // Queries: get_x(), get_y(), get_enemy_x(), get_enemy_y(),
        //          get_list_fish_pos(), get_list_fish_state(),
        //          is_grabbing_fish(), is_pixel_ground(x,y),
        //          get_left_sudden_death(), get_y_speed(), ...
        if (get_x() > get_enemy_x()) go_left();
        else                          go_right();
    }
};

REGISTER_BOT("mybot", MyBot)
```

Then `#include "submissions/mybot.hpp"` at the top of `main.cpp` and rebuild.
