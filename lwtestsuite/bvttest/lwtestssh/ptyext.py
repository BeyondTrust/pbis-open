"""Pseudo terminal utilities."""

# Bugs: No signal handling.  Doesn't set slave termios and window size.
#       Only tested on Linux.
# See:  W. Richard Stevens. 1992.  Advanced Programming in the
#       UNIX Environment.  Chapter 19.
# Author: Steen Lumholt -- with additions by Guido.

from select import select, error
import os

# Absurd:  import termios and then delete it.  This is to force an attempt
# to import pty to raise an ImportError on platforms that lack termios.
# Without this explicit import of termios here, some other module may
# import tty first, which in turn imports termios and dies with an
# ImportError then.  But since tty *does* exist across platforms, that
# leaves a damaged module object for tty in sys.modules, and the import
# of tty here then appears to work despite that the tty imported is junk.
import termios
del termios

import tty

__all__ = ["openpty","fork","spawn","th_spawn","popen2"]

STDIN_FILENO = 0
STDOUT_FILENO = 1
STDERR_FILENO = 2

CHILD = 0

def openpty():
    """openpty() -> (master_fd, slave_fd)
    Open a pty master/slave pair, using os.openpty() if possible."""

    try:
        return os.openpty()
    except (AttributeError, OSError):
        pass
    master_fd, slave_name = _open_terminal()
    slave_fd = slave_open(slave_name)
    return master_fd, slave_fd

def master_open():
    """master_open() -> (master_fd, slave_name)
    Open a pty master and return the fd, and the filename of the slave end.
    Deprecated, use openpty() instead."""

    try:
        master_fd, slave_fd = os.openpty()
    except (AttributeError, OSError):
        pass
    else:
        slave_name = os.ttyname(slave_fd)
        os.close(slave_fd)
        return master_fd, slave_name

    return _open_terminal()

def _open_terminal():
    """Open pty master and return (master_fd, tty_name).
    SGI and generic BSD version, for when openpty() fails."""
    try:
        import sgi
    except ImportError:
        pass
    else:
        try:
            tty_name, master_fd = sgi._getpty(os.O_RDWR, 0666, 0)
        except IOError, msg:
            raise os.error, msg
        return master_fd, tty_name
    for x in 'pqrstuvwxyzPQRST':
        for y in '0123456789abcdef':
            pty_name = '/dev/pty' + x + y
            try:
                fd = os.open(pty_name, os.O_RDWR)
            except os.error:
                continue
            return (fd, '/dev/tty' + x + y)
    raise os.error, 'out of pty devices'

def slave_open(tty_name):
    """slave_open(tty_name) -> slave_fd
    Open the pty slave and acquire the controlling terminal, returning
    opened filedescriptor.
    Deprecated, use openpty() instead."""

    return os.open(tty_name, os.O_RDWR)

def fork():
    """fork() -> (pid, master_fd)
    Fork and make the child a session leader with a controlling terminal."""

    try:
        pid, fd = os.forkpty()
    except (AttributeError, OSError):
        pass
    else:
        if pid == CHILD:
            try:
                os.setsid()
            except OSError:
                # os.forkpty() already set us session leader
                pass
        return pid, fd

    master_fd, slave_fd = openpty()
    pid = os.fork()
    if pid == CHILD:
        # Establish a new session.
        os.setsid()
        os.close(master_fd)

        # Slave becomes stdin/stdout/stderr of child.
        os.dup2(slave_fd, STDIN_FILENO)
        os.dup2(slave_fd, STDOUT_FILENO)
        os.dup2(slave_fd, STDERR_FILENO)
        if (slave_fd > STDERR_FILENO):
            os.close (slave_fd)

    # Parent and child process.
    return pid, master_fd

def _writen(fd, data):
    """Write all the data to a descriptor."""
    while data != '':
        n = os.write(fd, data)
        data = data[n:]

def _read(fd):
    """Default read function."""
    return os.read(fd, 1024)

def _copy(master_fd, master_read=_read, stdin_read=_read, stdin_fd=STDIN_FILENO,
             stdout_fd=STDOUT_FILENO):
    """Parent copy loop.
    Copies
            pty master -> stdout_fd     (master_read)
            stdin_fd   -> pty master    (stdin_read)"""
    try:
        mode = tty.tcgetattr(stdin_fd)
        tty.setraw(stdin_fd)
        restore = 1
    except tty.error:    # This is the same as termios.error
        restore = 0
    try:
        while 1:
            rfds, wfds, xfds = select(
                    [master_fd, stdin_fd], [], [])
            if master_fd in rfds:
                data = master_read(master_fd)
                os.write(stdout_fd, data)
            if stdin_fd in rfds:
                data = stdin_read(stdin_fd)
                _writen(master_fd, data)
    except (IOError, OSError, error):  # The last entry is select.error
        if restore:
            tty.tcsetattr(stdin_fd, tty.TCSAFLUSH, mode)
        if stdin_fd > STDERR_FILENO:
            os.close(stdin_fd)
        if stdout_fd > STDERR_FILENO:
            os.close(stdout_fd)

def spawn(argv, master_read=_read, stdin_read=_read, stdin_fd=STDIN_FILENO,
            stdout_fd=STDOUT_FILENO):
    """Create a spawned process. The controlling terminal reads and
    writes its data to stdin_fd and stdout_fd respectively.
    
    NOTE: This function does not return until one of the input or output file
    descriptors are closed, or the child process exits."""
    if type(argv) == type(''):
        argv = (argv,)
    pid, master_fd = fork()
    if pid == CHILD:
        apply(os.execlp, (argv[0],) + argv)
    _copy(master_fd, master_read, stdin_read, stdin_fd, stdout_fd)

def th_spawn(argv, master_read=_read, stdin_read=_read, stdin_fd=STDIN_FILENO,
                stdout_fd=STDOUT_FILENO):
    """Create a spawned process. The controlling terminal reads and
    writes its data to stdin_fd and stdout_fd respectively. The function
    returns the pid of the spawned process.  (It returns immediately.)"""
    import thread
    if type(argv) == type(''):
        argv = (argv,)
    pid, master_fd = fork()
    if pid == CHILD:
        apply(os.execlp, (argv[0],) + argv)
    thread.start_new_thread(_copy, (master_fd, master_read, stdin_read, \
                                            stdin_fd, stdout_fd))
    return pid

def popen2(cmd, bufsize=1024, master_read=_read, stdin_read=_read):
    """Execute the shell command 'cmd' in a sub-process.
    
    If 'bufsize' is specified, it sets the buffer size for the I/O pipes.
    The function returns (child_stdin, child_stdout, child_pid), where the
    file objects are pipes connected to the spawned process's controling
    terminal, and the child_pid is the pid of the child process.
    """
    argv = ('/bin/sh', '-c', cmd)
    child_stdin_rfd, child_stdin_wfd = os.pipe()
    child_stdout_rfd, child_stdout_wfd = os.pipe()
    child_pid = th_spawn(argv, master_read, stdin_read, child_stdin_rfd, \
                            child_stdout_wfd)
    child_stdin = os.fdopen(child_stdin_wfd, 'w', bufsize)
    child_stdout = os.fdopen(child_stdout_rfd, 'r', bufsize)
    return child_stdin, child_stdout, child_pid
