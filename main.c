#include <stdio.h>
#include "chip8.h"

static void print_display(const Chip8 *c) {
    for (int y = 0; y < SCREEN_H; y++) {
        for (int x = 0; x < SCREEN_W; x++) 
            putchar(c->display[y * SCREEN_W + x] ? '#' : '.');
        putchar('\n');
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <rom>\n", argv[0]);
        return 1;
    }

    Chip8 c;
    chip8_init(&c);
    if(!chip8_load_rom(&c, argv[1])) return 1;

    // printf("ROM at 0x200: ");
    // for(int i = 0; i < 16; i++)
    //     printf("%02X ", c.memory[START_ADDR + i]);
    // printf("\n");

    // printf("Font '1' at 0x%02X: ", FONT_ADDR + 5);
    // for (int i = 0; i < 5; i++)
    //     printf("%02X ", c.memory[FONT_ADDR + 5 + i]);
    // printf("\n");

    for (int i = 0; i < 5; i++) {
        //printf("pc=0x%03X ", c.pc);
        chip8_cycle(&c);
        //printf("\n");
    }
    print_display(&c);

    return 0;
}