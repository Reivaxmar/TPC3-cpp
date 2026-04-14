#pragma once
#include <string>
#include <vector>
#include <utility>
#include <functional>
#include <map>
#include <memory>

// Tile size in pixels (shared across the whole engine)
inline constexpr int TILE_SIZE = 32;
// Frames before sudden death starts
inline constexpr int SUDDEN_DEATH_DELAY = 420;

class Controller {
public:
    // Bot settings (set inside info())
    std::string team_name = "Default Team";
    int look  = 1;  // 1-5
    int color = 1;  // 0-3

protected:
    // Action flags – cleared each frame by the game
    bool fgo_right    = false;
    bool fgo_left     = false;
    bool fjump        = false;
    bool fgrab        = false;
    bool fthrow_right = false;
    bool fthrow_left  = false;
    bool fthrow_down  = false;

    // Game-state data written by the game engine each frame
    std::vector<std::vector<char>>    level;         // tile matrix ('o' = solid, ' ' = empty)
    std::vector<std::vector<char>>    pixeled_level; // pixel-resolution map
    std::vector<std::pair<int,int>>   fish_pos;
    std::vector<int>                  fish_state;
    std::pair<int,int>                my_pos          = {0,0};
    std::pair<int,int>                other_player_pos = {0,0};
    int  game_time          = 0;
    int  my_y_speed         = 0;
    bool is_first_controller = true;

public:
    virtual ~Controller() = default;

    // Must be implemented by every bot
    virtual void info()     = 0;
    virtual void behavior() = 0;

    // Called by the game engine -------------------------------------------
    void clear();
    void update(const std::vector<std::pair<int,int>>& fp,
                const std::vector<int>&                fs,
                std::pair<int,int>                     my,
                std::pair<int,int>                     other,
                int time, int y_speed);
    void set_level(const std::vector<std::vector<char>>& lvl);
    void make_first_time_pixel_map();
    void set_is_first_controller(bool v) { is_first_controller = v; }

    // Action buttons (called from behavior()) --------------------------------
    void go_right()    { fgo_right    = true; }
    void go_left()     { fgo_left     = true; }
    void jump()        { fjump        = true; }
    void grab()        { fgrab        = true; }
    void throw_right() { fthrow_right = true; }
    void throw_left()  { fthrow_left  = true; }
    void throw_down()  { fthrow_down  = true; }

    // Getters used by the game engine ----------------------------------------
    bool get_go_right()    const { return fgo_right;    }
    bool get_go_left()     const { return fgo_left;     }
    bool get_jump()        const { return fjump;        }
    bool get_grab()        const { return fgrab;        }
    bool get_throw_right() const { return fthrow_right; }
    bool get_throw_left()  const { return fthrow_left;  }
    bool get_throw_down()  const { return fthrow_down;  }

    int         get_look()      const { return look;      }
    int         get_color()     const { return color;     }
    const std::string& get_team_name() const { return team_name; }

    // Getters available to bots (mirrors the Python Controller API) ----------
    int get_x()       const { return my_pos.first;          }
    int get_y()       const { return my_pos.second;         }
    int get_enemy_x() const { return other_player_pos.first;  }
    int get_enemy_y() const { return other_player_pos.second; }

    const std::vector<std::pair<int,int>>& get_list_fish_pos() const { return fish_pos; }
    std::vector<int> get_list_fish_state() const;

    bool is_grabbing_fish()       const;
    bool is_enemy_grabbing_fish() const;
    bool is_pixel_ground(int x, int y) const;

    int  get_level_x_pixel_size() const;
    int  get_level_y_pixel_size() const;

    std::vector<std::vector<char>> get_tile_level_matrix()  const;
    std::vector<std::vector<char>> get_pixel_level_matrix() const;

    int  get_left_sudden_death()  const;
    int  get_right_sudden_death() const;
    int  get_y_speed()            const { return my_y_speed; }
    bool is_sudden_death_active() const;
};

// ---------------------------------------------------------------------------
// Global bot registry: name -> factory function
// ---------------------------------------------------------------------------
using BotFactory = std::function<std::unique_ptr<Controller>()>;

class BotRegistry {
public:
    static BotRegistry& instance() {
        static BotRegistry reg;
        return reg;
    }
    void register_bot(const std::string& name, BotFactory factory) {
        factories_[name] = std::move(factory);
    }
    std::unique_ptr<Controller> create(const std::string& name) const {
        auto it = factories_.find(name);
        if (it == factories_.end()) return nullptr;
        return it->second();
    }
    bool exists(const std::string& name) const {
        return factories_.count(name) > 0;
    }
    const std::map<std::string, BotFactory>& all() const { return factories_; }
private:
    std::map<std::string, BotFactory> factories_;
};

#define REGISTER_BOT(name, cls)                                              \
    namespace {                                                              \
        struct cls##_registrar {                                             \
            cls##_registrar() {                                              \
                BotRegistry::instance().register_bot(                       \
                    name, []() -> std::unique_ptr<Controller> {             \
                        return std::make_unique<cls>();                     \
                    });                                                      \
            }                                                                \
        } cls##_registrar_instance;                                          \
    }
