#include "ImageUtils.hpp"
#include <stdexcept>

std::vector<sf::IntRect> chop_into_frames(const sf::Texture& texture,
                                           int frame_w, int frame_h) {
    sf::Vector2u size = texture.getSize();
    std::vector<sf::IntRect> frames;
    for (int y = 0; y < (int)size.y; y += frame_h)
        for (int x = 0; x < (int)size.x; x += frame_w)
            frames.push_back(sf::IntRect(x, y, frame_w, frame_h));
    return frames;
}

void recolor_three(sf::Image& img,
                   sf::Color srcA, sf::Color dstA,
                   sf::Color srcB, sf::Color dstB,
                   sf::Color srcC, sf::Color dstC) {
    sf::Vector2u size = img.getSize();
    for (unsigned py = 0; py < size.y; ++py) {
        for (unsigned px = 0; px < size.x; ++px) {
            sf::Color c = img.getPixel(px, py);
            if (c.r == srcA.r && c.g == srcA.g && c.b == srcA.b)
                img.setPixel(px, py, sf::Color(dstA.r, dstA.g, dstA.b, c.a));
            else if (c.r == srcB.r && c.g == srcB.g && c.b == srcB.b)
                img.setPixel(px, py, sf::Color(dstB.r, dstB.g, dstB.b, c.a));
            else if (c.r == srcC.r && c.g == srcC.g && c.b == srcC.b)
                img.setPixel(px, py, sf::Color(dstC.r, dstC.g, dstC.b, c.a));
        }
    }
}

void fixed_recolor(sf::Image& img, int color_index) {
    sf::Color a(164, 211, 242), b(106, 155, 232), c(113, 193, 66);
    if (color_index == 1)
        recolor_three(img, a, sf::Color(199,222,151), b, sf::Color(113,193,66),  c, sf::Color(219,138,228));
    else if (color_index == 2)
        recolor_three(img, a, sf::Color(221,198,229), b, sf::Color(219,138,228), c, sf::Color(193,206,86));
    else if (color_index == 3)
        recolor_three(img, a, sf::Color(221,219,182), b, sf::Color(193,206,86),  c, sf::Color(106,155,232));
}

sf::Texture load_player_texture(int char_index, int color_index) {
    std::string path = "./assets/P" + std::to_string(char_index) + "_Walk-Sheet.png";
    sf::Image img;
    if (!img.loadFromFile(path))
        throw std::runtime_error("Cannot load player sprite: " + path);
    fixed_recolor(img, color_index);
    sf::Texture tex;
    tex.loadFromImage(img);
    return tex;
}

std::vector<sf::IntRect> load_frames(const std::string& path,
                                      sf::Texture& tex_out,
                                      int frame_w, int frame_h) {
    if (!tex_out.loadFromFile(path))
        throw std::runtime_error("Cannot load texture: " + path);
    return chop_into_frames(tex_out, frame_w, frame_h);
}
