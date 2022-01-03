This CHIP-8 emulator strives to be accurate to the standard CHIP-8 instruction set as documented on: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
It does not provide support for CHIP-8 extendions such as Super Chip-48.

After running the .exe, you will be prompted to provide a path to the ROM file to run.
Following this, you will be asked to specify an entry point. Most CHIP-8 programs start at 512 but some do not.
Finally, you will be prompted to input a delay value. This controls how long the game idles after each cycle.
Most games are only playable with a delay value of around 2000+, but some demos may need to run faster.
After this, the game window will open and your game will play. Key input uses the numpad digits and A-F keys.
Many CHIP8 games simply halt after reaching an end game condition. You can press the R key at any time to reset the emulator.
A reset will reset all chip states, but will not reload the memory, so undefined behavior may occur.

Unlike Rome, this was (mostly) built in a day.