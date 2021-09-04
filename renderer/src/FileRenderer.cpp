#include "FileRenderer.hpp"
#include "sindarin-debug.h"

#define MAX_CHARS_PER_LINE 200

FileRenderer::FileRenderer(OpticsSystem *optic_input, DisplaySystem *display_input, Readable &readable_input)
    : Renderer(optic_input, display_input)
{

    // Pushing change to backwards-lineify branch

    // Second push
    int old_next_place;

    DEBUG_F("Attempting to open: %s\n", readable_input.path);
    file_handle = SPIFFS.open(readable_input.path);
    if (!file_handle) {
        DEBUG_LN("Failed to open file: ");
        DEBUG_VAR(readable_input.path);
        DEBUG_LN(" is the filename");
    }

    current_place = readable_input.place;
    //previous_place = readable_input.place_prev;

    previous_place = go_backwards_one_page(current_place);

    // LOAD TEXT for prev, current.
    load_prev_into_buffer();
    next_place = load_curr_into_buffer();
    old_next_place = next_place;
    load_next_into_buffer();
    next_place = old_next_place;

    screed_speed = 150;
    screed_last_time = 0;
    read_mode = PAGE_READ;
    //load_next_into_buffer();
    // HACK
    //strcpy(next_buffer, curr_buffer);
}

void FileRenderer::set_read_mode(int new_mode) {
    read_mode = new_mode;
}

int FileRenderer::get_read_mode() {
    return read_mode;
}

void FileRenderer::flip_read_mode() {
    if (read_mode == SCREED_READ)
        set_read_mode(PAGE_READ);
    else if (read_mode == PAGE_READ)
        set_read_mode(SCREED_READ);
}

void FileRenderer::increase_speed() {
    screed_speed += 10;
    
    if (screed_speed > 1000)
        screed_speed = 1000;
}

void FileRenderer::decrease_speed() {
    screed_speed -= 10;

    if (screed_speed < 50)
        screed_speed = 50;
}

void FileRenderer::set_speed(int new_speed) {
    
    screed_speed = new_speed;

    if (screed_speed < 50)
        screed_speed = 50;

    if (screed_speed > 1000)
        screed_speed = 1000;
}

unsigned long FileRenderer::render_continue_screed(int x_offset, int y_offset) {
    if (read_mode != SCREED_READ)
        return current_place;
   
    if ((millis() - screed_last_time) > (60000 / screed_speed)) {        
        char buffer[MAX_CHARS_PER_LINE];
        int plc_in_buffer = 0;
        buffer[0] = '\0';
        char cur_char;

        screed_last_time = millis();
        if (!file_handle.seek(current_place)) {

            DEBUG_LN("FAILED TO SEEK");
            DEBUG_VAR(current_place);
            DEBUG_LN(" is the place");
            return 0;
        }

        while (file_handle.available()) {
            cur_char = file_handle.read();
        
            // Turn new lines into spaces, for now.
            // But remember that it was a new line.
            switch (cur_char) {
                case '\n':
                 cur_char = ' ';
                break;
            }
            buffer[plc_in_buffer + 1] = '\0';
            buffer[plc_in_buffer] = cur_char;

            if (buffer[plc_in_buffer] == ' ') {
                buffer[plc_in_buffer] = '\0';
                render_max_box_with_word(x_offset, y_offset, String(buffer));
                DEBUG_LN(buffer);
                current_place++;
                break;
            } else {
                current_place++;
            }
            plc_in_buffer++;
        
        }
    }
    //DEBUG_LN("NEXT UP");
    return current_place;
}

unsigned long FileRenderer::get_previous_place() {
    return previous_place;
}

//unsigned long FileRenderer::render_next_page_with_box(unsigned long place_in_file, int x_offset, int y_offset) {
//
//
//}

// Show the (already loaded into buffer) current page
// and load the next page into the next buffer so we have it.
unsigned long FileRenderer::render_page_with_box(unsigned long place_in_file, int x_offset, int y_offset) {
    unsigned long next_pos = 0;


    //if (place_in_file == current_place) {
     //   DEBUG_LN("input place is same as current place");
      //  DEBUG_VAR(place_in_file);
      //  DEBUG_LN(" is place in file and current place");
       // render_max_box_with_text(x_offset, y_offset, curr_buffer);
        //DEBUG_LN(curr_buffer);
        //DEBUG_VAR(next_place);
        //DEBUG_LN(" IS THE NEXT PLACE FROM REDNER_PAGE_WITH_BOX");
        //strcpy(prev_buffer, curr_buffer);
        
        // Have to handle initial case. Barf.
        //if (first_go_around) {
        //    load_next_into_buffer();
        //    first_go_around = 0;
       // }
        //previous_place = go_backwards_one_page(current_place);
        //load_prev_into_buffer();
        //next_pos = next_place;
     //} else {
        DEBUG_LN("ALL OTHER CASES");
        DEBUG_VAR(place_in_file);
        DEBUG_LN(" is place in file THAT IS BEING REQUESTED");
        current_place = place_in_file;
        next_place = load_curr_into_buffer();
        render_max_box_with_text(x_offset, y_offset, curr_buffer);
        next_pos = next_place;
        load_next_into_buffer();
        previous_place = go_backwards_one_page(current_place);
        load_prev_into_buffer();
        DEBUG_LN(" FINISHED UP LOADING PREV AND LOADING NEXT");
     //} 
     
     /*
     else if (place_in_file == previous_place) {
        DEBUG_LN("input place is same as previous place");
        // Only do something if we haven't pressed previous twice
        if (current_place != previous_place) {
          render_max_box_with_text(x_offset, y_offset, prev_buffer);
          strcpy(next_buffer, curr_buffer);
          strcpy(curr_buffer, prev_buffer);
          next_pos = current_place;
          current_place = previous_place;
          previous_place = go_backwards_one_page(next_pos);
          load_prev_into_buffer();
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
        previous_place = go_backwards_one_page(current_place);
        load_prev_into_buffer();
    }*/
    next_place = next_pos;
    return next_pos;
}

// Seeks backwards and returns the place of one page previous. Uses a similar
// logic to the load_into_buffer function but working backwards instead.
// This function's purpose is only to find the place in the file that's one
// page previous, not to load anything into buffers. It's a little less 
// efficient, but given the load_into_buffer function works, we will opt for
// the inefficiency for now rather than rewrite the whole thing.

unsigned long FileRenderer::go_backwards_one_page(unsigned long place) {

    char cur_char = '\0';
    char main_backwards_buffer[MAX_CHARS_PER_PAGE + 1];
    unsigned long seek_place = 0;
    int num_chars_to_read;


    // Place here will point to the first character of the first word
    // on the current page. So we need to go back two from there to 
    // start with the correct word -- the last word on the previous
    // page
    // NOTE: This assumes that the previous page will have at least
    // one word. But the above return 0 and check on place == 0
    // should assure that.
    if (place < MAX_CHARS_PER_PAGE) {
        num_chars_to_read = place - 2;
        seek_place = 0;
    }
    else {
        num_chars_to_read =  MAX_CHARS_PER_PAGE;
        seek_place = place - num_chars_to_read - 2;
        if (place - num_chars_to_read - 2 < 0)
            seek_place = 0;
    }

    main_backwards_buffer[num_chars_to_read] = '\0';

    int total_chars_read = 0;

   // If we are at the beginning of the file, we stay there.
    if (place == 0)
        return 0;

    if (!file_handle.seek(seek_place)) {
        DEBUG_LN("FAILED TO SEEK IN GO BACKWARDS FUNCTION");
        DEBUG_VAR(place);
        DEBUG_LN(" is the place");
        return -1;
    }

    while (file_handle.available()) {
        if (total_chars_read < num_chars_to_read) {
          cur_char = file_handle.read();
          
          // We'll fill in the array backwards just so it's easier to read it forwards
          // to do the backwards lineify

          main_backwards_buffer[num_chars_to_read - total_chars_read - 1] = cur_char;
          total_chars_read++;
        } else {
            break;
        }
    }

    //DEBUG_LN("Looking backwards ");
    //DEBUG_VAR(total_chars_read);
    //DEBUG_LN(" is the total characters read");


    // At this point, the main_backwards_buffer will have more than a page worth of 
    // characters in backwards. So if we read forward and lineify, we should be able to
    // find the place where the previous place should start.

    // This code is mostly copied from load_into_buffer, so you know, that's
    // great.

    cur_char = '\0';
    unsigned int plc_in_buffer = 0;
    char buffer[MAX_CHARS_PER_PAGE];

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

    total_chars_read = 0;
    while (total_chars_read < num_chars_to_read) {
        cur_char = main_backwards_buffer[total_chars_read++];
        
        // Turn new lines into spaces, for now.
        switch (cur_char) {
            case '\n':
                cur_char = ' ';
                break;
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
                  total_chars_read = prev_seek_pos;
                  next_place = prev_seek_pos;
                  buffer[prev_space - chars_in_line] = '\0';
                  //DEBUG_LN(buffer);
                  //DEBUG_LN("SEE ABOVE FOR BUFFER");
                 // DEBUG_VAR(place);
                 // DEBUG_LN(" is the place");
                  //DEBUG_VAR(total_chars_read);
                  //DEBUG_LN(" are the total chars read");
                  break;
               }
          }
          else {
             // note where the previous space was in the buffer
             // as well as in the file handle and then keep going
             prev_space = plc_in_buffer;
             prev_seek_pos = total_chars_read;
             chars_in_line++;
          }
        } else {
            // keep going
            chars_in_line++;
        }
        plc_in_buffer++;
    }

    // At this point, total_chars_read is the number of characters that we need for the previous page.
    // So we should just be able to subtract that from the input place. If it's < 0, return 0.

    unsigned long previous_place_in_file;


    if ((place - total_chars_read - 2) <= 0)
        previous_place_in_file = 0;
    else
        previous_place_in_file = place - total_chars_read - 2;

    //DEBUG_LN("Ending the look backwards");
    //DEBUG_VAR(previous_place_in_file);
    //DEBUG_LN(" is the Previous place we found to return");
    
    return previous_place_in_file;
}

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
        return -1;
    }

    while (file_handle.available()) {
        cur_char = file_handle.read();

        // eliminate spaces at the beginning of the read
        // (This can happen due to heuristic to be able to go backwards arbitrarily)
        while ((plc_in_buffer == 0) && (cur_char == ' ') && file_handle.available())
            cur_char = file_handle.read();
        
        // Turn new lines into spaces, for now.
        // But remember that it was a new line.
        switch (cur_char) {
            case '\n':
                cur_char = ' ';
                break;
                /*
            case '“':
            case '”':
                cur_char = '"';
                break;
            case '—':
                cur_char = '-';
                break;
            case '‘':
            case '’':
                cur_char = '\'';
                break;
                */
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