<img width="484" height="272" alt="showcase" src="https://github.com/user-attachments/assets/ec091c89-67fe-495a-bc95-560bec0c6c4d" />

### Usage
1. Generate build files: ``cmake -B build``
2. Compile the program: ``cmake --build ./build``
3. Run the executable: ``build\chip8.exe .\flightrunner.ch8`` / ``build\chip8.exe <rom_path>``

### What I learned
- I understand bit masking & shifting now far more than I did before. And can see where it's applied beyond how it works. 

- Used hex values lots but I'm still not entirely comfortable with them yet. 

### Resources used
- Great introduction: [How to Build a Console Emulator](https://www.codemotion.com/magazine/frontend/gamedev/how-to-build-an-emulator/)

- Made bit masking & shifting click: [Implementing Chip-8 Instructions](https://www.emulationonline.com/systems/chip8/implementing_chip8_instructions/)

- Specs & detailed CHIP-8 instructions list: [CHIP-8 Wikipedia](https://en.wikipedia.org/wiki/CHIP-8)

- Flightrunner ROM: [Flightrunner](https://johnearnest.github.io/chip8Archive/play.html?p=flightrunner)

- Beep test ROM: [Beep test](https://github.com/Timendus/chip8-test-suite#beep-test)

- Fontset & implementations for a few instructions I struggled on: [How to write an emulator (CHIP-8 interpreter)](https://multigesture.net/articles/how-to-write-an-emulator-chip-8-interpreter/)

### Other resources

- FX0A implementation: [Help with Fx0A - Reddit](https://www.reddit.com/r/EmuDev/comments/13p6g9m/help_with_fx0a/)

- DXYN implementation: [Chip8 DXYN instruction - Reddit](https://www.reddit.com/r/EmuDev/comments/wxqqwt/chip8_dxyn_instruction/)

- RNG in C: [How to generate a random int in C? - Stack Overflow](https://stackoverflow.com/questions/822323/how-to-generate-a-random-int-in-c)

- Passing struct parameters: [Passing struct pointer to function in c - Stack Overflow](https://stackoverflow.com/questions/10066709/passing-struct-pointer-to-function-in-c)
