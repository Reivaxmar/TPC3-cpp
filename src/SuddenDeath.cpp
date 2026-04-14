#include "SuddenDeath.hpp"

SuddenDeath::SuddenDeath(int level_w_tiles, int level_h_tiles)
    : level_width(level_w_tiles * 32), level_height(level_h_tiles * 32) {}

void SuddenDeath::draw_spikes(sf::RenderTexture&              target,
                               const sf::Texture&              tex,
                               const std::vector<sf::IntRect>& frames,
                               int time) const {
    // frames[0] = solid wall block, frames[1] = spike tip (pointing right)
    // Right side tip is frames[1] flipped horizontally

    sf::Sprite sp_tip(tex, frames[1]);
    sf::Sprite sp_wall(tex, frames[0]);

    for (int row = 0; row < level_height; row += 32) {
        // Left spike tip
        sp_tip.setScale(1.f, 1.f);
        sp_tip.setPosition((float)(time - SUDDEN_DEATH_DELAY), (float)row);
        target.draw(sp_tip);

        // Right spike tip (flipped)
        sp_tip.setScale(-1.f, 1.f);
        sp_tip.setPosition((float)(level_width - time + SUDDEN_DEATH_DELAY - 32 + 32), (float)row);
        target.draw(sp_tip);

        // Left wall fill
        int px = time - SUDDEN_DEATH_DELAY - 32;
        while (px > -32) {
            sp_wall.setPosition((float)px, (float)row);
            target.draw(sp_wall);
            px -= 32;
        }

        // Right wall fill
        px = level_width - time + SUDDEN_DEATH_DELAY;
        while (px < level_width) {
            sp_wall.setPosition((float)px, (float)row);
            target.draw(sp_wall);
            px += 32;
        }
    }
}

bool SuddenDeath::check_death(int time, std::pair<int,int> pos) const {
    // Left wall kills
    if ((time - SUDDEN_DEATH_DELAY + 31 - 8) > pos.first) return true;
    // Right wall kills
    if ((level_width - time + SUDDEN_DEATH_DELAY - 32 + 8) < (pos.first + 31)) return true;
    return false;
}
