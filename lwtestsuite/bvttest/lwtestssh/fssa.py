# vi:et:ts=4:tw=0
""" fssa.py

    Search for an ssh-agent for the calling user and attach to it
    if found.

    Tested on poxix only
"""
# This is a Python port of Steve Allen's fsa.sh script found
# at http://www.ucolick.org/~sla/ssh/sshcron.html
# Ported by Mark W. Alexander <slash@dotnetslash.net>

import os
if os.name == 'posix':
    import pwd, stat, sys
    from commands import getoutput
    def fssa(key=None):
        """ fssa(key=None)

        Searches /tmp/ssh-* owned by the calling user for ssh-agent
        sockets. If key is provided, only sockets matching the key will be
        considered. If key is not provided, the calling users username
        will be used instead.
        """
        user, pw, uid, gid, gecos, home, shell = pwd.getpwuid(os.getuid())
        if key is None:
            key = user

        # Find /tmp/ssh-* dirs owned by this user
        candidate_dirs=[]
        for filename in os.listdir('/tmp'):
            file_stat = os.stat("/tmp/%s" % (filename))
            if file_stat[stat.ST_UID] == os.getuid() \
            and stat.S_ISDIR(file_stat[stat.ST_MODE]) \
            and filename.find('ssh-') == 0:
                candidate_dirs.append("/tmp/%s" % filename)

        candidate_sockets=[]
        for d in candidate_dirs:
            for f in os.listdir(d):
                file_stat = os.stat("%s/%s" % (d, f))
                if file_stat[stat.ST_UID] == os.getuid() \
                and stat.S_ISSOCK(file_stat[stat.ST_MODE]) \
                and f.find('agent.') == 0:
                    candidate_sockets.append("%s/%s" % (d, f))
        alive = None
        # order by pid, prefering sockets where the parent pid
        # is gone. This gives preference to agents running in the
        # background and reaped by init (maybe). Only works on
        # systems with a /proc filesystem
        if stat.S_ISDIR(os.stat("/proc")[stat.ST_MODE]):
            reorder=[]
            for s in candidate_sockets:
                pid = s[s.find('.')+1:]
                try:
                    stat.S_ISDIR(os.stat("/proc/%s" % pid)[stat.ST_MODE])
                    reorder.append(s)
                except:
                    reorder.insert(0,s)
            candidate_sockets = reorder

        for s in candidate_sockets:
            os.environ['SSH_AUTH_SOCK'] = s
            try:
                pubkey = getoutput("ssh-add -l 2>/dev/null")
            except:
                continue
            if pubkey.find(key):
                alive = 1
                break
            os.environ.pop('SSH_AUTH_SOCK')
        if alive:
            return pubkey
        else:
            return None
