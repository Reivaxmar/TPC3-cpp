# Bot Info – TPC3-cpp

A reference guide for every command, getter, mechanic, and convention available when writing a bot for this game.

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [How to Create a Bot](#how-to-create-a-bot)
3. [Action Commands](#action-commands)
4. [Game-State Getters](#game-state-getters)
5. [Fish States](#fish-states)
6. [Game Mechanics](#game-mechanics)
7. [Sudden Death](#sudden-death)
8. [Level Format](#level-format)
9. [Running the Game](#running-the-game)
10. [Tournament Mode](#tournament-mode)
11. [Bot Strategy Tips](#bot-strategy-tips)

---

## Quick Start

```bash
# Build (from project root)
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Run a match (must be run from project root so assets/ is accessible)
./build/TPC3 opbot example
```

---

## How to Create a Bot

1. Create a new `.hpp` file inside `submissions/`, e.g. `submissions/mybot.hpp`.
2. Include the controller header and inherit from `Controller`.
3. Implement `info()` and `behavior()`.
4. Register the bot with `REGISTER_BOT("name", ClassName)` at the bottom.
5. Add `#include "submissions/mybot.hpp"` to `main.cpp`.

```cpp
#pragma once
#include "src/Controller.hpp"

class MyBot : public Controller {
public:
    void info() override {
        team_name = "My Team";  // displayed in the HUD
        look  = 1;              // character skin: 1–5
        color = 0;              // colour palette: 0–3
    }

    void behavior() override {
        // Called once per frame (~60 fps).
        // Call any combination of the action commands below.
        go_right();
        if (get_y_speed() == 0) jump();
    }
};

REGISTER_BOT("mybot", MyBot)
```

`behavior()` is called every frame. All action flags are cleared automatically at the start of each frame, so you must call the command every frame you want it active.

---

## Action Commands

These are methods you call from inside `behavior()`.

| Method | Effect |
|---|---|
| `go_right()` | Move right (2 px/frame) |
| `go_left()` | Move left (2 px/frame) |
| `jump()` | Jump — only works when standing on solid ground |
| `grab()` | Attempt to pick up a fish you are touching |
| `throw_right()` | Release held fish; fish flies to the right |
| `throw_left()` | Release held fish; fish flies to the left |
| `throw_down()` | Release held fish; fish bounces downward (applies a jump impulse to the fish) |

> **Tip:** `throw_down()` while standing on a platform fires the fish downward. Combined with the fish's bounce physics it can curve in surprising directions.

---

## Game-State Getters

All these are available inside `behavior()`.

### Player positions (pixels)

| Method | Returns |
|---|---|
| `get_x()` | Your player's X position (top-left of sprite) |
| `get_y()` | Your player's Y position (top-left of sprite) |
| `get_enemy_x()` | Enemy's X position |
| `get_enemy_y()` | Enemy's Y position |
| `get_y_speed()` | Your current vertical speed (negative = moving up, positive = moving down / falling) |

### Fish

| Method | Returns |
|---|---|
| `get_list_fish_pos()` | `vector<pair<int,int>>` — pixel positions of all fish |
| `get_list_fish_state()` | `vector<int>` — state code of each fish (see [Fish States](#fish-states)); −1 = held by a player |
| `is_grabbing_fish()` | `true` if **you** are currently holding a fish |
| `is_enemy_grabbing_fish()` | `true` if the **enemy** is currently holding a fish |

### Level geometry

| Method | Returns |
|---|---|
| `is_pixel_ground(x, y)` | `true` if the pixel at (x, y) belongs to a solid tile (or is out of bounds) |
| `get_level_x_pixel_size()` | Total level width in pixels |
| `get_level_y_pixel_size()` | Total level height in pixels |
| `get_tile_level_matrix()` | Copy of the tile grid with `'A'`/`'B'`/`'0'`–`'3'` overlaid for players and fish |
| `get_pixel_level_matrix()` | Full pixel-resolution map (`'o'` = solid, `'A'`/`'B'` = players, `'0'`–`'3'` = fish) |

### Timing

| Method | Returns |
|---|---|
| `game_time` (protected field) | Frame counter since the round started |

### Sudden death

| Method | Returns |
|---|---|
| `is_sudden_death_active()` | `true` once the walls start closing |
| `get_left_sudden_death()` | X pixel position of the **left** death wall |
| `get_right_sudden_death()` | X pixel position of the **right** death wall |

### Bot identity

| Field / Method | Description |
|---|---|
| `is_first_controller` (protected) | `true` if you are Player 1 (A), `false` if Player 2 (B) |

---

## Fish States

Fish state codes returned by `get_list_fish_state()`:

| Code | Constant | Meaning |
|---|---|---|
| `0` | `Rest` | Stationary on the ground |
| `1` | `Right` | Moving to the right |
| `2` | `Left` | Moving to the left |
| `3` | `Falling` | In the air / falling |
| `4` | `HeldA` | Held by Player A (you if `is_first_controller`) |
| `5` | `HeldB` | Held by Player B |
| `−1` | — | Held (filtered out by `get_list_fish_state()`) |

`get_list_fish_state()` returns `−1` for states 4 and 5, so you don't need to filter held fish manually when looking for free ones — just check `state >= 0 && state <= 3`.

---

## Game Mechanics

### Coordinate system

- The top-left of the level is (0, 0).  
- X increases to the right; Y increases **downward**.
- `TILE_SIZE = 32` — each tile is 32 × 32 pixels.
- Player and fish positions are the **top-left corner** of their 32 × 32 sprite.

### Player physics

| Parameter | Value |
|---|---|
| Walk speed | 2 px/frame |
| Jump impulse | −7 px/frame (upward) |
| Max fall speed | 7 px/frame |
| Gravity increment | +1 every frame (with a sub-frame counter) |

`jump()` only takes effect when the player is on solid ground (`in_ground` flag). Calling `jump()` while airborne is silently ignored.

### Fish physics

- A fish in `Rest` (0) can be grabbed by walking into it and calling `grab()`.
- Once grabbed (`HeldA`/`HeldB`), the fish follows the player.
- Throwing launches it as a projectile (`Right`, `Left`, or `Falling`).
- A thrown fish that hits a player kills them and awards **1 point** to the thrower.
- If both players are hit simultaneously it counts as a **tie** (both get a point).
- Fish bounce off solid tiles and continue moving.

### Scoring

- 1 point is awarded to the player whose fish hits the opponent.
- The round continues through all available level files (`level_0.txt`, `level_1.txt`, …).
- Final scores are printed to stdout when all rounds finish.

### Tile grid

`level[row][col]`:

- `'o'` = solid tile (wall/floor)
- `' '` = empty space

Use `is_pixel_ground(x, y)` instead of accessing the matrix directly — it handles boundary conditions safely.

---

## Sudden Death

Sudden death starts at frame `SUDDEN_DEATH_DELAY` (420).

- Two walls of spikes close in from the left and right.
- Any player touched by a spike wall is eliminated instantly.
- `get_left_sudden_death()` / `get_right_sudden_death()` return the current pixel position of each wall.
- The walls accelerate as time passes, so act early.

```cpp
if (is_sudden_death_active()) {
    if (get_x() < get_left_sudden_death() + TILE_SIZE * 2) go_right();
    if (get_x() > get_right_sudden_death() - TILE_SIZE * 2) go_left();
}
```

---

## Level Format

Levels are plain-text files in `levels/` (e.g. `levels/level_0.txt`).

| Character | Meaning |
|---|---|
| `o` | Solid tile |
| `A` | Player 1 spawn |
| `B` | Player 2 spawn |
| `F` | Fish spawn |
| ` ` (space) | Empty air |

Example (`level_0.txt`):
```
 A         B
oo    F    oo
```

The engine pads all rows to the same length. You can create custom level sets and pass them with `--levels-dir`.

---

## Running the Game

```bash
# Normal match
./build/TPC3 <bot1> <bot2>

# Full-screen
./build/TPC3 <bot1> <bot2> --full-screen

# Custom level set (must be a subdirectory of levels/)
./build/TPC3 <bot1> <bot2> --levels-dir cuartos

# Tournament
./build/TPC3 --tournament tournament.json
./build/TPC3 --tournament tournament.json --auto   # skip manual confirmations
```

### Speed keys (during a match)

| Key | Speed |
|---|---|
| `1` | 1× (60 fps) |
| `2` | 2× (120 fps) |
| `3` | 4× (240 fps) |

### Listing registered bots

All registered bots are printed when you run the binary with no arguments or with an invalid bot name.

---

## Tournament Mode

Create a JSON config:

```json
{
  "name": "My Tournament",
  "participants": ["opbot", "example", "example2", "keyboard"],
  "levels": [".", "cuartos", "semifinales", "final"]
}
```

- `participants` — list of registered bot names.
- `levels` — one entry per round (subdirectory of `levels/`); the last entry is reused for any extra rounds.
- Bracket is generated automatically; BYEs are handled; a third-place match is played.
- In case of a tie the operator is asked to choose a winner (or use `--auto` for random resolution).

---

## Bot Strategy Tips

- **Check `on_ground` before jumping**: `is_pixel_ground(get_x() + 16, get_y() + TILE_SIZE)` returns `true` when standing on a solid tile — this mirrors the engine's `in_ground` flag and is the reliable way to know a `jump()` call will be effective.

- **BFS pathfinding**: Run BFS on the tile grid (`level[row][col]`) to find a path to a target. Convert pixel positions to tiles with `tile_x = (px + TILE_SIZE/2) / TILE_SIZE`. The 4-directional grid is a good enough approximation for most levels.

- **Fish targeting heuristic**: Score each free fish as `dist(me, fish) − α × dist(enemy, fish)`. A value of α ≈ 0.5 balances closeness to you against proximity to the enemy.

- **Throw timing**: A horizontal throw is most reliable when the enemy is within ~6 tiles horizontally and ~3 tiles vertically. If you are above the enemy, `throw_down()` creates a bouncing shot that is harder to dodge.

- **Dodging incoming fish**: Check states 1 (right), 2 (left), 3 (falling). Move away from the fish's position and jump sideways. Don't jump when the fish is directly above you (state 3) — it makes things worse.

- **Sudden death**: React before the wall reaches you. `get_left_sudden_death() + TILE_SIZE * 3` is a comfortable safety margin. Call `jump()` when there is a wall tile in the escape direction.

- **Sprite size**: Players and fish are 32 × 32 px. The "centre" of a player is approximately `(get_x() + 16, get_y() + 16)`. Use `get_x() + 16` when querying tiles for centre-based calculations.

- **`game_time`**: Use this field (protected, accessible from your bot) to implement timed behaviours, e.g. only start an aggressive push after frame 100.
