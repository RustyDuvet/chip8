#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "chip8.h"

static const uint8_t FONT[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, //0
    0x20, 0x60, 0x20, 0x20, 0x70, //1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, //2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, //3
    0x90, 0x90, 0xF0, 0x10, 0x10, //4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, //5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, //6
    0xF0, 0x10, 0x20, 0x40, 0x40, //7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, //8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, //9
    0xF0, 0x90, 0xF0, 0x90, 0x90, //A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, //B
    0xF0, 0x80, 0x80, 0x80, 0xF0, //C
    0xE0, 0x90, 0x90, 0x90, 0xE0, //D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, //E
    0xF0, 0x80, 0xF0, 0x80, 0x80  //F
};

void chip8_init(Chip8 *c) {
    memset(c, 0, sizeof(*c));
    c->pc = START_ADDR;
    memcpy(&c->memory[FONT_ADDR], FONT, sizeof(FONT));
}

bool chip8_load_rom(Chip8 *c, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "error: could not open '%s'\n", path);
        return false;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "error: fseek failed on '%s'\n", path);
        fclose(f);
        return false;
    }
    long size = ftell(f);
    if (size < 0) {
        fprintf(stderr, "error: could not determine size of '%s'\n", path);
        fclose(f);
        return false;
    }
    rewind(f);

    if (size > MEM_SIZE - START_ADDR) {
        fprintf(stderr, "error: ROM is %ld bytes, max is %d\n", size, MEM_SIZE - START_ADDR);
        fclose(f);
        return false;
    }

    size_t got = fread(&c->memory[START_ADDR], 1, (size_t)size, f);
    fclose(f);

    if (got != (size_t)size) {
        fprintf(stderr, "error: short read (%zu of %ld bytes)\n", got, size);
        return false;
    }
    
    return true;
}

void chip8_cycle(Chip8 *c) {
    uint16_t opcode = (c->memory[c->pc] << 8) | c->memory[c->pc + 1];
    c->pc += 2;

    uint8_t x = (opcode >> 8) & 0xF;
    uint8_t y = (opcode >> 4) & 0xF;
    uint8_t n = opcode & 0xF;
    uint8_t nn = opcode & 0xFF;
    uint8_t nnn = opcode & 0xFFF;

    switch (opcode & 0xF000) {
        case 0x0000:
            switch (nn) {
                case 0xE0:
                    memset(c->display, 0, sizeof(c->display)); //00E0: clear the screen
                    c->draw_flag = true;
                    break;
                case 0xEE: //00EE: return from subroutine
                    if (c->sp == 0) {
                        fprintf(stderr, "stack underflow at 0x%03X\n", c->pc - 2);
                        break;
                    }
                    c->pc = c->stack[--c->sp];
                    break;
                default:
                    goto unknown;
            }
            break;

        case 0x1000: //1NNN: jump
            c->pc = nnn; 
            break;

        case 0x2000: //2NNN: call subroutine
            if (c->sp >= 16) {
                fprintf(stderr, "stack overflow at 0x%03X\n", c->pc - 2);
                break;
            }
            c->stack[c->sp++] = c->pc;
            c->pc = nnn;
            break;

        case 0x3000: if (c->V[x] == nn)       c->pc += 2; break; //3XNN
        case 0x4000: if (c->V[x] != nn)       c->pc += 2; break;  // 4XNN
        case 0x5000: if (c->V[x] == c->V[y])  c->pc += 2; break;  // 5XY0
        case 0x6000: c->V[x] = nn;            break;              // 6XNN: load
        case 0x7000: c->V[x] += nn;           break;              // 7XNN: add
        case 0x9000: if (c->V[x] != c->V[y])  c->pc += 2; break;  // 9XY0
        case 0xA000: c->I = nnn;              break;              // ANNN: set I

        case 0x8000:
            switch(n) {
                case 0x0: c->V[x]  = c->V[y]; break;   // 8XY0: VX = VY
                case 0x1: c->V[x] |= c->V[y]; break;   // 8XY1: OR
                case 0x2: c->V[x] &= c->V[y]; break;   // 8XY2: AND
                case 0x3: c->V[x] ^= c->V[y]; break;   // 8XY3: XOR

                case 0x4: {                            // 8XY4: add with carry
                    uint16_t sum = c->V[x] + c->V[y];
                    c->V[x] = sum & 0xFF;
                    c->V[0xF] = sum > 0xFF;
                    break;
                }

                case 0x5: {                            // 8XY5: VX = VX - VY
                    uint8_t flag = c->V[x] >= c->V[y];
                    c->V[x] -= c->V[y];
                    c->V[0xF] = flag;
                    break;
                }

                case 0x7: {                            // 8XY7: VX = VY - VX
                    uint8_t flag = c->V[y] >= c->V[x];
                    c->V[x] = c->V[y] - c->V[x];
                    c->V[0xF] = flag;
                    break;
                }

                case 0x6: {                            // 8XY6: shift right
                    uint8_t flag = c->V[x] & 0x1;      // the bit that falls off
                    c->V[x] >>= 1;
                    c->V[0xF] = flag;
                    break;
                }

                case 0xE: {                            // 8XYE: shift left
                    uint8_t flag = (c->V[x] >> 7) & 0x1;
                    c->V[x] <<= 1;
                    c->V[0xF] = flag;
                    break;
                }
                default: goto unknown;

            }
            break;

        
        case 0xB000: c->pc = nnn + c->V[0]; break;       // BNNN: jump with offset
        case 0xC000: c->V[x] = (rand() & 0xFF) & nn; break;  // CXNN: random AND nn

        case 0xD000: {                                  // DXYN: draw sprite
            uint8_t px = c->V[x] % SCREEN_W;
            uint8_t py = c->V[y] % SCREEN_H;
            c->V[0xF] = 0;

            for (int row = 0; row < n; row++) {
                if (py + row >= SCREEN_H) break;        // clip at the bottom edge

                // Bounds check: a bad I must not read past the end of memory.
                uint16_t addr = c->I + row;
                if (addr >= MEM_SIZE) break;
                uint8_t sprite_byte = c->memory[addr];

                for (int col = 0; col < 8; col++) {
                    if (px + col >= SCREEN_W) break;    // clip at the right edge

                    uint8_t sprite_bit = (sprite_byte >> (7 - col)) & 1;
                    if (!sprite_bit) continue;          // 0 bits change nothing

                    int idx = (py + row) * SCREEN_W + (px + col);
                    if (c->display[idx]) c->V[0xF] = 1; // lit pixel about to be erased
                    c->display[idx] ^= 1;
                }
            }
            c->draw_flag = true;
            break;
        }
        
        default:
        unknown:
            fprintf(stderr, "unknown opcode 0x%04X at 0x%03X\n", opcode, c->pc - 2);
            break;
    }
}