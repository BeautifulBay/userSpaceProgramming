This is a tool for binary read/write

1. Build
	make main.c
2. Append 0xFF to output.bin(0-0x400 is already filled by 0xCC) till 0x800
	./a.out -o output.bin
3. Append input.bin to output.bin
	./a.out -o output.bin -i input.bin

output.bin
Step 1:
0x000 -- 0x400 : 0xCC

Step 2:
0x000 -- 0x400 : 0xCC
0x400 -- 0x800 : 0xFF

Step 3:
0x000 -- 0x400 : 0xCC
0x400 -- 0x800 : 0xFF
0x800 -- 0xC00 : 0x00

To check:
vi output.bin
:%!xxd 

Enjoy and have fun!
