#include "hobjdump.h"

/**
 * main - the entry point for the objdump wrapper
 * @argc: the count of command-line arguments
 * @argv: the array of command-line arguments; argv[1] should contain the file
 *        path to be dumped
 * @env: the environment variables array passed to the 'objdump' command
 *
 * Return: EXIT_SUCCESS upon success, otherwise EXIT_FAILURE
 * Author: Frank Onyema Orji
 */

int main(int argc, char **argv, char **env)
{
    char *av[] = {"/usr/bin/objdump", "-s", "-f", "", NULL};

    (void)argc;
    av[3] = argv[1];

    if (execve("/usr/bin/objdump", av, env) == -1)
    {
        perror("execv");
        return (EXIT_FAILURE);
    }

    return (EXIT_SUCCESS);
}
