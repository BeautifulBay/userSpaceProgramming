#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <test.h>

#define NAME_SIZE 32

struct cmdlist {
	const char name[NAME_SIZE];
	int (*func)(int, char *[]);
} command[] = {
	#include "../build/cmdlist.inc"
    { "", NULL }
};

void print_argv(int argc, char *argv[])
{
    int i = 0;
    printf("%s argc = %d\n", __func__, argc);
    for(i = 0; i < argc; i++) {
        printf("%s argv[%d] = %s\n", __func__, i, argv[i]);
    }
}

int test_cmd1(int argc, char *argv[])
{
    printf("1%s\n", __func__);
    print_argv(argc, argv);
    return 0;
}

int test_cmd2(int argc, char *argv[])
{
    printf("2%s\n", __func__);
    print_argv(argc, argv);
    return 0;
}

int test_cmd3(int argc, char *argv[])
{
    printf("3%s\n", __func__);
    print_argv(argc, argv);
    return 0;
}

int main(int argc, char *argv[])
{
    int i = 0;

    const char *appname = basename(argv[0]);
    //printf("appname = %s\n", appname);

    for (i = 0; command[i].name[0] != '\0'; i++) {
        if ( ! strncmp(command[i].name, appname, NAME_SIZE)  && command[i].func ) {
            (*command[i].func)(argc, argv);
        }
    }
}
