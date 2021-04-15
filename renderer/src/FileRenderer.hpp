#ifndef FILE_RENDERER_H
#define FILE_RENDERER_H

#include "Renderer.hpp"
#include "SPIFFS.h"

#define MAX_CHARS_PER_PAGE 1000

class FileRenderer : public Renderer {
    public:
        FileRenderer(OpticsSystem* optic_input, DisplaySystem* display_input, String name_input, unsigned long place_input, unsigned long prev_input);

        // Render a page of text with the current place
        // returns the next place for reading so the calling function can
        // track place.
        unsigned long render_page_with_box(unsigned long place_in_file, int x_offset, int y_offset);


    protected:
       
        String file_name = "";
        File file_handle;

        unsigned long current_place = 0;
        unsigned long previous_place = 0;
        unsigned long next_place = 0;
        int first_go_around = 1;

        char prev_buffer[MAX_CHARS_PER_PAGE];
        char curr_buffer[MAX_CHARS_PER_PAGE];
        char next_buffer[MAX_CHARS_PER_PAGE];

        unsigned long load_prev_into_buffer();
        unsigned long load_curr_into_buffer();
        unsigned long load_next_into_buffer();

        // This does the work of lineify from the old system. Loads one page of 
        // text into that buffer given the current offset in the file
        unsigned long load_into_buffer(char* buffer, unsigned long place);

};

#endif