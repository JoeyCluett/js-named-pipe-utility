/*
    Author: Joey Cluett

    Date Created: 1/3/2016

    Date Last Modified: 1/4/2016

    Purpose:
        This header file contains all functions needed to simulate a virtual 4x4 keypad
        It communicates with Index 0.5 Interpreter through a simplex named pipe
            made with the NamedPipeUtility library

    Note:
        This header and its' implementation have ben deprecated
*/

#ifndef HB_KEYBOARD_H
#define HB_KEYBOARD_H

#include "NamedPipeUtility.h"
#include <iostream>
#include <SDL.h>

//button pad dimensions
#define BUTTON_WIDTH   100
#define BUTTON_SPACING 20
#define EDGE_PADDING   10

struct point {
  int x;
  int y;
};

struct button {
    point UL;
    Uint32 color;
    bool clicked;
};

button b_array[16];

unsigned char button_output[16] = {0x00,  0x01,  0x02,  0x03,

                                   0x04,  0x05,  0x06,  0x07,

                                   0x08,  0x09,  0x0A,  0x0B,

                                   0x0C,  0x0D,  0x0E,  0x0F};

void update_keypad(SDL_Rect* rect, SDL_Surface* scr) {
    rect->h = BUTTON_WIDTH;
    rect->w = BUTTON_WIDTH;

    for(int i = 0; i < 16; i++) {
        rect->x = b_array[i].UL.x;
        rect->y = b_array[i].UL.y;
        SDL_FillRect(scr, rect, b_array[i].color);
    }

    SDL_Flip(scr);
}

int is_button_pressed(point* pt, Uint32 clicked, SDL_Surface* scr) { //modifies button state if button is pressed over virtual button
    for(int i = 0; i < 16; i++) {
        if(pt->x >= b_array[i].UL.x &&
                pt->y >= b_array[i].UL.y) {
            if(pt->x < b_array[i].UL.x + BUTTON_WIDTH &&
                    pt->y < b_array[i].UL.y + BUTTON_WIDTH) {
                b_array[i].color = clicked;
                return i;
            }
        }
    }
    return -1;
}

void reset_button_color(Uint32 unclicked) {
    for(int i = 0; i < 16; i++) {
        b_array[i].color = unclicked;
    }
}

void* keyboard_fork(void* reg) {

    std::cout << "*********************Fork created**********************" << std::endl;

    //open up a simplex named pipe to transfer information to index
    NamedPipeUtility keyboardwriter;
    keyboardwriter.Simplex_create("/tmp/keyboardnpu", false);
    keyboardwriter.Simplex_open(1);

    if(SDL_Init(SDL_INIT_EVERYTHING) < 0){
        std::cout << "SDL failed to initialize" << std::endl;
        SDL_Quit();
    }

    //calculate button pad width based on button dimensions above
    int pad_width = 0;
    pad_width += (4 * BUTTON_WIDTH);
    pad_width += (3 * BUTTON_SPACING);
    pad_width += (2 * EDGE_PADDING); //2 in each direction

    SDL_Surface* screen = SDL_SetVideoMode(pad_width, pad_width, 24, SDL_SWSURFACE);

    Uint32    red   = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00); //not clicked
    Uint32 dark_red = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00); //clicked

    int index = 0;

    for(int x = EDGE_PADDING; x < pad_width; x += (BUTTON_SPACING + BUTTON_WIDTH)) {
        for(int y = EDGE_PADDING; y < pad_width; y += (BUTTON_SPACING + BUTTON_WIDTH)) {
            b_array[index].UL.x = x;
            b_array[index].UL.y = y;
            b_array[index].clicked = false;
            b_array[index].color = red; //buttons turn dark red when clicked
            index++;
        }
    }

    SDL_Rect rect;

    update_keypad(&rect, screen); //supplying a rect to avoid having to make a new one each time (a time consuming operation)

    bool is_button_down = false;
    point mouse_pos;
    unsigned char zero = 0x00;

    //begin loop testing for keypresses
    while(1) {

        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            keyboardwriter.Simplex_write(button_output[index]);
            if(event.type == SDL_MOUSEBUTTONDOWN) {
                is_button_down = true;
                SDL_GetMouseState(&mouse_pos.x, &mouse_pos.y);
                index = is_button_pressed(&mouse_pos, dark_red, screen);
            } else if(event.type == SDL_MOUSEBUTTONUP) {
                is_button_down = false;
                reset_button_color(red);
            }
        }

        update_keypad(&rect, screen);

        if(index == -1) {
            keyboardwriter.Simplex_write(zero);
        } else {
            keyboardwriter.Simplex_write(button_output[index]);
        }

    }
    //end of keyboard loop
}

#endif // HB_KEYBOARD_H
