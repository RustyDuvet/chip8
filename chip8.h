#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>

#define MEM_SIZE 4096
#define START_ADDR 0x200
#define FONT_ADDR 0x50
#define SCREEN_W 64
#define SCREEN_H 32

typedef struct {
    uint8_t memory[MEM_SIZE];
    uint8_t V[16];
    uint16_t I;
    uint16_t pc;
    uint16_t stack[16];
    uint8_t sp;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint8_t display[SCREEN_W * SCREEN_H]
    uint8_t keys[16];
} Chip8;

void chip8_init(Chip8 *c);
bool chip8_load_rom(Chip8 *c, const char *path);
