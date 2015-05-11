"""Implements a non-blocking pipe class."""

# Since it uses thread rather than select, it is portable to at least
# posix and windows environments.

# Author: Rasjid Wilcox, copyright (c) 2002
# Ideas taken from the Python 2.2 telnetlib.py library.
#
# Last modified: 3 August 2002
# Licence: Python 2.2 Style License.  See license.txt.

# TO DO:
#     * Handle excpetions better, particularly Keyboard Interupts.
#     * Possibly do a threadless version for posix environments
#       where we can use select (is probably more efficient).
#     * A test function.

import Queue
import thread
import os
import time
import types


#INT_TYPE = type(1)
MIN_TIMEOUT = 0.01
DEBUG_LEVEL = 0

class nbpipe:
    def __init__(self, readfile, timeout=0.5, pipesize=0, blocksize=1024):
        """Initialise a non-blocking pipe object, given a real file or file-descriptor.
        timeout = the default timeout (in seconds) at which read_lazy will decide
                  that there is no more data in this read
        pipesize = the size (in blocks) of the queue used to buffer the blocks read
        blocksize = the maximum block size for a raw read."""
        self.debuglevel = DEBUG_LEVEL
        if type(readfile) == types.IntType:
            self.fd = readfile
        else:
            self.fd = readfile.fileno()
        self.timeout = timeout  # default timeout allowed between blocks
        self.pipesize = pipesize
        self.blocksize = blocksize
        self.eof = 0
        self._q = Queue.Queue(self.pipesize)
        thread.start_new_thread(self._readtoq, ())
    def _readtoq(self):
        finish = 0
        while (1):
            try:
                item = os.read(self.fd, self.blocksize)
            except (IOError, OSError):
                finish = 1
            if (item == '') or finish:
                # Wait until everything has been read from the queue before
                # setting eof = 1 and exiting.
                while self.has_data():
                    time.sleep(MIN_TIMEOUT)
                self.eof = 1
                thread.exit()
            else:
                self._q.put(item)
    def has_data(self):
        return not self._q.empty()
    def eof(self):
        return self.eof
    def read_very_lazy(self, maxblocks=0):
        """Read data from the queue, to a maximum of maxblocks (0 = infinite).
        Does not block."""
        data = ''
        blockcount = 0
        while self.has_data():
            data += self._q.get()
            blockcount += 1
            if blockcount == maxblocks:
                break
        return data

    def read_lazy(self, maxblocks=0, timeout=None):
        """Read data from the queue, allowing timeout seconds between block arrival.
        if timeout = None, then use the objects (default) timeout.
        Returns '' if we are at the EOF, or no data turns up within the timeout.
        Reads at most maxblocks (0 = infinite).
        Does not block."""
        if self.eof:
            return ''
        if timeout == None:
            timeout = self.timeout
        maxwait = timeout / MIN_TIMEOUT
        data = ''
        blockcount = 0
        waitcount = 0
        while waitcount < maxwait:
            block = self.read_very_lazy(1)
            if block != '':
                blockcount += 1
                data += block
                waitcount = 0  # reset the wait count
                if blockcount == maxblocks:
                    break
            else:
                time.sleep(MIN_TIMEOUT)
                waitcount += 1
        return data
    def read_some(self, maxblocks=0, timeout=None):
        """As for read_lazy, but always read a single block of data.
        May block."""
        if timeout == None:
            timeout = self.timeout
        data = ''
        readcount = 0
        while not self.eof and data == '':
            readcount += 1
            data = self.read_lazy()
            if readcount % 100 == 0:
                if self.debuglevel:
                    print "nbpipe::read_some() is reading"
        if self.eof:
            if self.debuglevel:
                print "nbpipe::readsome() found self.eof"
                
        if maxblocks != 1:
            data += self.read_lazy(maxblocks - 1, timeout)
        return data
    def read_all(self):
        """Read until the EOF. May block."""
        data = ''
        while not self.eof:
            data += self.read_lazy()
            #data += self.read_very_lazy()
            time.sleep(MIN_TIMEOUT)
        return data
    
