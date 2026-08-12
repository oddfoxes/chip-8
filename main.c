#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "raylib.h"

#define MEM_SIZE 4096
#define WIDTH 64
#define HEIGHT 32
#define FONTSET_ADDR 0x050

unsigned char chip8_fontset[80] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

uint8_t keymap[16] =
{
    KEY_ONE,    KEY_TWO,    KEY_THREE,  KEY_FOUR,
    KEY_Q,      KEY_W,      KEY_E,      KEY_R,
    KEY_A,      KEY_S,      KEY_D,      KEY_F,
    KEY_Z,      KEY_X,      KEY_C,      KEY_V
};

struct CHIP8
{
    // Memory
    uint8_t mem[MEM_SIZE];

    // Stack
    uint16_t stack[12];
    uint8_t SP;

    // Registers
    uint8_t V[16];
    uint16_t PC;
    uint16_t I;

    uint8_t display[WIDTH * HEIGHT];
    uint8_t keypad[16];
    uint16_t delay_timer;
    uint16_t sound_timer;

    _Bool update_display;
};

uint8_t load_rom(struct CHIP8* chip8, const char* path)
{
    FILE *f = fopen(path, "rb");
    int c = 0;
    int i = 0;

    if (f == NULL)
    {
        printf("ROM path (%s) doesn't exist. Failed to load to memory.\n", path);

        return 1;
    }

    while ((c = fgetc(f)) != EOF)
    {
        if (0x200 + i >= MEM_SIZE)
        {
            printf("ROM too big. Failed to load to memory.\n");
            memset(chip8->mem, 0, sizeof(*chip8->mem) * MEM_SIZE);
            fclose(f);
            
            return 1;
        }

        chip8->mem[0x200 + i] = c;

        i++;
    }

    fclose(f);

    return 0;
}

void execute_instruction(struct CHIP8* chip8)
{
    uint16_t opcode = (chip8->mem[chip8->PC] << 8) | chip8->mem[chip8->PC + 1]; // opcodes are 16-bit but each memory is only 8 bit. so we read two 8 bits. 
    //                                              shifts first byte to the left half of the 2 bytes to make room for the 2nd byte

    chip8->PC += 2;

    int X = (opcode & 0x0f00) >> 8; // 2nd symbol
    int Y = (opcode & 0x00f0) >> 4; // 3rd symbol
    int NNN = opcode & 0x0fff; // last 3 symbols
    int NN = opcode & 0x00ff; // last 2 symbols
    int N = opcode & 0x000f; // last symbol

    int f = opcode & 0xf000; // first symbol

    switch (opcode & 0xf000)
    {
        case 0x0000:
            
            switch (NN)
            {
                case 0xE0:      // 00E0 (clear display)
                    memset(chip8->display, 0, sizeof(uint8_t) * WIDTH * HEIGHT);
                    break;
                case 0xEE:      // 00EE (return)
                    if (chip8->SP > 0)
                    {
                        chip8->SP--;
                        chip8->PC = chip8->stack[chip8->SP];
                        chip8->stack[chip8->SP] = 0;
                    }    

                    break;
                default:        // 0000 (?)
                    break;
            }

            break;
        case 0x1000:            // 1NNN (goto NNN)
            chip8->PC = NNN;
            break;
        case 0x2000:            // 2NNN (call subroutine at NNN)
            if (chip8->SP < 12)
            {
                chip8->stack[chip8->SP] = chip8->PC;
                chip8->SP++;
                chip8->PC = NNN;
            }    
        
            break;
        case 0x3000:            // 3XNN (if Vx == NN, skip next instruction)
            if (chip8->V[X] == NN)
                chip8->PC += 2;
            break;
        case 0x4000:            // 4XNN (if Vx != NN, skip next instruction)
            if (chip8->V[X] != NN)
                chip8->PC += 2;
            break;
        case 0x5000:            // 5XY0 (if Vx == Vy, skip next instruction)
            if (chip8->V[X] == chip8->V[Y])
                chip8->PC += 2;
            break;
        case 0x6000:            // 6XNN (set Vx to NN)
            chip8->V[X] = NN;
            break;
        case 0x7000:            // 7XNN (add NN to X)
            chip8->V[X] += NN;
            break;
        case 0x8000:
            switch (N)
            {
                case 0x0000:    // 8XY0 (set Vx to Vy)
                    chip8->V[X] = chip8->V[Y];
                    break;
                case 0x0001:    // 8XY1 (set Vx to Vx OR Vy)
                    chip8->V[X] = chip8->V[X] | chip8->V[Y];
                    break;
                case 0x0002:    // 8XY2 (set Vx to Vx AND Vy)
                    chip8->V[X] = chip8->V[X] & chip8->V[Y];
                    break;
                case 0x0003:    // 8XY3 (set Vx to Vx XOR Vy)
                    chip8->V[X] = chip8->V[X] ^ chip8->V[Y];
                    break;
                case 0x0004:    // 8XY4 (add Vy to Vx, set VF to 1 if overflow, 0 if not)
                    uint16_t sum = chip8->V[X] + chip8->V[Y];

                    chip8->V[0xF] = (sum > 255U);
                    chip8->V[X] = sum & 0xFFu;

                    break;
                case 0x0005:    // 8XY5 (subtract Vy from Vx, set VF to 1 if underflow, 0 if not)
                    chip8->V[0xF] = (chip8->V[X] > chip8->V[Y]);
                    chip8->V[X] -= chip8->V[Y];

                    break;
                case 0x0006:    // 8XY6 (store least significant bit to VF, then shift Vx right once)
                    chip8->V[0xF] = (chip8->V[X] & 0x1);
                    chip8->V[X] >>= 1;

                    break;
                case 0x0007:    // 8XY7 (set Vx to Vy minus Vx, set VF to 1 if underflow, 0 if not)
                    chip8->V[0xF] = (chip8->V[Y] > chip8->V[X]);
                    chip8->V[X] = chip8->V[Y] - chip8->V[X];

                    break;
                case 0x000E:    // 8XYE (store most significant bit to VF, then shift Vx left once)
                    chip8->V[0xF] = (chip8->V[X] >> 7) & 0x1;
                    chip8->V[X] <<= 1;

                    break;
                default:
                    break;
            }
            break;
        case 0x9000:            // 9XY0 (skip next instruction if Vx does not equal Vy)
            if (chip8->V[X] != chip8->V[Y])
                chip8->PC += 2;
            break;
        case 0xA000:            // ANNN (sets I to NNN)
            chip8->I = NNN;
            break;
        case 0xB000:            // ANNN (jump to NNN plus V0)
            chip8->PC = NNN + chip8->V[0];
            break;
        case 0xC000:            // CXNN (sets Vx to *random number* AND NN)
            // 255 + 1
            chip8->V[X] = rand() % (256) & NN; 

            break;
        case 0xD000:            // DXYN (draw)
            chip8->V[0xF] = 0;

            uint8_t VX = chip8->V[X];
            uint8_t VY = chip8->V[Y];

            for (int yl = 0; yl < N; yl++)
            {
                if (yl + VY >= HEIGHT)
                    break;

                uint32_t pxl = chip8->mem[chip8->I + yl];

                for (int xl = 0; xl < 8; xl++)
                {
                    if (xl + VX >= WIDTH)
                            break;

                    if ((pxl & (0x80 >> xl)) != 0)
                    {
                        int i = VX + xl + ((VY + yl) * WIDTH);

                        if (chip8->display[i] == 1 && chip8->V[0xF] != 1)
                        {
                            chip8->V[0xF] = 1;
                        }

                        chip8->display[i] ^= 1;
                    }
                }
            }

            chip8->update_display = 1;
            break;
        case 0xE000:
            switch (NN)
            {
                case 0x9E:      // EX9E (skips next instruction if key stored in Vx is pressed)
                    if (chip8->keypad[chip8->V[X]])
                        chip8->PC += 2;    

                    break;
                case 0xA1:      // EXA1 (skips next instruction if key stored in Vx is not pressed)
                    if (!chip8->keypad[chip8->V[X]])
                        chip8->PC += 2;

                    break;
                default:
                    break;
            }

            break;
        case 0xF000:
            switch (NN)
            {
                case 0x07:      // FX07 (sets Vx to delay timer)
                    chip8->V[X] = chip8->delay_timer;
                    break;
                case 0x0A:      // FX0A (block all instructions until next keypress, timers should continue)
                    _Bool key_pressed = 0;

                    for (uint8_t i = 0; i < 16; i++)
                    {
                        if (chip8->keypad[i])
                        {
                            chip8->V[X] = i;
                            key_pressed = 1;
                            break;
                        }
                    }

                    if (!key_pressed) // repeat instruction unless key was pressed
                        chip8->PC -= 2;

                    break;
                case 0x15:      // FX15 (set delay timer to Vx)
                    chip8->delay_timer = chip8->V[X];

                    break;
                case 0x18:      // FX18 (set sound timer to Vx)
                    chip8->sound_timer = chip8->V[X];

                    break;
                case 0x1E:      // FX1E (adds Vx to I)
                    chip8->I += chip8->V[X];

                    break;
                case 0x29:      // FX29 (?)
                    chip8->I = FONTSET_ADDR + ((chip8->V[X] & 0x0F) * 5);
                    
                    break;
                case 0x33:      // FX33 (?)
                    uint16_t I_addr = chip8->I;

                    chip8->mem[I_addr] = chip8->V[X] / 100;
                    chip8->mem[I_addr + 1] = (chip8->V[X] / 10) % 10;
                    chip8->mem[I_addr + 2] = chip8->V[X] % 10;
                    break;
                case 0x55:      // FX55 (stores from V0 to VX inclusive into memory, starting at I)
                    for (int i = 0; i <= X; i++) // note to self '=' means inclusive
                        chip8->mem[chip8->I + i] = chip8->V[i];

                    break;
                case 0x65:      // FX65 (oposite of FX55, loading from memory to registers instead)
                    for (int i = 0; i <= X; i++) // note to self '=' means inclusive
                        chip8->V[i] = chip8->mem[chip8->I + i];

                    break;
                default:
                    break;
            }

            break;
        default:
            break;
    }
}

uint64_t get_milliseconds()
{
    struct timeval time;

    gettimeofday(&time, NULL);

    return (int64_t)(time.tv_sec) * 1000 + (time.tv_usec / 1000);
}

int main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        printf("no rom path provided.\nusage: chip8.exe <rom-path>\n");
        return 1;
    }

    const int WIN_WIDTH = WIDTH * 10;
    const int WIN_HEIGHT = HEIGHT * 10;

    struct CHIP8 chip8 = {0};
    chip8.PC = 0x200;

    // failed to load?
    if (load_rom(&chip8, argv[1]) != 0)
        return 1;

    printf("ROM loaded succesfully!\n");

    // set rng seed
    srand(time(NULL));

    // load fontset
    for (int i = 0; i < 80; i++)
        chip8.mem[i + FONTSET_ADDR] = chip8_fontset[i];

    // raylib
    InitWindow(640, 320, "CHIP-8");
    SetTargetFPS(60);

    InitAudioDevice();
    Music beep = LoadMusicStream("beep.wav");

    while (chip8.PC < MEM_SIZE && !WindowShouldClose())
    {
        UpdateMusicStream(beep);
        
        // instructions 600/s
        for (int i = 0; i < 10; i++)
        {
            execute_instruction(&chip8);
        }

        // input
        for (int i = 0; i < 16; i++)
        {
            chip8.keypad[i] = IsKeyDown(keymap[i]);
        }

        // timers
        if (chip8.delay_timer > 0)
            chip8.delay_timer--;

        if (chip8.sound_timer > 0)
        {
            chip8.sound_timer--;
            
            if (!IsMusicStreamPlaying(beep))
            {
                PlayMusicStream(beep);
            }
        } else if (IsMusicStreamPlaying(beep)) {
            StopMusicStream(beep);
        }

        // raylib 2
        BeginDrawing();
        ClearBackground(BLACK);

        for (int x = 0; x < WIDTH; x++)
        {
            for (int y = 0; y < HEIGHT; y++)
            {
                if (chip8.display[x + (y * WIDTH)])
                {
                    DrawRectangle(x * (WIN_WIDTH / WIDTH), y * (WIN_HEIGHT / HEIGHT), WIN_WIDTH/WIDTH, WIN_HEIGHT/HEIGHT, WHITE);
                }
            }
        }

        EndDrawing();
    }

    UnloadMusicStream(beep);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
