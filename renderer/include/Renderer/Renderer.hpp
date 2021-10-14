#ifndef RENDERER_H
#define RENDERER_H

#include "../Displays/DisplaySystem.hpp"
#include "../Optics/OpticsSystem.hpp"

class Renderer {
public:
    Renderer(OpticsSystem *optic_input, DisplaySystem *display_input);

    void render_max_box(int x_offset, int y_offset);
    void render_max_box_with_text(int x_offset, int y_offset, String text);

    void render_max_box_with_word(int x_offset, int y_offset, String word);

    unsigned long get_previous_place();

protected:
    OpticsSystem *optic;
    DisplaySystem *display;

    void compute_min_max();
    void compute_max_lines();

    int x_min = 0;
    int x_max = 0;
    int y_min = 0;
    int y_max = 0;
    int max_pixels_x = 0;
    int max_pixels_y = 0;

    int max_lines = 0;
};

#endif