#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>

#define MEM_SIZE 4096
#define RAND_MAX 255

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

    uint8_t display[64 * 32];
    uint8_t keypad[16];
    uint16_t delay_timer;
    uint16_t sound_timer;
};

uint8_t load_rom(struct CHIP8* chip8, const char* path)
{
    FILE *f = fopen(path, "rb");
    int c = 0;
    int i = 0;

    if (f == NULL)
    {
        printf("ROM path doesn't exist. Failed to load to memory.\n");

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
                    memset(chip8->display, 0, sizeof(uint8_t) * 64 * 32);
                    break;
                case 0xEE:      // 00EE (return)
                    // TODO: work
                    break;
                default:        // 0000 (?)
                    break;
            }

            break;
        case 0x1000:            // 1NNN (goto NNN)
            chip8->PC = NNN;
            break;
        case 0x2000:            // 2NNN (call subroutine at NNN)
            // TODO: work
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
                    chip8->V[0xF] = (chip8->V[X] + chip8->V[Y] > 255);

                    // my brain cant think of all this code at once so its probably not going to work first try
                    // but i tried my best and i wont really know if its wrong i cant check if its right cuz of why it might be wrong

                    chip8->V[X] += chip8->V[Y];
                    break;
                case 0x0005:    // 8XY5 (subtract Vy from Vx, set VF to 1 if underflow, 0 if not)
                    chip8->V[0xF] = (0 > chip8->V[X] - chip8->V[Y]);

                    chip8->V[X] -= chip8->V[Y];
                    break;
                case 0x0006:    // 8XY6 (store least significant bit to VF, then shift Vx right once)
                    chip8->V[0xF] = chip8->V[X] & 0x0F;
                    
                    chip8->V[X] >>= 1;
                    break;
                case 0x0007:    // 8XY7 (set Vx to Vy minus Vx, set VF to 1 if underflow, 0 if not)
                    chip8->V[0xF] = (0 > chip8->V[Y] - chip8->V[X]);

                    chip8->V[X] = chip8->V[Y] - chip8->V[X];
                    break;
                case 0x000E:    // 8XYE (store most significant bit to VF, then shift Vx left once)
                    chip8->V[0xF] = chip8->V[X] & 0xF0;
                    
                    chip8->V[X] <<= 1;
                    break;
                default:
                    break;
            }
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
        case 0xD000:            /* DXYN "Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels.
                                   Each row of 8 pixels is read as bit-coded starting from memory location I;
                                   I value does not change after the execution of this instruction.
                                   As described above, VF is set to 1 if any screen pixels are flipped from set to unset when the sprite is drawn,
                                   and to 0 if that does not happen." */
            // TODO: work
            break;
        case 0xE000:
            switch (NN)
            {
                case 0x9E:      // EX9E (skips next instruction if key stored in Vx is pressed)
                    // TODO: work
                    break;
                case 0xA1:      // EXA1 (skips next instruction if key stored in Vx is not pressed)
                    // TODO: work
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
                    // TODO: work
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
                    // TODO: work
                    break;
                case 0x33:      // FX33 (?)
                    // TODO: work
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

int main()
{
    struct CHIP8 chip8;

    if (load_rom(&chip8, "flightrunner.ch8") != 0)
        return 1;

    printf("ROM loaded succesfully!\n");

    srand(time(NULL));

    double last_milli = (double)get_milliseconds();
    const double TIMER_INTERVAL_MS = 1000.0 / 60.0;

    while (chip8.PC < MEM_SIZE)
    {
        if (((double)get_milliseconds() - last_milli) > TIMER_INTERVAL_MS)
        {
            last_milli += TIMER_INTERVAL_MS;

            if (chip8.delay_timer > 0)
                chip8.delay_timer--;

            if (chip8.sound_timer > 0)
            {
                // todo: play sound

                chip8.sound_timer--;
            }
        }

        printf("Delay: %d, Sound: %d\n", chip8.delay_timer, chip8.sound_timer);

        execute_instruction(&chip8);
    }
}