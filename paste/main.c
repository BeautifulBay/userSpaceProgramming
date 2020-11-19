#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

struct binary {
	char file_name[32];
	FILE *file_fd;
	int (*read_binary_size)(struct binary *);
	int (*write_binary_data)(struct binary*, unsigned char, int);
	int (*write_binary_data2)(struct binary*, char name[]);
};

static int read_binary_size(struct binary* data)
{
	int size = 0;
	int cur = 0;

	/* save current location */
	cur = ftell(data->file_fd);

	/* obtain file size */
	fseek(data->file_fd, 0, SEEK_END);
	size = ftell(data->file_fd);

	/* relocated */
	fseek(data->file_fd, cur, SEEK_SET);

	return size;
}

static int write_binary_data(struct binary *data, unsigned char code, int size)
{
	int i = 0;
	fseek(data->file_fd, 0, SEEK_END);
	for (i = 0; i < size; i++)
		fwrite(&code, sizeof(unsigned char), sizeof(code), data->file_fd);
	return 0;
}

static int write_binary_data2(struct binary *data, char name[])
{
	int i = 0;
	int size = 0;
	int n_size = 0;
	int n_count = 0;
	unsigned char code = 0x0;

	struct binary *test = (struct binary *)malloc(sizeof(struct binary));
	if (test == NULL) {
		printf("%s: malloc struct binary failed\n", __func__);
		return -2;
	}

	/* Init */
	strncpy(test->file_name, name, strlen(name) + 1);
	test->read_binary_size = read_binary_size;

	/* Open */
	test->file_fd = fopen(test->file_name, "ab+");

	/* Obtain size */
	size = test->read_binary_size(test);
	n_size = sizeof(unsigned char);
	n_count = sizeof(code);

	/* Fill in */
	fseek(data->file_fd, 0, SEEK_END);
	for (i = 0; i < size; i++) {
		fread(&code, n_size, n_count, test->file_fd);
		fwrite(&code, n_size, n_count, data->file_fd);
	}

	/* Close */
	fclose(test->file_fd);
	free(test);
	return 0;
}

static void usage(char *cmd)
{
	printf("Usage:\n");
	printf("     : %s -o output.bin --> append with 0xFF to output.bin\n", cmd);
	printf("     : %s -o output.bin -i input.bin --> append with input.bin to output.bin\n", cmd);
}

int main(int argc, char *argv[])
{

	int i = 0;
	int size = 0;
	int opt = 0;
	unsigned char code = 0xFF;
	char file_name1[32] = { 0 };
	char file_name2[32] = { 0 };

	#define LENGTH 0x800

	while((opt = getopt(argc, argv, "o:i:")) != -1) {
		switch(opt) {
			case 'o':
				strncpy(file_name1, optarg, strlen(optarg) + 1);
				break;
			case 'i':
				strncpy(file_name2, optarg, strlen(optarg) + 1);
				break;
			default:
				printf("Invalid parameter!\n");
				usage(argv[0]);
				return -1;
		}
	}

	if (argc != 3 && argc != 5) {
		usage(argv[0]);
		return -1;
	}

	struct binary *test = (struct binary *)malloc(sizeof(struct binary));
	if (test == NULL) {
		printf("malloc struct binary failed\n");
		return -2;
	}

	/* Init */
	strncpy(test->file_name, file_name1, strlen(file_name1) + 1);
	test->read_binary_size = read_binary_size;
	test->write_binary_data = write_binary_data;
	test->write_binary_data2 = write_binary_data2;

	/* Open */
	test->file_fd = fopen(test->file_name, "ab+");

	/* Obtain size */
	size = test->read_binary_size(test);

	if (file_name2[0] == '\0') {
		/* Fill in 0xFF */
		printf("Fill in with 0xFF\n");
		test->write_binary_data(test, code, LENGTH - size);
	} else {
		/* Fill with file */
		printf("Fill in with %s\n", file_name2);
		test->write_binary_data2(test, file_name2);
	}

	/* Close */
	fclose(test->file_fd);
	free(test);
	return 0;
}
