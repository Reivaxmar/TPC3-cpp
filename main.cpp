#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <random>
#include <cmath>
#include <cstdlib>

// Include all submissions to register their bots
#include "submissions/example.hpp"
#include "submissions/example2.hpp"
#include "submissions/keyboard.hpp"
#include "submissions/keyboard2.hpp"
#include "submissions/pon_tu_nombre.hpp"

#include "src/Game.hpp"
#include "third_party/json.hpp"

namespace fs = std::filesystem;
using json   = nlohmann::json;

// ---------------------------------------------------------------------------
// Simple argument parser
// ---------------------------------------------------------------------------
struct Args {
    std::string player1;
    std::string player2;
    bool        full_screen = false;
    std::string levels_dir  = ".";
    bool        tournament_mode = false;
    std::string tournament_cfg;
    bool        tournament_auto = false;
};

static void print_usage(const char* prog) {
    std::cerr << "Usage:\n"
              << "  " << prog << " <player1> [player2] [--full-screen] [--levels-dir <dir>]\n"
              << "  " << prog << " --tournament <config.json> [--auto]\n\n"
              << "Available bots:\n";
    for (auto& [name, _] : BotRegistry::instance().all())
        std::cerr << "  " << name << "\n";
}

static Args parse_args(int argc, char* argv[]) {
    Args a;
    std::vector<std::string> positional;
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        if (s == "--full-screen") {
            a.full_screen = true;
        } else if (s == "--levels-dir" && i + 1 < argc) {
            a.levels_dir = argv[++i];
        } else if (s == "--tournament" && i + 1 < argc) {
            a.tournament_mode = true;
            a.tournament_cfg  = argv[++i];
        } else if (s == "--auto") {
            a.tournament_auto = true;
        } else {
            positional.push_back(s);
        }
    }
    if (!a.tournament_mode) {
        if (positional.empty()) { print_usage(argv[0]); std::exit(1); }
        a.player1 = positional[0];
        a.player2 = positional.size() > 1 ? positional[1] : "example";
    }
    return a;
}

// ---------------------------------------------------------------------------
// Play a set of levels and return {score_a, score_b}
// ---------------------------------------------------------------------------
static std::array<int,2> play_levels(const std::string& p1_name,
                                      const std::string& p2_name,
                                      const std::string& levels_dir,
                                      bool full_screen) {
    auto& reg = BotRegistry::instance();

    if (!reg.exists(p1_name)) {
        std::cerr << "Error: bot '" << p1_name << "' not found.\n"; std::exit(1);
    }
    if (!reg.exists(p2_name)) {
        std::cerr << "Error: bot '" << p2_name << "' not found.\n"; std::exit(1);
    }

    Game game;
    int round_num = 0;
    std::string dir = "./levels/" + levels_dir;

    while (true) {
        std::string map = dir + "/level_" + std::to_string(round_num) + ".txt";
        if (!fs::exists(map)) break;

        auto ctrl1 = reg.create(p1_name);
        auto ctrl2 = reg.create(p2_name);
        ctrl1->info();
        ctrl2->info();
        game.set_up(map, std::move(ctrl1), std::move(ctrl2), full_screen);
        game.loop();
        ++round_num;
    }

    return game.team_points();
}

// ---------------------------------------------------------------------------
// Tournament
// ---------------------------------------------------------------------------
struct Match {
    std::string id;
    int         round = 0;
    // slot can be: a team name, or a reference to another match's winner/loser
    struct Slot {
        bool is_ref    = false;
        std::string team;    // set when it's a direct team name or BYE (empty)
        std::string match_id;
        bool want_winner = true; // true=winner, false=loser
    };
    Slot slot_a, slot_b;
    std::string team_a, team_b;
    std::string winner, loser;
    bool played = false;
};

class Tournament {
public:
    std::string              name;
    std::vector<std::string> participants;
    std::vector<std::string> levels;
    std::vector<Match>       all_matches;
    bool finished = false;

    static Tournament from_config(const std::string& cfg_path) {
        std::ifstream f(cfg_path);
        if (!f.is_open()) { std::cerr << "Cannot open " << cfg_path << "\n"; std::exit(1); }
        json data = json::parse(f);
        Tournament t;
        t.name = data["name"].get<std::string>();
        for (auto& p : data["participants"]) t.participants.push_back(p.get<std::string>());
        if (data.contains("levels"))
            for (auto& l : data["levels"]) t.levels.push_back(l.get<std::string>());
        if (t.levels.empty()) t.levels.push_back(".");
        t.build_tree();
        return t;
    }

    Match* get_match(const std::string& mid) {
        for (auto& m : all_matches) if (m.id == mid) return &m;
        return nullptr;
    }

    std::string read_slot(const Match::Slot& s) {
        if (!s.is_ref) return s.team;
        Match* src = get_match(s.match_id);
        if (!src) return "";
        return s.want_winner ? src->winner : src->loser;
    }

    void fill_teams(Match& m) {
        m.team_a = read_slot(m.slot_a);
        m.team_b = read_slot(m.slot_b);
    }

    void set_result(const std::string& mid, const std::string& winner_name,
                    bool is_auto = false) {
        Match* m = get_match(mid);
        if (!m || m->played) return;
        fill_teams(*m);
        m->winner = winner_name;
        m->loser  = (winner_name == m->team_a) ? m->team_b : m->team_a;
        m->played = true;
        (void)is_auto;
    }

    bool is_dead_end(const Match::Slot& s, const std::string& val) {
        if (!val.empty()) return false;
        if (!s.is_ref)   return true;  // BYE
        Match* src = get_match(s.match_id);
        if (!src || !src->played) return false;
        std::string res = s.want_winner ? src->winner : src->loser;
        return res.empty();
    }

    void auto_pass() {
        bool changed = true;
        while (changed) {
            changed = false;
            for (auto& m : all_matches) {
                if (m.played) continue;
                fill_teams(m);
                bool a_dead = is_dead_end(m.slot_a, m.team_a);
                bool b_dead = is_dead_end(m.slot_b, m.team_b);
                if (a_dead && b_dead) { set_result(m.id, "", true); changed = true; }
                else if (!m.team_a.empty() && b_dead) { set_result(m.id, m.team_a, true); changed = true; }
                else if (!m.team_b.empty() && a_dead) { set_result(m.id, m.team_b, true); changed = true; }
            }
        }
    }

    void sync_done() {
        finished = std::all_of(all_matches.begin(), all_matches.end(),
                               [](const Match& m){ return m.played; });
    }

    Match* next_match() {
        for (auto& m : all_matches) {
            if (m.played) continue;
            fill_teams(m);
            if (!m.team_a.empty() && !m.team_b.empty()) return &m;
        }
        return nullptr;
    }

    std::string tournament_winner() {
        for (auto it = all_matches.rbegin(); it != all_matches.rend(); ++it)
            if (it->id != "THIRD_PLACE" && it->played)
                return it->winner;
        return "TBD";
    }

    // Run a match, handling errors interactively
    void run_match(Match& m, bool full_screen,
                   const std::vector<std::string>& level_list) {
        std::string levels_dir = (m.round < (int)level_list.size())
                                    ? level_list[m.round] : level_list.back();

        while (true) {
            auto scores = play_levels(m.team_a, m.team_b, levels_dir, full_screen);
            if (scores[0] != scores[1]) {
                std::string w = (scores[0] > scores[1]) ? m.team_a : m.team_b;
                set_result(m.id, w);
                auto_pass();
                sync_done();
                std::cout << "===========================================\n"
                          << "RESULTADO: " << m.id << "  "
                          << m.team_a << " [" << scores[0] << "] - ["
                          << scores[1] << "] " << m.team_b << "\n"
                          << "GANADOR: " << w << "\n"
                          << "===========================================\n";
                return;
            }
            // Tie – ask user
            std::cout << "\n[!] EMPATE en " << m.id
                      << " (" << m.team_a << " vs " << m.team_b << ")\n"
                      << "1) Dar victoria a " << m.team_a << "\n"
                      << "2) Dar victoria a " << m.team_b << "\n"
                      << "3) Elegir ganador al azar\n"
                      << "4) Reiniciar el partido\n";
            std::string choice;
            while (choice != "1" && choice != "2" && choice != "3" && choice != "4") {
                std::cout << "Elige (1/2/3/4): ";
                std::cin >> choice;
            }
            if (choice == "4") continue;
            std::string winner;
            if (choice == "1") winner = m.team_a;
            else if (choice == "2") winner = m.team_b;
            else {
                static std::mt19937 rng(std::random_device{}());
                winner = (rng() % 2 == 0) ? m.team_a : m.team_b;
            }
            set_result(m.id, winner);
            auto_pass();
            sync_done();
            return;
        }
    }

private:
    void build_tree() {
        auto teams = participants;
        std::shuffle(teams.begin(), teams.end(), std::mt19937(std::random_device{}()));
        int size = 1;
        while (size < (int)teams.size()) size <<= 1;
        while ((int)teams.size() < size) teams.push_back("");  // BYE

        // Standard seeding order
        std::vector<int> seeds = {0};
        while ((int)seeds.size() < size) {
            int curr = (int)seeds.size();
            std::vector<int> next;
            for (int s : seeds) { next.push_back(s); next.push_back(curr*2-1-s); }
            seeds = next;
        }
        std::vector<std::string> ordered;
        for (int i : seeds) ordered.push_back(teams[i]);

        // Build first round
        std::vector<Match*> prev;
        int match_num = 1;
        for (int i = 0; i < size; i += 2) {
            Match m;
            m.id    = "R1M" + std::to_string(match_num++);
            m.round = 0;
            m.slot_a.is_ref = false; m.slot_a.team = ordered[i];
            m.slot_b.is_ref = false; m.slot_b.team = ordered[i+1];
            all_matches.push_back(m);
        }
        for (int i = (int)all_matches.size() - size/2; i < (int)all_matches.size(); ++i)
            prev.push_back(&all_matches[i]);

        // Subsequent rounds
        int round_id = 1;
        while (prev.size() > 1) {
            std::vector<Match*> curr;
            for (int i = 0; i < (int)prev.size(); i += 2) {
                Match m;
                bool is_final = (prev.size() == 2 && (i == 0 || i == (int)prev.size()-2));
                m.id    = is_final ? "FIRST_PLACE" : ("R" + std::to_string(round_id+1) + "M" + std::to_string(i/2+1));
                m.round = round_id;
                m.slot_a.is_ref = true; m.slot_a.match_id = prev[i]->id;   m.slot_a.want_winner = true;
                m.slot_b.is_ref = true; m.slot_b.match_id = prev[i+1]->id; m.slot_b.want_winner = true;
                all_matches.push_back(m);
                curr.push_back(&all_matches.back());
            }
            // Third place match
            if (prev.size() == 2) {
                Match tp;
                tp.id    = "THIRD_PLACE";
                tp.round = round_id;
                tp.slot_a.is_ref = true; tp.slot_a.match_id = prev[0]->id; tp.slot_a.want_winner = false;
                tp.slot_b.is_ref = true; tp.slot_b.match_id = prev[1]->id; tp.slot_b.want_winner = false;
                all_matches.push_back(tp);
            }
            prev = curr;
            ++round_id;
        }
        auto_pass();
        sync_done();
    }
};

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    Args args = parse_args(argc, argv);

    if (args.tournament_mode) {
        Tournament t = Tournament::from_config(args.tournament_cfg);
        while (true) {
            t.auto_pass();
            t.sync_done();
            Match* m = t.next_match();
            if (!m) break;

            if (!args.tournament_auto) {
                std::cout << "\nNext: " << m->id << "  "
                          << m->team_a << " vs " << m->team_b << "\n"
                          << "Press Enter to start...";
                std::cin.ignore();
                std::string line;
                std::getline(std::cin, line);
            }
            t.run_match(*m, args.full_screen, t.levels);
        }
        std::cout << "\n------- >>> GANADOR DEL TORNEO: "
                  << t.tournament_winner() << " <<< -------\n";
        return 0;
    }

    // Regular game mode
    auto& reg = BotRegistry::instance();
    if (!reg.exists(args.player1)) {
        std::cerr << "Error: bot '" << args.player1 << "' not found.\n";
        print_usage(argv[0]);
        return 1;
    }
    if (!reg.exists(args.player2)) {
        std::cerr << "Error: bot '" << args.player2 << "' not found.\n";
        print_usage(argv[0]);
        return 1;
    }

    Game game;
    int round_num = 0;
    while (true) {
        std::string map_path;
        if (args.levels_dir == ".")
            map_path = "./levels/level_" + std::to_string(round_num) + ".txt";
        else
            map_path = "./levels/" + args.levels_dir + "/level_" + std::to_string(round_num) + ".txt";

        if (!fs::exists(map_path)) break;

        auto ctrl1 = reg.create(args.player1);
        auto ctrl2 = reg.create(args.player2);
        ctrl1->info();
        ctrl2->info();
        game.set_up(map_path, std::move(ctrl1), std::move(ctrl2), args.full_screen);
        game.loop();
        ++round_num;
    }

    auto& pts = game.team_points();
    std::cout << "Final scores: [" << pts[0] << ", " << pts[1] << "]\n";
    return 0;
}
