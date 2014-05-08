"""A SSH Interface class.

An interface to ssh on posix systems, and plink (part of the Putty
suite) on Win32 systems.

By Rasjid Wilcox.
Copyright (c) 2002.

Version: 0.2
Last modified 4 September 2002.

Drawing on ideas from work by Julian Schaefer-Jasinski, Guido's telnetlib and
version 0.1 of pyssh (http://pyssh.sourceforge.net) by Chuck Esterbrook.

Licenced under a Python 2.2 style license.  See License.txt.
"""

DEBUG_LEVEL = 1

import os, getpass, sys
import signal    # should cause all KeyboardInterrupts to go to the main thread
                 # try for Linux, does not seem to be try under Cygwin
import nbpipe
import time

# Constants
SSH_PORT=22
SSH_PATH=''

CTRL_C=chr(3)

READ_LAZY=0
READ_SOME=1
READ_ALL=2
READ_LW_ALL=3
MAX_TIMEOUT = 30

# set the path to ssh / plink, and chose the popen2 funciton to use
if os.name=='posix':
    import fssa    # we can look for ssh-agent on posix
                   # XXX Can we on Win32/others?
    import ptyext  # if my patch gets accepted, change this to check for a
                   # sufficiently high version of python, and assign ptyext=pty
                   # if sufficient.
    sshpopen2=ptyext.popen2
    CLOSE_STR='~.'
    tp=os.popen('/usr/bin/which ssh')
    SSH_PATH=tp.read().strip()
    try:
        tp.close()
    except IOError:
        # probably no child process
        pass
    if SSH_PATH == '':
        tp=os.popen('command -v ssh')  # works in bash, ash etc, not csh etc.
        SSH_PATH=tp.read().strip()
        tp.close()
    if SSH_PATH == '':
        check = ['/usr/bin/ssh', '/usr/local/bin/ssh', '/bin/ssh']
        for item in check:
            if os.path.isfile(item):
                SSH_PATH=item
                break
    PORT_STR='-p '
else:
    sshpopen2=os.popen2
    CLOSE_STR=CTRL_C        # FIX-ME: This does not work.
                            # I think I need to implement a 'kill' component
                            # to the close function using win32api.
    SSH_PATH=''
    PORT_STR='-P '

class mysshError(Exception):
    """Error class for myssh."""
    pass

# Helper functions
def _prompt(message, prompt, password):
    """Print the message as the prompt for input.
    Return the text entered."""
    foundprompt = (message.lower().find(prompt) >= 0)# or \
        #(message.lower().find('passphrase:') >=0)
    #print """User input required for ssh connection.
    #(Type Ctrl-C to abort connection.)"""
    abort = 0
    try:
        if foundprompt:
            response = password #getpass.getpass(prompt)
        elif (message.lower().find('sa key fingerprint') >= 0):
            if DEBUG_LEVEL:
                print "\nFound DSA/RSA key fingerprint request"
            """Found DSA/RSA key fingerprint request"""
            response = "yes"
        elif (message.lower().find('denied') >=0):
            if DEBUG_LEVEL:
                print "\nPermission denied from host"
            response = ''
            abort = 1
        elif (message.lower().find('do you want this directory created now') >= 0):
            response = "y"
        else:
            response = ""
    except KeyboardInterrupt:
        response = ''
        abort = 1
    return response, abort


def _lwiprompt(message, prompt, response):
    """Print the message as the prompt for input.
    Return the text entered."""
    abort = 0
    try:
        if (message.lower().find(prompt.lower()) >= 0):
            answer = response
        else:
            answer = ''
            abort = 1

    except KeyboardInterrupt:
        answer = ''
        abort = 1
    return answer, abort    

"""
def _lwiprompt(message, prompt, response):
    return _lwiprompt(message, prompt, response, "", "")
"""
class Ssh:
    """A SSH connection class."""
    def __init__(self, username=None, host='localhost', port=None):
        """Constructor.  This does not try to connect."""
        self.debuglevel = DEBUG_LEVEL
        self.sshpath = SSH_PATH
        self.username = username
        self.host = host
        self.port = port
        self.isopen = 0
        self.sshpid = 0  # perhaps merge this with isopen
        self.old_handler = signal.getsignal(signal.SIGCHLD)
        self.connected = False
        sig_handler = signal.signal(signal.SIGCHLD, self.sig_handler)
        self.__prompt = "PROMPT% "
    
    def getprompt(self):
        return self.__prompt
    hostprompt = property(getprompt)


    def __del__(self):
        """Destructor -- close the connection."""
        if self.isopen:
            self.close()
    
    def sig_handler(self, signum, stack):
        """ Handle SIGCHLD signal """
        if signum == signal.SIGCHLD:
            try:
                os.waitpid(self.sshpid, 0)
            except:
                pass
        if self.old_handler != signal.SIG_DFL:
            self.old_handler(signum, stack)

    
    def setparameter(self, username):
        self.username = username

    def attach_agent(self, key=None):
        if os.name != 'posix':
            # only posix support at this time
            return
        if 'SSH_AUTH_SOCK' not in os.environ.keys():
            fssa.fssa(key)

    def set_debuglevel(self, debuglevel):
        """Set the debug level."""
        self.debuglevel = debuglevel
        
    def set_sshpath(self, sshpath):
        """Set the ssh path."""
        self.sshpath=sshpath
    
    # Low level functions
    def open(self, cmd=None):
        """Opens a ssh connection.
        
        Raises an mysshError if myssh.sshpath is not a file.
        Raises an error if attempting to open an already open connection.
        """
        self.attach_agent()
        if not os.path.isfile(self.sshpath):
            raise mysshError, \
            "Path to ssh or plink is not defined or invalid.\nsshpath='%s'" \
             % self.sshpath
        if self.isopen:
            raise mysshError, "Connection already open."
        sshargs = ''
        if self.sshpath.lower().find('plink') != -1:
            sshargs = '-ssh '
        if self.port and self.port != '':
            sshargs += PORT_STR + self.port + ' '
        if self.username and self.username !='':
            sshargs += self.username + '@'
        sshargs += self.host
        if cmd:
            sshargs += ' ' + cmd
        if self.debuglevel:
            print ">> Running %s %s." % (self.sshpath, sshargs)
        # temporary workaround until I get pid's working under win32
        if os.name == 'posix':
            self.sshin, self.sshoutblocking, self.sshpid = \
                                sshpopen2(self.sshpath + ' ' + sshargs)
        else:
            self.sshin, self.sshoutblocking = \
                                sshpopen2(self.sshpath + ' ' + sshargs)
        self.sshout = nbpipe.nbpipe(self.sshoutblocking)
        self.isopen = 1
        if self.debuglevel:
            print ">> ssh pid is %s." % self.sshpid
        
    def close(self, addnewline=1):
        """Close the ssh connection by closing the input and output pipes.
        Returns the closing messages.
        
        On Posix systems, by default it adds a newline before sending the
        disconnect escape sequence.   Turn this off by setting addnewline=0.
        """
        if os.name == 'posix':
            try:
                if addnewline:
                    self.write('\n')
                self.write(CLOSE_STR)
            except (OSError, IOError, mysshError):
                pass
        output = self.read_lazy()
        try:
            self.sshin.close()
            self.sshoutblocking.close()
        except:
            pass
        if os.name == 'posix':
            try:
                os.kill(self.sshpid, signal.SIGHUP)
            except:
                pass
        self.isopen = 0
        if self.debuglevel:
            print ">> Connection closed."
        self.connected = False
        return output
        
    def write(self, text):
        try:
            """Send text to the ssh process."""
            # May block?? Probably not in practice, as ssh has a large input buffer.
            if self.debuglevel:
                print ">> Sending %s" % text
            if self.isopen:
                while len(text):
                    numtaken = os.write(self.sshin.fileno(),text)
                    if self.debuglevel:
                        print ">> %s characters taken" % numtaken
                    text = text[numtaken:]
            else:
                raise mysshError, "Attempted to write to closed connection."
        except (OSError, IOError, mysshError): 
            return
    
    # There is a question about what to do with connections closed by the other
    # end.  Should write and read check for this, and force proper close?
    def read_very_lazy(self):
        """Very lazy read from sshout. Just reads from text already queued."""
        return self.sshout.read_very_lazy()
    
    def read_lazy(self):
        """Lazy read from sshout.  Waits a little, but does not block."""
        return self.sshout.read_lazy()
    
    def read_some(self):
        """Always read at least one block, unless the connection is closed.
        May block."""
        if self.isopen:
            return self.sshout.read_some()
        else:
            return self.sshout.read_very_lazy()    
        
    def read_all(self):
        """Reads until end of file hit.  May block."""
        if self.isopen:
            return self.sshout.read_all()
        else:
            return self.sshout.read_very_lazy()
    
    def read_lw_all(self):
        """Reads until prompt returns. May block."""
        if self.isopen:
            timeout = 0
            banner = self.sshout.read_some()
            print "In read_lw_all Expecting:" + self.__prompt
            print "In read_lw_all Received:" + banner
            while banner.find(self.__prompt) == -1 and timeout < MAX_TIMEOUT:
                time.sleep(1)
                timeout += 1
                banner = banner + self.sshout.read_very_lazy()
            if self.debuglevel:
                print "================================================="
                print "Prompt: " + self.__prompt
                print banner
                print "================================================="
            return banner
        else:
            if self.debuglevel:
                print "Ssh::read_lw_all() reading very lazy when should be all"
                time.sleep(10)
            return self.sshout.read_very_lazy()
        time.sleep(5)

    def login(self, password):
        """Logs in to the ssh host.  Checks for standard prompts, and calls
        the function passed as promptcb to process them.
        Returns the login banner, or 'None' if login process aborted.
        """
        self.connected = False
        logintext='Last login:'
        text = ''
        trycnt = 0

        self.open()
        banner = self.read_some()

        if self.debuglevel:
            print ">> 1st banner read is: %s" % banner
            print "Logging in to:" + self.username
            print "Password:" + password
            print "Hostname:" + self.host

        while banner.find(logintext) == -1 and banner.find("$") == -1 and banner.find("#") == -1 and banner.find(">") == -1:
            if banner.lower().find("permission denied") > -1 or banner.lower().find("try again") > -1 or \
                banner.lower().find("connection refused") > -1 or banner.lower().find("host key verification failed") > -1:
                self.close()
                return banner

            if trycnt == 6:
                self.close()
                return banner

            response, abort = _prompt(banner, "password", password)
            trycnt += 1

            if abort:
                self.close()
                return banner

            if response != "":
                if self.debuglevel:
                    print "pySsh sending: " + response
                self.write(response + '\n')

            banner += self.read_some()

        if banner.find(logintext) != -1 or banner.find("Have") != -1 or \
            banner.find("$") != -1 or banner.find("#") != -1 or banner.find(">") != -1:
            self.password  = password
            self.connected = True
        else:
            self.connected = False
            return banner

        self.write("pwd\n")
        output = self.read_some()
        result = output.split("\n")
        if len(result) > 2:
            self.__prompt = result[len(result)-1]
        return banner
    
    def ssologin(self, logintext='Last login:'):
        retVal = True
        """Logs in to the ssh host.  Checks for standard prompts, and calls
        the function passed as promptcb to process them.
        Returns the login banner, or 'None' if login process aborted.
        """
        text = ''
        self.open()
        banner = self.read_some()
        if self.debuglevel:
            print ">> 1st banner read is: %s" % banner
            
        promptcount = 0
        #print "=======",banner, "======="
        #print "logintext",logintext
        #if banner.find(logintext) > -1:
        #    return banner
        abort = 0
        while banner.find(logintext) == -1:
            print banner
            if banner.lower().find("password") > -1:
                retVal = False
                abort  = 1   
                print "Found password prompt on", self.host
            elif banner.lower().find("connection refused"):
                retVal = False
                abort  = 1
                print "Connection refused from host", self.host
            elif (banner.lower().find('rsa key fingerprint') >= 0):
                """Found RSA key fingerprint request"""
                response = "yes"
                self.write(response + '\n')
                banner = self.read_some()
            if abort:
                self.connected = False
                return self.close()
        self.connected = True
        return banner
            
    def multipasslogin(self, password1, password2, password3, logintext='Last login:', prompt_callback=_prompt):
        """Logs in to the ssh host.  Checks for standard prompts, and calls
        the function passed as promptcb to process them.
        Returns the login banner, or 'None' if login process aborted.
        """
        text = ''
        self.open()
        banner = self.read_some()
        if self.debuglevel:
            print ">> 1st banner read is: %s" % banner
        
        password = password1
        promptcount = 0
        while banner.find(logintext) == -1:
            print banner
            if banner.find("password") > -1:
                promptcount = promptcount + 1
            if promptcount == 1:
                password = password2
            elif promptcount == 2:
                password = password3
            response, abort = prompt_callback(banner, password=password)
            if abort:
                return self.close()
            print "pySsh sending: " + response
            self.write(response + '\n')
            banner = self.read_some()
        #end while
        if promptcount > 1:
            return "Fail"
        else:
            return banner
        
    def logout(self):
        """Logs out the session."""
        self.close()
        
    def sendcmd(self, cmd, timeout=0, readtype=READ_SOME):
        """Sends the command 'cmd' over the ssh connection, and returns the
        result.  By default it uses read_some, which may block.
        """
        banner = self.__prompt.strip()
        cmd = cmd.strip() #+ ";cd ~"
        try:
            if cmd[-1] != '\n':
                cmd += '\n'
            
            self.write(cmd)
            if readtype == READ_ALL:
                return banner + self.read_all()
            elif readtype == READ_LAZY:
                return self.read_lazy()
            else:
                if timeout > 0:
                    time.sleep(timeout)
                banner = banner + self.read_some()
                return banner
        except mysshError:
            self.close()
   
   
    def sudocmd(self, command):
        #sys.stdout.write(command + "\n")
        #sys.stdout.write("Password: " + self.password + "\n")
        sudooutput = self.interact("sudo " + command + "; sudo -k", "password", self.password)
        return sudooutput
                
    def interact(self, cmd, prompt, answer, readtype=READ_LW_ALL):
        try:
            cmd = cmd.strip() + "; cd ~"
            if cmd[-1] != '\n':
                cmd += '\n'
            self.write(cmd)
            if readtype == READ_ALL:
                if self.debuglevel:
                    print "Ssh::interact() reading all"
                return self.read_all()
            elif readtype == READ_LAZY:
                if self.debuglevel:
                    print "Ssh::interact() reading lazy"
                return self.read_lazy()
            elif readtype == READ_SOME:
                if self.debuglevel:
                    print "Ssh::interact() reading lw_all"
                banner = self.read_some()
                response, abort = _lwiprompt(banner, prompt, answer)
                if response:
                    if self.debuglevel:
                        print "Prompt:  ", prompt
                        print "Response:", response
                    self.write(response + '\n')
                    banner = banner + self.read_some()
                else:
                    if self.debuglevel:
                        print "No response sent"
                        print "Prompt:  ", prompt
                        print "Response:", response
                return banner
            else:
                if self.debuglevel:
                    print "Ssh::interact() reading lw_all"
                banner = self.read_some()
                response, abort = _lwiprompt(banner, prompt, answer)
                if response:
                    if self.debuglevel:
                        print "Prompt:  ", prompt
                        print "Response:", response
                    self.write(response + '\n')
                    banner = banner + self.read_lw_all()
                    #banner = banner + self.read_some()
                else:
                    if self.debuglevel:
                        print "No response sent"
                        print "Prompt:  ", prompt
                        print "Response:", response
                #time.sleep(10)
                return banner
        except mysshError:
            if self.debuglevel:
                print "Ssh::interact encountered mysshError"
            self.close()
        return banner


    def runcmd_readall(self, cmd):
        try:
            cmd = cmd.strip() + "; cd ~"
            if cmd[-1] != '\n':
                cmd += '\n'
            self.write(cmd)
            if self.debuglevel:
                print "Ssh::interact() reading lw_all"
            banner = self.read_lw_all()
            return banner
        except mysshError:
            if self.debuglevel:
                print "Ssh::interact encountered mysshError"
            self.close()
        return banner

    def setpass(self, cmd, prompt, answer):
        try:
            import commands
            readtype=READ_SOME
            if cmd[-1] != '\n':
                cmd += '\n'
            self.write(cmd)
            if readtype == READ_ALL:
                return self.read_all()
            elif readtype == READ_LAZY:
                return self.read_lazy()
            else:
                banner = self.read_some()
                response, abort = _lwiprompt(banner, prompt, answer)
                if response:
                    #print "Banner:  ", banner
                    #print "Prompt:  ", prompt
                    #print "Response:", response
                    self.write(response + '\n')
                    banner = banner + self.read_some()
                    response, abort = _lwiprompt(banner, prompt, answer)
                    if response:
                        #print "Banner:  ", banner
                        #print "Prompt:  ", prompt
                        #print "Response:", response
                        self.write(response + '\n')
                        banner = banner + self.read_some()
                        #print "Wrote response"
                else:
                    print "No response sent"
                    print "Prompt:  ", prompt
                    print "Response:", response
                #self.sendcmd("sleep 0")
                #commands.getoutput("sleep 1")
                return banner
        except mysshError:
            self.close()

    def changepass(self, oldpass, newpass):
        try:
            self.sendcmd("sleep 0")
            retVal = False
            cmd = "/opt/likewise/bin/passwd"
            readtype=READ_SOME
            if cmd[-1] != '\n':
                cmd += '\n'
            self.write(cmd)
            if readtype == READ_ALL:
                return self.read_all()
            elif readtype == READ_LAZY:
                return self.read_lazy()
            else:
                banner = self.read_some()
                response, abort = _lwiprompt(banner, "current", oldpass)
                if response:
                    self.write(response + '\n')
                    banner = banner + self.read_some()
                    response, abort = _lwiprompt(banner, "new", newpass)
                    if response:
                        self.write(response + '\n')
                        banner = banner + self.read_some()
                    response, abort = _lwiprompt(banner, "password", newpass)
                    if response:
                        self.write(response + '\n')
                        banner = banner + self.read_some()
                        if banner.lower().find("success") > -1:
                            retVal = True
                else:
                    print "No password changed"
                    #print "Prompt:  ", prompt
                    #print "Response:", response
                #self.write("sleep 3\n")
                return banner
        except mysshError:
            self.close()
            
def test():
    """Test routine for myssh.
    
    Usage: python myssh.py [-d] [-sshp path-to-ssh] [username@host | host] [port]
    
    Default host is localhost, default port is 22.
    """
    import sys
    debug = 0
    if sys.argv[1:] and sys.argv[1] == '-d':
        debug = 1
        del sys.argv[1]
    testsshpath = SSH_PATH
    if sys.argv[1:] and sys.argv[1] == '-sshp':
        testsshpath = sys.argv[2]
        del sys.argv[1]
        del sys.argv[1]
    testusername = None
    testhost = "testuser@10.100.0.1"
    testport = '22'
    testpassword = 'password'
    if sys.argv[1:]:
        testhost = sys.argv[1]
        if testhost.find('@') != -1:
            testusername, testhost = testhost.split('@')
    if sys.argv[2:]:
        testport = sys.argv[2]
        
    testcon = Ssh(testusername, testhost, testport)
    testcon.set_debuglevel(debug)
    testcon.set_sshpath(testsshpath)
    testcon.login(testpassword)
    
    cmd = None
    cmd = "cd /home"
    stdout = testcon.sendcmd(cmd)
    print stdout
    cmd = "pwd"
    stdout = testcon.sendcmd(cmd)
    print stdout
    if (stdout.find("/home") != -1):
        print "found me a home!"
    cmd = "exit"
    print testcon.sendcmd(cmd)
    while (cmd != 'exit') and testcon.isopen:
        cmd = raw_input("Enter command to send: ")
        print testcon.sendcmd(cmd)
    testcon.close()

if __name__ == '__main__':
    test()
