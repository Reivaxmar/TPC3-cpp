#pragma once
#include "src/Controller.hpp"
#include <vector>
#include <queue>
#include <cmath>
#include <limits>

// OpBot: example bot with BFS pathfinding, obstacle-aware jumping,
// fish collection/throwing, incoming-fish dodging, and sudden-death escape.
class OpBot : public Controller {
    static constexpr int TS = TILE_SIZE; // 32 px per tile

public:
    void info() override {
        team_name = "OpBot";
        look  = 2;
        color = 3;
    }

    void behavior() override {
        const int mx = get_x(), my = get_y();
        const int ex = get_enemy_x(), ey = get_enemy_y();

        // Priority 1: escape closing sudden-death walls
        if (handle_sudden_death(mx, my)) return;

        // Priority 2: if we're holding a fish, go throw it
        if (is_grabbing_fish()) {
            throw_at_enemy(mx, my, ex, ey);
            return;
        }

        // Priority 3: dodge if enemy is holding a fish
        if (is_enemy_grabbing_fish()) {
            dodge_fish(mx, my);
        }

        // Priority 4: collect the best available free fish
        int tx, ty;
        if (best_free_fish(mx, my, ex, ey, tx, ty)) {
            move_to(mx, my, tx, ty);
            // Grab when close enough
            if (pixel_dist(mx, my, tx, ty) < TS * 2) grab();
        } else {
            // No fish on the field: move toward the horizontal centre
            int cx = get_level_x_pixel_size() / 2;
            move_to(mx, my, cx, my);
        }
    }

private:
    // -----------------------------------------------------------------------
    // Utilities
    // -----------------------------------------------------------------------

    static int pixel_dist(int x1, int y1, int x2, int y2) {
        int dx = x1 - x2, dy = y1 - y2;
        return (int)std::sqrt((double)(dx * dx + dy * dy));
    }

    // True when the player is standing on solid ground (can jump)
    bool on_ground() const {
        return is_pixel_ground(get_x() + TS / 2, get_y() + TS);
    }

    // -----------------------------------------------------------------------
    // BFS pathfinding on the tile grid
    // Returns the tile-space direction of the first step toward (tx, ty).
    // -----------------------------------------------------------------------
    struct Step { int dx, dy; };

    Step bfs_first_step(int mx, int my, int tx, int ty) const {
        const int rows = (int)level.size();
        const int cols  = rows ? (int)level[0].size() : 0;
        if (rows == 0 || cols == 0) return {0, 0};

        // Tile coordinates (using sprite centre)
        int fx = (mx + TS / 2) / TS, fy = (my + TS / 2) / TS;
        int gx = (tx + TS / 2) / TS, gy = (ty + TS / 2) / TS;
        if (fx == gx && fy == gy) return {0, 0};

        // Clamp to valid range
        auto clamp = [](int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); };
        fx = clamp(fx, 0, cols - 1);  fy = clamp(fy, 0, rows - 1);
        gx = clamp(gx, 0, cols - 1);  gy = clamp(gy, 0, rows - 1);

        const int N = rows * cols;
        std::vector<int> par(N, -1);
        std::vector<bool> vis(N, false);

        auto idx = [&](int c, int r) { return r * cols + c; };
        auto ok  = [&](int c, int r) {
            return c >= 0 && r >= 0 && c < cols && r < rows && level[r][c] != 'o';
        };

        static const int DX[] = {-1, 1,  0, 0};
        static const int DY[] = { 0, 0, -1, 1};

        int start = idx(fx, fy);
        vis[start] = true;
        par[start] = start;

        std::queue<int> q;
        q.push(start);

        while (!q.empty()) {
            int cur = q.front(); q.pop();
            int cx = cur % cols, cy = cur / cols;
            if (cx == gx && cy == gy) break;

            for (int d = 0; d < 4; ++d) {
                int nx = cx + DX[d], ny = cy + DY[d];
                if (!ok(nx, ny)) continue;
                int ni = idx(nx, ny);
                if (!vis[ni]) {
                    vis[ni] = true;
                    par[ni] = cur;
                    q.push(ni);
                }
            }
        }

        int dest = idx(gx, gy);
        if (!vis[dest]) {
            // No path found: aim directly
            return {(gx > fx) - (gx < fx), (gy > fy) - (gy < fy)};
        }

        // Trace parent chain back to the node right after start
        int cur = dest;
        while (par[cur] != start) {
            int prev = par[cur];
            if (prev == start) break;
            cur = prev;
        }
        return {cur % cols - fx, cur / cols - fy};
    }

    // -----------------------------------------------------------------------
    // Move toward a pixel target using BFS + obstacle-aware jumping
    // -----------------------------------------------------------------------
    void move_to(int mx, int my, int tx, int ty) {
        Step s = bfs_first_step(mx, my, tx, ty);

        if (s.dx > 0) go_right();
        if (s.dx < 0) go_left();

        // Jump if the path goes upward, or there is a wall directly ahead
        bool need_jump = (s.dy < 0);
        if (!need_jump && s.dx != 0) {
            int ahead_x = mx + s.dx * (TS + 2);
            if (is_pixel_ground(ahead_x, my + TS / 2)) need_jump = true;
        }
        if (need_jump && on_ground()) jump();
    }

    // -----------------------------------------------------------------------
    // Sudden-death wall avoidance
    // -----------------------------------------------------------------------
    bool handle_sudden_death(int mx, int my) {
        if (!is_sudden_death_active()) return false;

        const int margin = TS * 3;
        bool acted = false;

        if (mx < get_left_sudden_death() + margin) {
            go_right();
            if (is_pixel_ground(mx + TS, my + TS / 2) && on_ground()) jump();
            acted = true;
        }
        if (mx > get_right_sudden_death() - margin) {
            go_left();
            if (is_pixel_ground(mx - TS, my + TS / 2) && on_ground()) jump();
            acted = true;
        }
        return acted;
    }

    // -----------------------------------------------------------------------
    // Throw fish at the enemy
    // -----------------------------------------------------------------------
    void throw_at_enemy(int mx, int my, int ex, int ey) {
        int dx    = ex - mx;
        int hdist = std::abs(dx);
        int vdist = ey - my; // positive = enemy is below

        if (hdist < TS * 6 && std::abs(vdist) < TS * 3) {
            // Clean horizontal shot
            if (dx > 0) throw_right();
            else         throw_left();
        } else if (vdist > TS && hdist < TS * 4) {
            // We are above the enemy: bounce it down
            throw_down();
        } else {
            // Not yet in range: close the gap
            move_to(mx, my, ex, ey);
        }
    }

    // -----------------------------------------------------------------------
    // Dodge a fish that is flying toward us
    // -----------------------------------------------------------------------
    void dodge_fish(int mx, int my) {
        const auto& fps = get_list_fish_pos();
        const auto  fss = get_list_fish_state();

        for (int i = 0; i < (int)fps.size(); ++i) {
            int s = fss[i];
            if (s < 1 || s > 3) continue; // only moving fish

            int fx  = fps[i].first,  fy  = fps[i].second;
            int fdx = fx - mx,        fdy = fy - my;

            bool dangerous = false;
            // State 1 = moving right: dangerous if fish is to our left
            if (s == 1 && fdx < 0 && std::abs(fdy) < TS * 2) dangerous = true;
            // State 2 = moving left: dangerous if fish is to our right
            if (s == 2 && fdx > 0 && std::abs(fdy) < TS * 2) dangerous = true;
            // State 3 = falling: dangerous if directly above us
            if (s == 3 && std::abs(fdx) < TS * 2 && fdy < 0)  dangerous = true;

            if (dangerous && std::abs(fdx) < TS * 5) {
                // Dodge away from the fish's position
                if (fdx < 0) go_right(); else go_left();
                // Also jump sideways (except for falling fish – jump would worsen it)
                if (on_ground() && s != 3) jump();
                return;
            }
        }
    }

    // -----------------------------------------------------------------------
    // Pick the best free fish: closest to us, penalised if near the enemy
    // -----------------------------------------------------------------------
    bool best_free_fish(int mx, int my, int ex, int ey, int& tx, int& ty) const {
        const auto& fps = get_list_fish_pos();
        const auto  fss = get_list_fish_state();

        float best = std::numeric_limits<float>::max();
        tx = ty = -1;

        for (int i = 0; i < (int)fps.size(); ++i) {
            if (fss[i] < 0 || fss[i] > 3) continue; // not free

            float d_me = std::hypot((float)(fps[i].first - mx), (float)(fps[i].second - my));
            float d_en = std::hypot((float)(fps[i].first - ex), (float)(fps[i].second - ey));
            float score = d_me - 0.5f * d_en; // lower = better

            if (score < best) {
                best = score;
                tx   = fps[i].first;
                ty   = fps[i].second;
            }
        }
        return tx >= 0;
    }
};

REGISTER_BOT("opbot", OpBot)
