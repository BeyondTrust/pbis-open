#include <config.h>

#include <err.h>
#include <dlfcn.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

#include <lw/types.h>

#include <lwerror.h>

int
main(
    int argc,
    char * const *argv
    )
{
    const char *symbol;
    char path[PATH_MAX];
    int errors = 0;
    int i;

    if (argc < 3 || (strcmp(argv[1], "enable") != 0 &&
                            strcmp(argv[1], "disable") != 0))
    {
        errx(1, "Usage: %s <enable|disable> mechanism [...]");
    }

    openlog(argv[0], LOG_PERROR, LOG_USER);

    if (argv[1][0] == 'e')
    {
        symbol = "Enable";
    }
    else
    {
        symbol = "Disable";
    }

    for (i = 2; i < argc; ++i)
    {
        void *mechanism;
        int (*function)(void);
        DWORD error;

        snprintf(path, sizeof(path), "%s/%s.so", AUTH_MECHANISM_DIR, argv[2]);

        mechanism = dlopen(path, RTLD_NOW | RTLD_LOCAL);
        if (mechanism == NULL)
        {
            fprintf(stderr, "%s\n", dlerror());
            ++errors;
            continue;
        }

        function = dlsym(mechanism, symbol);
        if (function != NULL)
        {
            error = function();
            if (error != LW_ERROR_SUCCESS)
            {
                ++errors;
            }
        }
        else
        {
            fprintf(stderr, "%s\n", dlerror());
            ++errors;
        }

        dlclose(mechanism);
    }

    return errors;
}
