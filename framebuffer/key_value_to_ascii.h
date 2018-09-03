
#define ASCII_0 48
#define ASCII_A 65
struct {
	int key;
	int ascii;
} key_ascii[] = {
	{ .key = KEY_0, .ascii = ASCII_0 },
	{ .key = KEY_1, .ascii = ASCII_0 + 1 },
	{ .key = KEY_2, .ascii = ASCII_0 + 2 },
	{ .key = KEY_3, .ascii = ASCII_0 + 3 },
	{ .key = KEY_4, .ascii = ASCII_0 + 4 },
	{ .key = KEY_5, .ascii = ASCII_0 + 5 },
	{ .key = KEY_6, .ascii = ASCII_0 + 6 },
	{ .key = KEY_7, .ascii = ASCII_0 + 7 },
	{ .key = KEY_8, .ascii = ASCII_0 + 8 },
	{ .key = KEY_9, .ascii = ASCII_0 + 9 },

	{ .key = KEY_A, .ascii = ASCII_A},
	{ .key = KEY_B, .ascii = ASCII_A + 1 },
	{ .key = KEY_C, .ascii = ASCII_A + 2 },
	{ .key = KEY_D, .ascii = ASCII_A + 3 },
	{ .key = KEY_E, .ascii = ASCII_A + 4 },
	{ .key = KEY_F, .ascii = ASCII_A + 5 },
	{ .key = KEY_G, .ascii = ASCII_A + 6 },

	{ .key = KEY_H, .ascii = ASCII_A + 7 },
	{ .key = KEY_I, .ascii = ASCII_A + 8 },
	{ .key = KEY_J, .ascii = ASCII_A + 9 },
	{ .key = KEY_K, .ascii = ASCII_A + 10 },
	{ .key = KEY_L, .ascii = ASCII_A + 11 },
	{ .key = KEY_M, .ascii = ASCII_A + 12 },
	{ .key = KEY_N, .ascii = ASCII_A + 13 },

	{ .key = KEY_O, .ascii = ASCII_A + 14 },
	{ .key = KEY_P, .ascii = ASCII_A + 15 },
	{ .key = KEY_Q, .ascii = ASCII_A + 16 },
	{ .key = KEY_R, .ascii = ASCII_A + 17 },
	{ .key = KEY_S, .ascii = ASCII_A + 18 },
	{ .key = KEY_T, .ascii = ASCII_A + 19 },

	{ .key = KEY_U, .ascii = ASCII_A + 20 },
	{ .key = KEY_V, .ascii = ASCII_A + 21 },
	{ .key = KEY_W, .ascii = ASCII_A + 22 },
	{ .key = KEY_X, .ascii = ASCII_A + 23 },
	{ .key = KEY_Y, .ascii = ASCII_A + 24 },
	{ .key = KEY_Z, .ascii = ASCII_A + 25 },

	{ .key = KEY_SPACE, .ascii = 32 },
	{ .key = KEY_BACKSPACE, .ascii = 8 },
	{ .key = KEY_ENTER, .ascii = 10 },
};
