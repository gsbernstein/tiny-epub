#include "FileRenderer.hpp"
#include "sindarin-debug.h"

FileRenderer::FileRenderer(OpticsSystem* optic_input, DisplaySystem* display_input, String name_input, unsigned long place_input, unsigned long prev_input) 
  : Renderer(optic_input, display_input) {
    file_name = name_input;
    file_handle = SPIFFS.open(file_name);
    if (!file_handle) {
        DEBUG_LN("Failed to open file: ");
        DEBUG_VAR(file_name);
        DEBUG_LN(" is the filename");
    }

    current_place = place_input;
    previous_place = prev_input;

    // LOAD TEXT for prev, current.
    load_prev_into_buffer();
    load_curr_into_buffer();

    //load_next_into_buffer();
    // HACK
    //strcpy(next_buffer, curr_buffer);
}


// Show the (already loaded into buffer) current page
// and load the next page into the next buffer so we have it.
unsigned long FileRenderer::render_page_with_box(unsigned long place_in_file, int x_offset, int y_offset) {
    unsigned long next_pos = 0;


    if (place_in_file == current_place) {
        DEBUG_LN("input place is same as current place");
        DEBUG_VAR(place_in_file);
        DEBUG_LN(" is place in file and current place");
        render_max_box_with_text(x_offset, y_offset, curr_buffer);
        //DEBUG_LN(curr_buffer);
        //DEBUG_VAR(next_place);
        //DEBUG_LN(" IS THE NEXT PLACE FROM REDNER_PAGE_WITH_BOX");
        //strcpy(prev_buffer, curr_buffer);
        
        // Have to handle initial case. Barf.
        if (first_go_around) {
            load_next_into_buffer();
            first_go_around = 0;
        }
        next_pos = next_place;
     } else if (place_in_file == previous_place) {
        DEBUG_LN("input place is same as previous place");
        // Only do something if we haven't pressed previous twice
        if (current_place != previous_place) {
          render_max_box_with_text(x_offset, y_offset, prev_buffer);
          strcpy(next_buffer, curr_buffer);
          strcpy(curr_buffer, prev_buffer);
          next_pos = current_place;
          current_place = previous_place;
        }
    } else if (place_in_file == next_place) {
        DEBUG_LN("input place is same as next place");
        previous_place = current_place;
        strcpy(prev_buffer, curr_buffer);
        strcpy(curr_buffer, next_buffer);
        current_place = next_place;
        render_max_box_with_text(x_offset, y_offset, next_buffer);
        next_pos = load_next_into_buffer();
    }
    else {
        DEBUG_VAR(next_place);
        DEBUG_LN(" DIDN'T MATCH");
        DEBUG_VAR(place_in_file);
        DEBUG_LN(" was the place passed in");
        current_place = place_in_file;
        next_pos = load_curr_into_buffer();
        render_max_box_with_text(x_offset, y_offset, curr_buffer);
    }
    next_place = next_pos;
    return next_pos;
}

#define MAX_CHARS_PER_LINE 200

// returns the new place of the file_handle
unsigned long FileRenderer::load_into_buffer(char* buffer, unsigned long place) {
    char cur_char = '\0';
    unsigned int plc_in_buffer = 0;
    //int found_newline = 0;
    int lines_in_buffer = 1;
    int pix_len = 0;
    char single_line[MAX_CHARS_PER_LINE];
    int chars_in_line = 0;

    // It's minus 4 because we want to start 2 off the top and not go below
    // 2 off the bottom due to the bounding box.
    // Note that get_text_height adds 1 over the max character height
    int max_lines = (max_pixels_y - 4) / display->get_text_height();
    int max_width = (max_pixels_x - 4);

    int prev_space = 0;
    unsigned long prev_seek_pos = 0;

    buffer[plc_in_buffer] = cur_char;
    single_line[chars_in_line] = cur_char;

    if (!file_handle.seek(place)) {

        DEBUG_LN("FAILED TO SEEK");
        DEBUG_VAR(place);
        DEBUG_LN(" is the place");
        return 0;
    }

    while (file_handle.available()) {
        cur_char = file_handle.read();
        
        // Turn new lines into spaces, for now.
        // But remember that it was a new line.
        if (cur_char == '\n') {
            //found_newline = 1;
            cur_char = ' ';
        } else {
            //found_newline = 0;
        }
        
        // Terminate and add the character to the buffer
        buffer[plc_in_buffer + 1] = '\0';
        buffer[plc_in_buffer] = cur_char;
        // Same for the single_line. Note that the single line will
        // reset at each new line but the buffer will not
        single_line[chars_in_line + 1] = '\0';
        single_line[chars_in_line] = cur_char;
        
      // Found a word break, do a check.
        if (buffer[plc_in_buffer] == ' ') {
          // Check the length of the buffer
          pix_len = display->get_text_width(single_line);
          //DEBUG_LN("Checked a width after a thing");
          //DEBUG_LN(buffer);
          if (pix_len > max_width) {
               //rewind to last word and cut it off there.
               // start new line
               buffer[prev_space] = '\n';
               // reset the line buffer by putting this last word (the word that didn't fit) in it.
               strcpy(single_line, &(buffer[prev_space + 1]));
               //DEBUG_LN("Saw new line and wrapped this word: ");
               //DEBUG_LN(single_line);
               
               chars_in_line = strlen(single_line);

               // Update the prev_space to the current place.
               prev_space += chars_in_line;

               lines_in_buffer++;
               // we're done if this is true!
               if (lines_in_buffer > max_lines) {
                  // reset the file_handle to the end of this buffer so 
                  // the next reads do the right thing.
                  file_handle.seek(prev_seek_pos);
                  next_place = prev_seek_pos;
                  buffer[prev_space - chars_in_line] = '\0';
                  //DEBUG_LN(buffer);
                  //DEBUG_LN("SEE ABOVE FOR BUFFER");
                  break;
               }
          }
          else {
             // note where the previous space was in the buffer
             // as well as in the file handle and then keep going
             prev_space = plc_in_buffer;
             prev_seek_pos = file_handle.position();
             chars_in_line++;
          }
        } else {
            // keep going
            chars_in_line++;
        }
        plc_in_buffer++;
    }

    return prev_seek_pos;
}

unsigned long FileRenderer::load_prev_into_buffer() {
    DEBUG_LN("Loading PREV into buffer");
    return load_into_buffer(prev_buffer, previous_place);
}

unsigned long FileRenderer::load_curr_into_buffer() {
    DEBUG_LN("Loading CURR into buffer");  
    return load_into_buffer(curr_buffer, current_place);
}

unsigned long FileRenderer::load_next_into_buffer() {
    DEBUG_LN("Loading NEXT into buffer");
    return load_into_buffer(next_buffer, next_place);
}