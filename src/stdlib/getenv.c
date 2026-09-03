#include <stddef.h>
#include <stdlib.h>

static char **mini_envp;

void __mini_set_envp(char **envp)
{
    mini_envp = envp;
}

char *getenv(const char *name)
{
    size_t name_length = 0;
    char **entry;

    while (name[name_length] != '\0') {
        if (name[name_length] == '=') {
            return (char *)0;
        }
        ++name_length;
    }
    if (name_length == 0 || mini_envp == (char **)0) {
        return (char *)0;
    }

    for (entry = mini_envp; *entry != (char *)0; ++entry) {
        size_t index = 0;

        while (index < name_length && (*entry)[index] == name[index]) {
            ++index;
        }
        if (index == name_length && (*entry)[index] == '=') {
            return *entry + index + 1;
        }
    }
    return (char *)0;
}
