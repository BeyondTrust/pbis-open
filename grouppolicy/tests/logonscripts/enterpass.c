/* ex: set tabstop=4 expandtab shiftwidth=4: */
#define _XOPEN_SOURCE
#define _GNU_SOURCE
#define _XPG4_2
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/signal.h>
#include <unistd.h>

/*
 * This allocates the master pseudo-terminal object. It returns the file
 * descriptor of the master terminal, and the filename of the slave terminal
 * is stored in *pts_name. Pass *pts_name to ptys_open.
 *
 * Free *pts_name afterwards
 */
int ptym_open(char **pts_name)
{
    int fdm;
    char *tmp;

    if(pts_name != NULL)
        *pts_name = NULL;

#if defined(__CYGWIN__) || defined(SOLARIS)
    if( (fdm = open("/dev/ptmx", O_RDWR)) < 0)
#else
    if( (fdm = posix_openpt(O_RDWR)) < 0)
#endif
    {
        return -1;
    }
    if(grantpt(fdm) < 0)
    {
        close(fdm);
        return -2;
    }
    if(unlockpt(fdm) < 0)
    {
        close(fdm);
        return -3;
    }
    if( (tmp = ptsname(fdm)) == NULL)
    {
        close(fdm);
        return -4;
    }

    if(pts_name)
        *pts_name = strdup(tmp);
    return fdm;
}

/* Allocates the slave pseudo-terminal. Returns the file descriptor
 */
int ptys_open(char *pts_name)
{
    int fds;

    if( (fds = open(pts_name, O_RDWR)) < 0)
        return -1;

    //I don't think it matters if these fail
#ifdef I_PUSH
    ioctl(fds, I_PUSH, "ptem");
    ioctl(fds, I_PUSH, "ldterm");
    ioctl(fds, I_PUSH, "ttcompat");
#endif

    return fds;
}

/* Forks with a new pseudo-terminal. The child process has a new
 * pseudo-terminal, but none of its file descriptors are redirected to it. The
 * file descriptor of the master pseudo-terminal is returned in *ptrfdm on the
 * parent process only.
 */
pid_t pty_fork(int *ptrfdm, int redirect_stdin)
{
    int fdm = -1;
    pid_t pid;
    char *pts_name = NULL;
    pid_t return_code = 0;
    int fds = -1;

    if( (fdm = ptym_open(&pts_name)) < 0)
    {
        fprintf(stderr, "Unable to open the master pseudo-terminal. Error %d\n", fdm);
        return_code = -1;
        goto error;
    }

    if( (pid = fork()) < 0)
    {
        fprintf(stderr, "Fork failed. Error %d\n", pid);
        return_code = -2;
        goto error;
    }
    else if(pid == 0)
    {
        //This is the child process

        //This creates a new session and process group. That is important
        //because a terminal is attached to a session.
        if(setsid() < 0)
        {
            fprintf(stderr, "Unable to create new session\n");
            return_code = -3;
            goto error;
        }

        if( (fds = ptys_open(pts_name)) < 0)
        {
            fprintf(stderr, "Unable to open slave pseudo-terminal. Error %d\n", fds);
            return_code = -4;
            goto error;
        }
        close(fdm);
        fdm = -1;
        //Only BSD systems need to have the pseudo-terminal explicitly set.
        //Other systems automatically set the terminal when the terminal was
        //opened.
#if defined(TIOCSTTY) && !defined(CIBAUD)
        if( (ioctl(fds, TIOCSCTTY, (char *)0)) < 0)
        {
            fprintf(stderr, "Error setting pseudo-terminal\n");
            return_code = -5;
            goto error;
        }
#endif
        if(redirect_stdin)
        {
            if(dup2(fds, 0) < 0)
            {
                fprintf(stderr, "Unable to redirect stdin to terminal\n");
                return_code = -6;
                goto error;
            }
        }
        else
        {
            //Keep the terminal open
            fds = -1;
        }
    }
    else
    {
        //This is the parent process
        *ptrfdm = fdm;
        //Set this to -1 so it doesn't get closed
        fdm = -1;
        return_code = pid;
    }

error:
    if(fdm > 0)
        close(fdm);
    if(pts_name != NULL)
        free(pts_name);
    if(fds > 0)
        close(fds);
    return return_code;
}

int main(int argc, char *argv[])
{
    int fdm;
    int verbose=0, show_help=0, redirect_stdin=0;
    pid_t child;
    char buffer[1024];
    ssize_t read_count;
    const char *password;
    int status;
    char c;
    int password_attempts=0;
    fd_set rfds;

    while( (c = getopt(argc, argv, "hvi")) != EOF)
    {
        switch(c)
        {
        case 'v':
            verbose = 1;
            break;
        case 'h':
            show_help = 1;
            break;
        case 'i':
            redirect_stdin = 1;
            break;
        case '?':
        default:
            fprintf(stderr, "Unrecognized option: -%c\n", optopt);
            show_help = 1;
            break;
        }
    }
    if(optind >= argc)
        show_help = 1;
    password = getenv("PASSWORD");
    if(!password && !show_help)
    {
        fprintf(stderr, "PASSWORD not set\n");
        show_help = 1;
    }
    if(show_help)
    {
        printf("enterpass [-v] [-i] program [arg1] [arg2] ...\n"
                "$PASSWORD - environmental variable containing password to send\n"
                "-v          verbose mode\n"
                "-i          redirect stdin to pseudo-terminal\n"
                "\n"
                "Runs the specified program through a pseudo-terminal. Stdin, stdout, and stderr are not redirected. The only way the inner program can access the pseudo-terminal is by opening /dev/tty. When a password prompt is detected on the pseudo-terminal, the password is passed from the environment to the terminal.\n");
        return -1;
    }

    //Fork into a child and parent process. On the parent process, fdm
    //is the file descriptor of the master pseudo-terminal. On the child
    //process, the controlling terminal is set to the pseudo-teriminal.
    if(verbose)
        fprintf(stderr, "Forking child\n");
    if( (child = pty_fork(&fdm, redirect_stdin)) < 0)
    {
        return -1;
    }
    else if(child == 0)
    {
        int error_code;
        //Run the program.
        if( (error_code = execvp(argv[optind], &argv[optind])) < 0)
        {
            fprintf(stderr, "Unable to exec %s. Error %d\n", argv[optind], error_code);
            return -1;
        }
    }

    FD_ZERO(&rfds);
    if(verbose)
        fprintf(stderr, "Starting read loop\n");
    //Continue until master terminal is closed
    while(1)
    {
        if(redirect_stdin)
            FD_SET(0, &rfds);
        FD_SET(fdm, &rfds);
        if(select(fdm + 1, &rfds, NULL, NULL, NULL) <= 0)
        {
            fprintf(stderr, "select failed\n");
            break;
        }
        if(FD_ISSET(0, &rfds))
        {
            /* Read data from stdin and write to the terminal. This is
             * because the child process has its stdin set to the
             * terminal.
             */
            read_count = read(0, buffer, sizeof(buffer)-1);
            buffer[read_count] = 0;
            if(verbose)
                fprintf(stderr, "[From stdin: %s]", buffer);
            if(read_count > 0)
            {
                int write_count = write(fdm, buffer, read_count);
                if(read_count != write_count)
                {
                    fprintf(stderr, "I expected to write %d, but I only wrote %d\n", read_count, write_count);
                    kill(child, SIGTERM);
                }
            }
        }
        if(FD_ISSET(fdm, &rfds))
        {
            //Read from the master terminal until an error occurs or the slave
            //terminal is closed (shows up as reading 0 bytes).
            read_count = read(fdm, buffer, sizeof(buffer)-1);
            if(read_count <= 0)
                break;
            buffer[read_count] = 0;
            if(verbose)
                fprintf(stderr, "[From terminal: %s]", buffer);

            if(strstr(buffer, "password:") != NULL ||
                strstr(buffer, "Password:") != NULL ||
                strstr(buffer, "Enter passphrase") != NULL)
            {
                if(password_attempts>0)
                {
                    fprintf(stderr, "wrong password\n");
                    kill(child, SIGTERM);
                    return -2;
                }
                password_attempts++;
                if(verbose)
                    fprintf(stderr, "sending password\n");
                write(fdm, password, strlen(password));
                write(fdm, "\n", 1);
            }
            else if(strstr(buffer, "continue connecting (yes/no)") != NULL)
            {
                if(verbose)
                    fprintf(stderr, "Saying yes to accept key\n");
                write(fdm, "yes\n", 4);
            }
        }
    }
    close(fdm);

    //Get child exit code and return it as our exit code.
    //Aside from this being a nice to have, if the exit code is not
    //retreived, the child will stay a zombie process.
    if(waitpid(child, &status, 0) != child)
        fprintf(stderr, "Waitpid failed\n");
    return WEXITSTATUS(status);
}
