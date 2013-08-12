#!/usr/bin/perl
# Copyright 2008-2011 Likewise Software, 2011-2013 BeyondTrust Software
# by Robert Auch
# gather information for emailing to support.
#
#
#
# run "perldoc pbis-support.pl" for documentation
#

#
#
#
# v0.1 2009-06-12 RCA - first version, build structures from likewise-health-check.sh
# v0.2 2009-06-23 RCA - add agent restarts with logging
# v0.3 2009-06-24 RCA - add ssh tests and other info gathering for the logfile
# v0.4 2009-06-25 RCA - tarball complete - moving to beta stage
# v0.5b 2009-06-25 RCA - cleaning up Mac support
# v0.6b 2009-06-25 RCA - daemon restarts cleaned up and stable.
# v0.7b 2009-06-28 RCA - edit syslog.conf and set "lw-set-log-level" if --norestart
# v0.8b 2009-07-06 RCA - Solaris and AIX support fixups
# v0.9b 2009-07-08 RCA - PAC info gathering if any auth tests are attempted and sudoers file gather
# v1.0 2009-07-28 RCA - turn on auth tests always, tested on Solaris 9, 10, AIX 5, HP-UX 11.23, 11.11, RHEL 3,4,5, Ubuntu 8,9, Suse 9,10, and Mac OSX10.5 as working. Works on Solaris 8 if Perl upgraded to at least 5.6
# v1.1 2009-08-14 RCA - clean up version information to getLikewiseVersion only, add inclusion of /etc/likewise and timezone data
# v1.1.1 2009-08-20 RCA - gather samba logs (option)
# v1.1.2 2009-08-20 RCA - write out options passed
# v1.1.3 2009-09-10 RCA - documentation updates, and better hunt for smb.conf
# v1.2.0 2009-10-01 RCA - Changes based on CR by Danilo
# v1.2.1 2009-11-04 RCA - Daemon Restart rewrite
# v1.2.2 2009-12-12 RCA - Tool modularity update
# v1.2.3 2009-12-22 RCA - rewrite signal handling
# v1.2.4 2010-03-18 RCA - support Solaris 10 svcs commands properly
# v1.3 2010-09-28 RCA - support LW 6.
# v2.0 2010-09-28 RCA - support LW 6 fully.
# v2.0.1b 2011-04-11 RCA - PID matching and process shutdown improvements
# v2.0.2b 2011-04-13 RCA - Mac restart issue fixes
# v2.5.0  2013-02-11 RCA PBIS 7.0 (container) support
# v2.5.1  2013-06-13 RCA config dump, code cleanup, logfile locations.
# v2.5.2  2013-08-12 RCA fix up DNS, runTool for large environments using open() rather than ``
#
# Data structures explained at bottom of file
#
# TODO (some order of importance):
# Do pre-script cleanup (duplicate daemons, running tap-log, etc.)
# gather nscd information?
# gpo testing
# samba test / review
# edit lwiauthd and smb.conf for log level = 10 in [global] section properly
# do smbclient tests
# syslog-ng editing to allow non-restarts

use strict;
#use warnings;

use Getopt::Long;
use File::Basename;
use Carp;
use FindBin;
use Config;

# Define global variables
my $gVer = "2.5.2";
my $gDebug = 0;  #the system-wide log level. Off by default, changable by switch --loglevel
my $gOutput = \*STDOUT;
my $gRetval = 0; #used to determine exit status of program with bitmasks below:

# Define system signals
use Config;
my (%gSignals, @gSignalno); # Signame names, and signal numbers, respectively
defined $Config{sig_name} || die "No signals are available on this OS? That's not right!";
my $i = 0;
foreach my $name (split(' ', $Config{sig_name})) {
    $gSignals{$name} = $i;
    $gSignalno[$i] = $name;
    $i++;
}

# masking subs for applying to, and allowing us to return, $gRetval
sub ERR_UNKNOWN ()      { 1; }
sub ERR_OPTIONS ()      { 2; }
sub ERR_OS_INFO ()      { 2; }
sub ERR_ACCESS  ()      { 4; }
sub ERR_FILE_ACCESS ()  { 4; }
sub ERR_SYSTEM_CALL ()  { 8; }
sub ERR_DATA_INPUT  ()  { 16; }
sub ERR_LDAP        ()  { 32; }
sub ERR_NETWORK ()      { 64; }
sub ERR_CHOWN   ()      { 256; }
sub ERR_STAT    ()      { 512; }
sub ERR_MAP     ()      { 1024; }

sub main();
main();
exit $gRetval;

sub usage($$)
{
    my $opt = shift || confess "no options hash passed to usage!\n";
    my $info = shift || confess "no info hash passed to usage!\n";
    my $scriptName = fileparse($0);

    my $helplines = "
$scriptName version $gVer
(C)2008-2011, Likewise Software, 2011-2013 BeyondTrust Software

usage: $scriptName [tests] [log choices] [options]

    This is the PBIS support tool.  It creates a log as specified by
    the options, and creates a gzipped tarball in:
    $opt->{tarballdir}/$opt->{tarballfile}$opt->{tarballext}, for emailing to 
    $info->{emailaddress} 

Tests to be performed:

    --(no)ssh (default = ".&getOnOff($opt->{ssh}).")
        Test ssh logon interactively and gather logs
    --sshcommand <command> (default = '".$opt->{sshcommand}."')
    --sshuser <name> (instead of interactive prompt)
    --(no)gpo --grouppolicy (default = ".&getOnOff($opt->{gpo}).")
        Perform Group Policy tests and capture Group Policy cache
    -u --(no)users (default = ".&getOnOff($opt->{users}).")
        Enumerate all users
    -g --(no)groups (default = ".&getOnOff($opt->{groups}).")
        Enumerate all groups
    --autofs --(no)automounts (default = ".&getOnOff($opt->{automounts}).")
        Capture /etc/lwi_automount in tarball
    --(no)dns (default = ".&getOnOff($opt->{dns}).")
        DNS lookup tests
    -c --(no)tcpdump (--capture) (default = ".&getOnOff($opt->{tcpdump}).")
        Capture network traffic using OS default tool
        (tcpdump, nettl, snoop, etc.)
    --capturefile <file> (default = $opt->{capturefile})
    --(no)smb (default = ".&getOnOff($opt->{smb}).")
        run smbclient against local samba server
    -o --(no)othertests (--other) (default = ".&getOnOff($opt->{othertests}).")
        Pause to allow other tests (interactive logon,
        multiple ssh tests, etc.) to be run and logged.
    --(no)delay (default = ".&getOnOff($opt->{delay}).")
        Pause the script for $opt->{delaytime} seconds to gather logging
        data, for example from GUI logons.
    -dt --delaytime <seconds> (default = $opt->{delaytime})

Log choices: 

    --(no)lsassd (--winbindd) (default = ".&getOnOff($opt->{lsassd}).")
        Gather lsassd debug logs
    --(no)lwiod (--lwrdrd | --npcmuxd) (default = ".&getOnOff($opt->{lwiod}).")
        Gather lwrdrd debug logs
    --(no)netlogond (default = ".&getOnOff($opt->{netlogond}).")
        Gather netlogond debug logs
    --(no)gpagentd (default = ".&getOnOff($opt->{gpagentd}).")
        Gather gpagentd debug logs
    --(no)eventlogd (default = ".&getOnOff($opt->{eventlogd}).")
        Gather eventlogd debug logs
    --(no)eventfwdd (default = ".&getOnOff($opt->{eventfwdd}).")
        Gather eventfwdd debug logs
    --(no)reapsysld (default = ".&getOnOff($opt->{reapsysld}).")
        Gather reapsysld debug logs
    --(no)regdaemon (default = ".&getOnOff($opt->{lwregd}).")
        Gather regdaemon debug logs
    --(no)lwsm (default = ".&getOnOff($opt->{lwsmd}).")
        Gather lwsm debug logs
    --(no)smartcard (default = ".&getOnOff($opt->{lwscd}).")
        Gather smartcard daemon debug logs
    --(no)messages (default = ".&getOnOff($opt->{messages}).")
        Gather syslog logs
    --(no)gatherdb (default = ".&getOnOff($opt->{gatherdb}).")
        Gather PBIS Databases
    --(no)sambalogs (default = ".&getOnOff($opt->{sambalogs}).")
        Gather logs and config for Samba server
    -ps --(no)psoutput (default = ".&getOnOff($opt->{psoutput}).")
        Gathers full process list from this system

Options: 

    -r --(no)restart (default = ".&getOnOff($opt->{restart}).")
        Allow restart of the PBIS daemons to separate logs
    --(no)syslog (default = ".&getOnOff($opt->{syslog}).")
        Allow editing syslog.conf during the debug run if not
        restarting daemons (exclusive of -r)
    -V --loglevel {error,warning,info,verbose,debug}
        Changes the logging level. (default = $opt->{loglevel} )
    -l --log --logfile <path> (default = $opt->{logfile} )
        Choose the logfile to write data to.
    -t --tarballdir <path> (default = $opt->{tarballdir} )
        Choose where to create the gzipped tarball of log data

    Examples:

    $scriptName --ssh --lsassd --nomessages --restart -l pbis.log
    $scriptName --restart --regdaemon -c
        Capture a tcpdump or snoop of all daemons starting up
        as well as full logs

";
    return $helplines;
}

######################################
# Helper Functions Defined Below
# 
# Used as shortcuts throughout the
# other subroutines, or called in 
# multiple "main" routines, or
# just planned to be reused

sub daemonRestart($$) {
    my $info = shift || confess "no info hash to restartDaemon!!\n";
    my $options = shift || confess "no options hash to restartDaemon!!\n";
    my ($startscript, $logopts, $result);
    logInfo("Stopping $options->{daemon}...");

    if ($info->{$options->{daemon}}->{pid}) {
        # script was started manually (not via startups script)
        # cause we don't store the pid when starting via init script
        logInfo("killing process $options->{daemon} by pid $info->{$options->{daemon}}->{pid}");
        $result = killProc($info->{$options->{daemon}}->{pid}, 15, $info);
        if ($options->{daemon}=~/^lwsm/) {
            logInfo("Sleeping 30 seconds for $options->{daemon} to safely stop");
            sleep 30;
        }
        my $procpid = findProcess($options->{daemon}, $info);
        $result = ERR_SYSTEM_CALL if (defined($procpid->{pid}));
        if ($result & ERR_SYSTEM_CALL) {
            logVerbose("Failed kill by ID, trying by name $options->{daemon}");
            $result = killProc("$options->{daemon}", 9, $info);
            if ($result & ERR_SYSTEM_CALL) {
                logError("Couldn't stop or kill $options->{daemon}");
                logError("Manually stop or kill $options->{daemon} or it will continue running with debugging on.");
            }
        }
    } else {
        # Build the startscript based on hash data, then replace the generic "daemonname"
        # with the proper value.
        $startscript = $info->{svcctl}->{stop1}.$info->{svcctl}->{stop2}.$info->{svcctl}->{stop3};
        $startscript =~ s/daemonname/$options->{daemon}/;
        logDebug("Calling $options->{daemon} stop as: ".$startscript);
        $result = System("$startscript"); #removed for Mac Stpuidness:  > /dev/null 2>&1"); #2011-04-12 RCA
        if ($options->{daemon}=~/^lwsm/) {
            logInfo("Sleeping 30 seconds for $options->{daemon} to safely stop");
            sleep 30;
        } else {
            sleep 1;
        }

        my $proc=findProcess($options->{daemon}, $info);
        if (($info->{OStype} eq "darwin") and defined($proc->{pid})) {
            $result = System("$startscript > /dev/null 2>&1");
            # Darwin 10.6 seems to need 2 "launchctl stop" commands in testing - 2011-04-12 RCA
            sleep 2;
        }
        $proc=findProcess($options->{daemon}, $info) if ($info->{OStype} eq "darwin");
        if ($result || ($info->{OStype} eq "darwin")) {
            logWarning("Process $options->{daemon} failed to stop, attempting kill");
            if (defined($info->{$options->{daemon}}->{pid})) {
                logVerbose("killing process $options->{daemon} by pid $info->{$options->{daemon}}->{pid}");
                $result = killProc($info->{$options->{daemon}}->{pid}, 9, $info);
            } else {
                logVerbose("killing process $options->{daemon} with pkill");
                $result = killProc($options->{daemon}, 9, $info);
            }
            if ($result) {
                $gRetval |= ERR_SYSTEM_CALL;
                logError("Couldn't stop or kill $options->{daemon}");
            }
        } else {
            logVerbose("Successfully stopped $options->{daemon}");
        }
    }
    # make sure it's really down, else recursively try again
    my $catch;
    for my $i (1 .. 10) {
        sleep 2;
        $catch = findProcess($options->{daemon}, $info);
        last if (not defined($catch->{pid}));
        killProc($options->{daemon}, 15, $info);
        my $j=10-$i;
        logVerbose("$options->{daemon} failed to stop, doing last-ditch kill (".$j." attempts)...")
    }
    killProc($options->{daemon}, 9, $info) if defined($catch->{pid});
    sleep 5;
    # now we start the daemon back up
    if (not defined($options->{loglevel}) or $options->{loglevel} eq "normal") {
        #restart using init scripts, no special logging
        logInfo("Starting $options->{daemon}...");
        $startscript = $info->{svcctl}->{start1}.$info->{svcctl}->{start2}.$info->{svcctl}->{start3};
        $startscript =~ s/daemonname/$options->{daemon}/;
        logDebug("Calling $options->{daemon} start as: '$startscript'");
        $result = System("$startscript"); # removed for Mac stupidness:  > /dev/null 2>&1");
        if ($result) {
            $gRetval |= ERR_SYSTEM_CALL;
            logError("Failed to start $options->{daemon}");
            logError("System may be in an unusable state!!");
        } else {
            logDebug("Successfully started $options->{daemon}");
        }
    } else {
        $logopts = " --loglevel $options->{loglevel} --logfile ".$info->{logpath}."/".$options->{daemon}.".log ".$info->{lw}->{daemons}->{startcmd};
        $startscript = $info->{lw}->{base}."/sbin/".$options->{daemon}.$logopts;
        logInfo("Starting $options->{daemon} as: $startscript");
        #TODO replace with proper forking
        $result = open($info->{$options->{daemon}}->{handle}, "$startscript > /dev/null|");
        if ($result) {
            my $proc=findProcess($options->{daemon}, $info);
            $info->{$options->{daemon}}->{pid} = $proc->{pid};
            logVerbose("pid for $options->{daemon} = $proc->{pid}");
        } else {
            $gRetval |= ERR_SYSTEM_CALL;
            logError("Failed to start $options->{daemon}");
            logError("System may be in an unusable state!!");
        }
    }
    return;
}

sub daemonContainerStop($$) {
    my $info = shift || confess "no info hash to daemonContainerStop!!\n";
    my $options = shift || confess "no options hash to daemonContainerStop!!\n";
    my ($result, $script);
    logInfo("Stopping $options->{daemon}...");
    my $proc=findProcess($options->{daemon}, $info);
    if ($proc) {
        $script=$info->{lw}->{path}."/".$info->{lwsm}->{control}." stop ".$options->{daemon};
        logVerbose("Running: $script");
        $result = System("$script");
    }
    if (defined($info->{$options->{daemon}}->{pid}) and $info->{$options->{daemon}}->{pid}) {
        logWarning("$options->{daemon} failed to stop, killing process by stored pid $info->{$options->{daemon}}->{pid}");
        $result = killProc($info->{$options->{daemon}}->{pid}, 9, $info);
    }
    $proc=findProcess($options->{daemon}, $info);
    if (exists $proc->{pid}) {
        logWarning("$options->{daemon} failed to stop, killing process by found pid $proc->{pid}!");
        $result = killProc($proc->{pid}, 9, $info);
        if ($result) {
            logError("Failed to stop $options->{daemon} - this system is in an unusable state!");
            $gRetval |= ERR_SYSTEM_CALL;
            return $gRetval;
        }
    } else {
        logVerbose("$options->{daemon} not found to be running (probably ok)");
    }
    if (exists $info->{$options->{daemon}}->{pid}) {
        delete $info->{$options->{daemon}}->{pid};
#        I don't think I have handles in use in this portion of the code. But if they are, here's how to clean them up.
#        close $info->{$options->{daemon}}->{handle};
#        delete $info->{$options->{daemon}}->{handle};
        logVerbose("Clearing pid for $options->{daemon}");
    }
    return $result;
}

sub daemonContainerStart($$) {
    my $info = shift || confess "no info hash to daemonContainerStart!!\n";
    my $options = shift || confess "no options hash to daemonContainerStart!!\n";
    my ($startscript, $logopts, $result);
    if ($info->{$options->{daemon}}->{pid}) {
        logWarning("daemonContainerStart was called with an already valid PID for $options->{daemon}, killing it.");
        daemonContainerStop($info, $options);
    }
    if (not ($options->{loglevel} eq "normal") and defined ($options->{loglevel})) {
        $startscript = $info->{lw}->{base}."/sbin/".$info->{lw}->{daemons}->{lwsm}."d --container ";
        $startscript = $startscript.$options->{daemon}." ";
        $startscript = $startscript."--loglevel $options->{loglevel} ";
        $startscript = $startscript."--logfile ".$info->{logpath}."/".$options->{daemon}.".log";
        logInfo("Starting container $startscript...");
        $result = System("$startscript &");
        if (not $result) {
            my $proc=findProcess($options->{daemon}, $info);
            $info->{$options->{daemon}}->{pid} = $proc->{pid};
            logVerbose("pid for $options->{daemon} = $proc->{pid}");
        } else {
            $gRetval |= ERR_SYSTEM_CALL;
            logError("Failed to start container $startscript!");
            logError("System may be in an unusable state!!");
        }
    }
    $startscript = $info->{lw}->{path}."/".$info->{lwsm}->{control}." start ".$options->{daemon};
    logVerbose("Running: $startscript");
    $result = System($startscript);
    if ($result) {
        logError("Failed to start daemon $options->{daemon} via lwsm!");
        $gRetval |= ERR_SYSTEM_CALL;
        return $gRetval;
    }
    return $result;
}

sub dnsLookup($$) {
    my $query = shift || confess "No name to lookup passed to dnsSrvLookup()!";
    my $type = shift || confess "No Query Type passed to dnsLookup()!";
    my @results;

    my $lookup = {};
    foreach (("dig", "nslookup")) {
        $lookup = findInPath("dig", ["/sbin", "/usr/sbin", "/bin", "/usr/bin", "/usr/local/sbin", "/usr/local/bin"]);
        last if ($lookup->{path} and ($lookup->{perm}=~/x/));
    }
    unless ($lookup->{path}) {
        $gRetval |= ERR_FILE_ACCESS;
        $gRetval |= ERR_NETWORK;
        logError("Could not find 'dig' or 'nslookup' - unable to do any network tests!");
        return;
    }
    if ($lookup->{name} == "dig") {
        logVerbose("Performing DNS dig: '$lookup->{path} $type $query'.");
        open(NS, "$lookup->{path} $type $query |");
        while (<NS>) {
            next if (/^;/);
            next if (/^\s*$/);
            push(@results, $_);
        }
        close NS;

    } elsif ($lookup->{name} == "nslookup") {
        my $line="";
        logVerbose("Performing DNS nslookup: '$lookup->{path} -query=$type $query'.");
        open(NS, "$lookup->{path} -query=$type $query |");
        while (<NS>) {
            chomp;
            my ($p1, $p2) = split(/\s+/, $_, 2);
            if ($p1 =~ /Name:/) {
                $line=$p2;
                logDebug("Matched a server name in nslookup: $p2");
            }
            if ($p1 =~ /Address:/ and $line) {
                push(@results, $line."     $p2");
                logDebug("Matched a server address in nslookup: $line is $p2");
                $line="";
            }
        }
    }
    return @results;
}
sub dnsSrvLookup($) {
    my $query = shift || confess "No name to lookup passed to dnsSrvLookup()!";

    my @records;
    logVerbose("Performing DNS lookup: 'dnsLookup() SRV $query'.");
    my @dclist = dnsLookup($query, "SRV");
    foreach my $dc (@dclist) {
        next unless ($dc =~ /^$query\./);
        logDebug("Looking at DNS Record: $dc");
        $dc =~ /([^\s]+)\.$/;
        push(@records, $1); # if ($1 =~ /^[a-zA-Z0-9\-\.]$/);
    }

    foreach (@records) {
        logVerbose("Returning '$_'");
    }
    return @records;
}

sub findProcess($$) {
    my $process = shift;
    my $info=shift;
    my $proc={};
    $process=safeRegex($process);

    if (not ($process =~/^\d+$/)) {
        logDebug("Passed $process by name, figuring out its PID...");
        my @lines;
        open(PS, "$info->{pscmd}|");
        my $catch;
        while (<PS>) {
            chomp;
            $_=~s/^\s*//;
            $_=~s/\s*$//;
            if ($_ =~/$process/i) {
                my @els = split(/\s+/, $_);
                $catch = $els[1];
                $proc->{cmd} = $els[7];
                if ($proc->{cmd} =~ /(lw-container|lwsm)/) {
                    $proc->{cmd} = join(" ", $els[7],$els[8],$els[9]);
                }
                logDebug("Checking '$proc->{cmd}' for /$process/.");
                unless ($proc->{cmd} =~ /$process/) {
                    undef $catch;
                    $proc={};
                }
            }
            logDebug("ps line: $_");
            last if $catch;
        }
        close PS;
        if ($catch) {
            logVerbose("Found $process with pid $catch.");
            $proc->{pid} = $catch;
            $proc->{bin} = $process;
        } else {
            logVerbose("Didn't find $process running.");
            return {};
        }
    } else {
        my @lines;
        open(PS, "$info->{pscmd}|");
        my $catch;
        while (<PS>) {
            chomp;
            $_=~s/^\s*//;
            $_=~s/\s*$//;
            logDebug("ps line: $_");
            my @els = split(/\s+/, $_);
            if ($els[1] == $process) {
                $catch = $els[1];
                $proc->{cmd} = $els[7];
            }
            last if $catch;
        }
        close PS;
        if ($catch) {
            logVerbose("Found $process with pid $catch.");
            $proc->{pid} = $catch;
            $proc->{cmd} =~/\/(\w+)(\s|$)/;
            $proc->{bin} = $1;
        } else {
            logVerbose("Didn't find $process running.");
            return {};
        }
    }
    return $proc;
}

sub GetErrorCodeFromChildError($)
{
    my $error = shift;

    if ($error == -1)
    {
        return $error;
    }
    else
    {
        return $error >> 8;
    }
}

sub getOnOff($) {
    # returns pretty "on/off" status for the default values
    # for the help screen.
    my $test = shift;
    if ($test) {
        return $test if ($test=~/../ || $test > 1);
        return "on";
    } else {
        return "off";
    }
}

sub getUserInfo($$$) {
    my $info = shift || confess "no info hash passed to getUserInfo!!\n";
    my $opt = shift || confess "no options hash passed to getUserInfo!!\n";
    my $name = shift;
    my ($data, $error);
    if (not defined($name)) {
        if (not defined($info->{name})) {
            logError("No username passed for user info lookup!");
            $gRetval |= ERR_DATA_INPUT;
            return $gRetval;
        } else {
            $name=$info->{name};
        }
    }
    logData("getent passwd $name: ");
    logData(join(":", getpwnam($name)));
    logData("");
    return 0 if ($name=~/root/i);  #no need to do AD lookups for root
    logData("PBIS direct lookup:");
    runTool($info, $opt, "$info->{lw}->{tools}->{userbyname} $name", "print");
    logData("User Group Membership:");
    runTool($info, $opt, "$info->{lw}->{tools}->{groupsforuser} $name", "print");

    return 0;
}

sub findInPath($$) {
    # finds a particular file in a path
    # (filename,pathArrayReference) expected input
    # does an lstat, so returns info from the lstat as well for convenience
    # returns ref to hash:
    # hash->{path} = path to file
    # hash->{type} = file type (file, directory, executable, etc.)
    # hash->{info} = ref to info{} hash from lstat
    # if file not found, return $file with undef $file->{path}

    my $filename = shift || confess "ERROR: no filename passed for path search!\n";
    my $paths = shift || confess "ERROR: no paths passed to search for $filename!\n";
    my $file = {};

    foreach my $path (@$paths) {
        if (-e "$path/$filename") {
            $file->{info} = stat(_);
            $file->{perm} = "";
            $file->{path} = "$path/$filename";
            $file->{type} = "d" if (-d _);
            $file->{type} = "f" if (-f _);
            $file->{type} = "c" if (-c _);
            $file->{perm} .= "r" if (-r _);
            $file->{perm} .= "x" if (-x _);
            $file->{perm} .= "w" if (-w _);
            $file->{name} = $filename;
            $file->{dir} = $path;
            last;
        }        
    }
    if (not defined($file->{path})) {
        $file->{info} = [];
    }
    return $file;
}

sub lineDelete($$) {
    my $file = shift || confess "ERROR: No file hash to delete line from!\n";
    my $line = shift || confess "ERROR: no line to delete from $file!\n";
    my $error;
    if ($file->{perm}!~/w/) {
        $gRetval |= ERR_FILE_ACCESS;
        logError("could not read from $file->{path} to see if '$line' already exists");
        return $gRetval;
    }
    $error = "";
    my $data;
    {
        local @ARGV=($file->{path});
        local $^I = '.lwd'; # <-- turns on inplace editing (d for delete)
        my $regex = safeRegex($line);
        while (<>) {
            if (/^[#;]+\s*$regex/) {
                $data = "Found '$line' commented out in $file->{path}, leaving alone.";
                print;
            } elsif (s/^\s*$regex/#    $line/) {
                $data = "Found '$line' in $file->{path}, commenting out.";
                $error = "found";
                print;
            } else {
                print;
            }
        }
    }
    logDebug($data) if ($data);
    if (defined($error) && $error ne "found") {
        logInfo("Could not find '$line' in $file->{path}.");
    }
}

sub lineInsert($$) {
    my $file = shift || confess "ERROR: No file hash to insert line into!\n";
    my $line = shift || confess "ERROR: no line to insert into $file!\n";
    my $error;
    if ($file->{perm}!~/w/) {
        $gRetval |= ERR_FILE_ACCESS;
        logError("could not read from $file->{path} to see if '$line' already exists");
        return $gRetval;
    }
    $error = "";
    my $data;
    {
        local @ARGV=($file->{path});
        local $^I = '.lwi'; # <-- turns on inplace editing (i for insert)
        my $regex = safeRegex($line);
        while (<>) {
            if (s/^[#;]+\s*$regex/$line/) {
                $error = "found";
                $data ="Found line '$line' commented out in $file->{path}, removing comments";
                print;
            } elsif (/^\s*$regex/) {
                $error = "found";
                $data = "Found line '$line' in $file->{path}, leaving it alone";
                print;
            } else {
                print;
            }
        }
    }
    logDebug($data) if ($data);
    if (defined($error) && $error ne "found") {
        open(FH, ">>$file->{path}");
        $error = print FH "$line\n";
        unless ($error) {
            $gRetval |= ERR_FILE_ACCESS;
            logError("Could not append $line to $file->{path} - $error - $!\n");
        }
        close FH;
    }
}

sub logData($) {
    my $line = shift;
    logger(1, $line);
}

sub logError($) {
    my $line = shift;
    $line = "ERROR: ".$line;
    my $error = 0;
    $error = print STDERR "$line\n";
    $gRetval |= ERR_FILE_ACCESS unless $error;
    logger(1, $line);
}

sub logWarning($) {
    my $line = shift;
    $line = "WARNING: ".$line;
    logger(2, $line);
}

sub logInfo($) {
    my $line = shift;
    $line = "INFO: ".$line;
    logger(3, $line);
}

sub logVerbose($) {
    my $line = shift;
    $line = "VERBOSE: ".$line;
    logger(4, $line);
}

sub logDebug($) {
    my $line = shift;
    $line = "DEBUG: ".$line;
    logger(5, $line);
}

sub logger($$) {
    # Writes to the global $gOutput file handle
    # handles errors with $gRetval
    # can be called directly, but it's better to call the error
    # handlers above, logError, logWarning, logData, etc.

    my $level = shift ||confess "ERROR: No verbosity level passed to logger!\n";
    my $line = shift; # now ok to pass empty line to logger ||confess "ERROR: No line to log passed to logger!\n";

    return $gRetval if ($level>$gDebug);

    $line = " " if (not defined($line));

    my $error = 0;
    chomp $line;
    $error = print $gOutput "$line\n";
    $gRetval |= ERR_FILE_ACCESS unless $error;
    if ($gOutput != \*STDOUT ) {
    print "$line\n";
}
return $gRetval;
}

sub killProc($$$) {
    my $process = shift || confess "ERROR: no process to kill!\n";
    my $signal = shift || confess "ERROR: No signal to send to $process!\n";
    my $info = shift || confess "ERROR: No info hash!\n";

    my $proc=findProcess($process, $info);

    if (defined($proc->{pid}) && $proc->{pid}=~/^\d+$/) {
        logInfo("Found $process with pid $proc->{pid}.");
    } else {
        $gRetval |= ERR_SYSTEM_CALL;
        logError("Could not pkill $process - it does not appear to be running!");
        return $gRetval;
    }

    my $error;
    if ($signal == 9 || $signal=~/kill/i) {
        logInfo("Attempting to kill PID $process with signal 15");
        $error = killProc2(15, $proc);
        if ($error) {
            logWarning("$process did not respond to SIGTERM, having to send KILL");
            $error = killProc2(9, $proc);
            if ($error) {
                return $gRetval;
            } else {
                logInfo("Successfully killed hung process $proc->{pid}");
                return 0;
            }
        } else {
            logVerbose("Successfully terminated PID $process");
            return 0;
        }
    } else {
        logVerbose("Attemping to kill PID $process with signal $signal");
        $error = killProc2($signal, $proc);
        return $error;
    }
}

sub killProc2($$) {
    my $signal = shift || confess "ERROR: No signal to kill process with!";
    my $proc = shift || confess "ERROR: No process to kill!";
    my $process = $proc->{pid};
    my $safesig;
    unless ($process=~/^\d+$/) {
        $gRetval |= ERR_SYSTEM_CALL;
        logError("$process is not a numeric PID, so we cannot kill it!");
        return $gRetval;
    }
    foreach my $sig (sort(keys(%gSignals))) { 
        if ($signal eq $sig) {
            $safesig = $signal;
            last;
        }
    }
    unless ($safesig or $signal=~/^\d+$/ or $signal > ($#gSignalno + 1)) {
        logError("$signal is unknown, so can't send it to $process!");
        $gRetval |= ERR_SYSTEM_CALL;
        return $gRetval;
    }
    my $error = kill($signal, $process);
    unless ($error) {
        # kill returns number of processes killed.
        $gRetval |= ERR_SYSTEM_CALL;
        logError("Could not kill PID $process with signal $signal - $!");
        return $gRetval;
    } else {
        logVerbose("successfully killed $error processes");
        return 0;
    }
}

sub readFile($$) {
    my $info = shift || confess "no info hash passed to readFile!";
    my $filename = shift || confess "no filename passed to readFile!";

    $filename =~/^(.*)[\/]([^\/]+)$/;

    my $file = findInPath($2, ["$1"]);
    if (defined($file->{path})) {
        my $error = open(SV, "<$file->{path}");
        unless ($error) {
            $gRetval |= ERR_FILE_ACCESS;
            logError("Can't open $file->{path}");
            return $gRetval;
        } else {
            while (<SV>) {
                logData($_);
            }
            close SV;
        }
    }
    return 0;

}

sub runTool($$$$;$) {
    my $info = shift || confess "no info hash passed to runTool!\n";
    my $opt = shift || confess "no opt hash passed to runTool!\n";
    my $tool = shift || confess "no tool to run passed to runTool!\n";
    my $action = shift || confess "no action passed to runTool!\n";
    my $filter = shift;

    logDebug("Attempting to run $tool");
    my $cmd="";
    my $data="";
    if (! -x $tool) {
        $cmd = "$info->{lw}->{path}/$tool 2>&1";
    } else {
        $cmd = "$tool 2>&1";
    }
    if ($action eq "bury") {
        $data=`$cmd 2>&1`;
        $data="" unless ($?);
    } elsif ($action eq "print") {
        open(RT, "$cmd |");
        while (<RT>) {
            logData("$_");
        }
        close RT;
        $data="";
    } elsif ($action eq "grep") {
        open(RT, "$cmd | ");
        my @results;
        while (<RT>) {
            if ($_=~/$filter/) {
                if ($1) {
                    push(@results, $1);
                } else {
                    push(@results, $_);
                }
            }
        }
        close RT;
        $data=join("\n", @results);
    } else { # ($action eq "return") 
        $data=`$cmd`;
    }
    if ($?) {
        $gRetval |= ERR_SYSTEM_CALL;
        logError("Error running $tool!");
        logInfo("$data");
        $data = "";
    }
    return $data;
}

sub safeRegex($) {
    my $line = shift || confess "no line to clean up for regex matching!\n";
    my $regex = $line;
    $regex=~s/([\*\[\]\-\(\)\.\?\/\^\\])/\\$1/g;
    logDebug("Cleaned up '$line' as '$regex'");
    return $regex;
}

sub safeUsername($) {
    my $name = shift || confess "No username to clean up!!\n";
    my $cleaned = $name;
    $cleaned=~s/\\\\/\\/g;
    $cleaned=~s/(\$\*\{\})/\\$1/g;
    $cleaned="'$cleaned'";
    logDebug("Cleaned up $name as $cleaned");
    return $cleaned;
}

sub sectionBreak($) {
    my $title = shift || confess "no title to print section break to!\n";
    logData(" ");
    logData("############################################");
    logData("# Section $title");
    logData("# ".scalar(localtime()));
    logData("# ");
    return 0;
}

sub System($;$$)
{
    my $command = shift || confess "No Command to launch passed to System!\n";
    my $print = shift;
    my $timeout = shift;

    if (defined($print) && $print=~/^\d+$/) {
        logDebug("RUN: $command");
    }

    if ($timeout) {
        my $pid = fork();
        if (not defined $pid) {
            logError("FORK failed for: $command");
            return 1;
        }
        if (not $pid) {
            exec("$command");
            exit($?);
        } else {
            my $rc;
            eval {
                local $SIG{ALRM} = sub { logError("ALARM: process timeout"); };
                alarm($timeout);
                my $child = waitpid($pid, 0);
                if ($child >= 0) {
                    $rc = GetErrorCodeFromChildError($?);
                } else {
                    $rc = 1;
                }
                alarm(0);
            };
            if ($@) {
                if ($@ =~ /ALARM: process timeout/) {
                    logError("\n*** PROCESS TIMED OUT ***\n");
                    killProc2(9, $pid);
                    $rc = 1;
                } else {
                    confess;
                }
            }
            return $rc;
        }
    } else {
        system("nohup $command");
        return GetErrorCodeFromChildError($?);
    }
}

sub tarFiles($$$$) {
    my $info = shift || confess "no info hash passed to tar appender!\n";
    my $opt = shift || confess "no options hash passed to tar appender!\n";
    my $tar = shift || confess "no tar file passed to tar appender!\n";
    my $file = shift || confess "no append file passed to tar appender!\n";

    logInfo("Adding file $file to $tar");
    my $error;
    if (-e $tar) {
        $error = System("tar -rf $tar $file > /dev/null 2>&1");
    } else {
        $error = System("tar -cf $tar $file > /dev/null 2>&1");
    }
    if ($error) {
        $gRetval |= ERR_SYSTEM_CALL;
        logError("Error $error adding $file to $tar - $!");
    }
}

sub tcpdumpStart($$) {
    my $info = shift || confess "No info hash passed to tcpdump()!\n";
    my $opt = shift || confess "No options hash passed to tcpdump()!\n";

    logInfo("starting tcpdump analogue for $info->{OStype}");
    my $dumpcmd = "$info->{tcpdump}->{startcmd} $info->{tcpdump}->{args} $opt->{capturefile} $info->{tcpdump}->{filter}";
    logVerbose("Trying to run: $dumpcmd");
    System("$dumpcmd &");
}

sub tcpdumpStop($$) {
    my $info = shift || confess "No info hash passed to tcpdump()!\n";
    my $opt = shift || confess "No options hash passed to tcpdump()!\n";

    logInfo("Stopping tcpdump analogue for $info->{OStype}");
    my $stopcmd = "$info->{tcpdump}->{stopcmd}";
    if ($stopcmd eq "kill") {
        logVerbose("Stopping tcpdump by killing it...");
        killProc("$info->{tcpdump}->{startcmd}", 9, $info);
    } else {
        logVerbose("Sending stop command...");
        my $error = System($stopcmd);
        if ($error) {
            killProc($info->{tcpdump}->{startcmd}, 9, $info);
        }
    }
}

# Helper Functions End
#####################################

#####################################
# Main Functions Below

sub changeLogging($$$) {
    my $info = shift || confess "no info hash passed to log starter!\n";
    my $opt = shift || confess "no options hash passed to log starter!\n";
    my $state = shift || confess "no start/stop state passed to log start!\n";
    logDebug("Determining restart ability");
    if ($opt->{restart} && $info->{uid} == 0) {
        logDebug("requested to restart daemons, beginning.");
        my $options = { daemon => "",
            loglevel => "$state",
        };
        if ($info->{lwsm}->{control} eq "lwsm") {
            if ($info->{lwsm}->{type} eq "standalone") {
                logVerbose("Restarting standalone daemons inside lwsm.");
                changeLoggingWithLwSm($info, $opt, $options);
            } elsif ($info->{lwsm}->{type} eq "container") {
                logVerbose("Restarting containerized daemons inside lwsm");
                changeLoggingWithContainer($info, $opt, $options);
            }
            return;
        } else {
            logVerbose("Restarting standalone daemons.");
            changeLoggingStandAlone($info, $opt, $options);
            return;
        }
    } else {
        if ($info->{uid} != 0) {
            logError("can't restart daemons or make syslog.conf changes");
            logError("This tool needs to be run as root for these options");
            $gRetval |= ERR_SYSTEM_CALL;
            return;
        }
        if ($info->{lwsm}->{type} eq "container") {
            logVerbose("Setting up tap-log log captures.");
            changeLoggingByTap($info, $opt, $state);
            return;
        } else {
            logVerbose("Setting up syslog.conf edited log captures.");
            changeLoggingBySyslog($info, $opt, $state);
            return;
        }
    }

    return;
}

sub changeLoggingByTap($$$) {
    my $info = shift || confess "no info hash passed to log starter!\n";
    my $opt = shift || confess "no options hash passed to log starter!\n";
    my $state = shift || confess "no start/stop state passed to log start!\n";
    $opt->{paclog} = "$info->{logpath}/lsass.log" if ($opt->{lsassd});
    foreach my $daemonname (keys(%{$info->{lw}->{daemons}})) {
        my $daemon = $info->{lw}->{daemons}->{$daemonname};
        if ($state eq "normal" and exists($info->{logging}->{$daemon}->{proc})) {
            logInfo("Killing tap for $daemon at pid $info->{logging}->{$daemon}->{proc}->{pid}.");
            killProc2(9, $info->{logging}->{$daemon}->{proc});
        } elsif ($state eq "normal") {
            logVerbose("Nothing to do for $daemon.");
        } else {
            logDebug("Checking if I need to tap $daemonname daemon $daemon...");
            my $daemonopt=$daemon."d";
            next unless(defined($opt->{$daemonopt}) and $opt->{$daemonopt});
            logInfo("Tapping $daemon daemon for $state mode.");
            my $tapscript = $info->{lw}->{path}."/".$info->{lwsm}->{control}." tap-log ";
            $tapscript = $tapscript.$daemon." - ";
            $tapscript = $tapscript.$state." > ";
            $tapscript = $tapscript.$info->{logpath}."/".$daemon.".log";
            logDebug("Running: $tapscript");
            my $result = System($tapscript." & ", 0, $opt->{delaytime});
            sleep 2;
            #Sleep required for background process startup on slower systems.
            $info->{logging}->{$daemon}->{proc}=findProcess("tap-log $daemon", $info);
        }
    };
}

sub changeLoggingBySyslog($$$) {
    my $info = shift || confess "no info hash passed to log starter!\n";
    my $opt = shift || confess "no options hash passed to log starter!\n";
    my $state = shift || confess "no start/stop state passed to log start!\n";
    $opt->{paclog} = "$info->{logpath}/$info->{logfile}";

    my ($error);
    if (not(defined($info->{logedit}->{file}))) {
        $info->{logedit} = {};
        $info->{logedit}->{line} = "*.*\t\t\t\t$info->{logpath}/$info->{logfile}";
        $info->{logedit}->{line} = "*.debug\t\t\t\t$info->{logpath}/$info->{logfile}" if ($info->{OStype} eq "solaris");
        $info->{logedit}->{file} = findInPath("syslog.conf", ["/etc", "/etc/syslog", "/opt/etc/", "/usr/local/etc/"]);
        if (not defined($info->{logedit}->{file}->{path})) {
            $info->{logedit}->{file} = findInPath("rsyslog.conf", ["/etc", "/etc/syslog", "/opt/etc/", "/usr/local/etc/", "/etc/rsyslog/"]);
        }
        if (not defined($info->{logedit}->{file}->{path})) {
            logError("Couldn't find syslog.conf or rsyslog.conf, and we don't support syslog-ng. Choose a different logging option!");
        }
    }
    if ($info->{logedit}->{file}->{type} eq "f") {
        if ($state eq "normal") {
            if (not defined($info->{logedit}->{line})) {
                logError("Don't know what to remove from syslog.conf!!");
                return;
            }
            logWarning("Removing debug logging from syslog.conf");
            lineDelete($info->{logedit}->{file}, $info->{logedit}->{line});
            logWarning("Changing log levels for PBIS daemons to $state");
            runTool($info, $opt, "$info->{lw}->{logging}->{netdaemon} error", "bury") if ($opt->{netlogond} and defined($info->{lw}->{logging}->{netdaemon}));
            runTool($info, $opt, "$info->{lw}->{logging}->{smbdaemon} error", "bury") if ($opt->{lwiod} and defined($info->{lw}->{logging}->{smbdaemon}));
            runTool($info, $opt, "$info->{lw}->{logging}->{authdaemon} error", "bury") if ($opt->{lsassd} and defined($info->{lw}->{logging}->{authdaemon}));
            runTool($info, $opt, "$info->{lw}->{logging}->{eventlogd} error", "bury") if ($opt->{eventlogd} and defined($info->{lw}->{logging}->{eventlogd}));
            runTool($info, $opt, "$info->{lw}->{logging}->{eventfwdd} error", "bury") if ($opt->{eventfwdd} and defined($info->{lw}->{logging}->{eventfwdd}));
            runTool($info, $opt, "$info->{lw}->{logging}->{syslogreaper} error", "bury") if ($opt->{reapsysld} and defined($info->{lw}->{logging}->{syslogreaper}));
            runTool($info, $opt, "$info->{lw}->{logging}->{regdaemon} error", "bury") if ($opt->{lwregd} and defined($info->{lw}->{logging}->{regdaemon}));
            runTool($info, $opt, "$info->{lw}->{logging}->{gpagent} error", "bury") if ($opt->{gpagentd} and defined($info->{lw}->{logging}->{gpagent}));
#TODO Put in changes for lw 4.1
        } else {
# Force the "messages" option on, since that's where we'll gather data from
            $opt->{messages} = 1;
            logWarning("system has syslog.conf, editing to capture debug logs");
            lineInsert($info->{logedit}->{file}, $info->{logedit}->{line});
            logWarning("Changing log levels for PBIS daemons to $state");
            runTool($info, $opt, "$info->{lw}->{logging}->{netdaemon} $state", "bury") if ($opt->{netlogond} and defined($info->{lw}->{logging}->{netdaemon}));
            runTool($info, $opt, "$info->{lw}->{logging}->{smbdaemon} $state", "bury") if ($opt->{lwiod} and defined($info->{lw}->{logging}->{smbdaemon}));
            runTool($info, $opt, "$info->{lw}->{logging}->{authdaemon} $state", "bury") if ($opt->{lsassd} and defined($info->{lw}->{logging}->{authdaemon}));
            if ($info->{OStype} eq "darwin" and $opt->{lsassd}) {
                my $odutil = findInPath("odutil", ["/usr/bin", "/bin", "/usr/sbin", "/sbin"]);
                if ($odutil->{perm}=~/x/) {
                    runTool($info, $opt, "$odutil->{path} set log debug", "bury");                    
                } else {
                    killProc("DirectoryService", "USR1", $info);
                }
            }
            runTool($info, $opt, "$info->{lw}->{logging}->{eventlogd} $state", "bury") if ($opt->{eventlogd} and defined($info->{lw}->{logging}->{eventlogd}));
            runTool($info, $opt, "$info->{lw}->{logging}->{eventfwdd} $state", "bury") if ($opt->{eventfwdd} and defined($info->{lw}->{logging}->{eventfwdd}));
            runTool($info, $opt, "$info->{lw}->{logging}->{syslogreaper} $state", "bury") if ($opt->{reapsysld} and defined($info->{lw}->{logging}->{syslogreaper}));
            runTool($info, $opt, "$info->{lw}->{logging}->{regdaemon} $state", "bury") if ($opt->{lwregd} and defined($info->{lw}->{logging}->{regdaemon}));
            if ($opt->{gpagentd} and defined($info->{lw}->{logging}->{gpagent})) {
                $state="verbose" if ($state eq "debug"); #TODO: 6.0.217 doesn't support "debug" with gp-set-log-level
                runTool($info, $opt, "$info->{lw}->{logging}->{gpagent} $state", "bury");
            }
        }
        killProc("syslog", 1, $info);
    } else {
        $gRetval |= ERR_FILE_ACCESS;
        logError("syslog.conf is not a file, could not edit!")
    }
}
sub changeLoggingStandalone($$$) {
    my $info = shift || confess "no info hash passed to standalone log starter!\n";
    my $opt = shift || confess "no opt hash passed to standalone log starter!\n";
    my $options = shift || confess "no options hash passed to standalone log starter!\n";
    my ($startscript, $logopts, $result);
    $opt->{paclog} = "$info->{logpath}/lsassd.log" if ($opt->{lsassd});

    if ($opt->{lwsmd} && defined($info->{lw}->{daemons}->{lwsm})) {
        logVerbose("Attempting restart of service controller");
        $options->{daemon} = $info->{lw}->{daemons}->{lwsm};
        daemonRestart($info, $options);
        sleep 5;
    }
    if ($opt->{lwregd} && defined($info->{lw}->{daemons}->{registry})) {
        logVerbose("Attempting restart of registry daemon");
        $options->{daemon} = $info->{lw}->{daemons}->{registry};
        daemonRestart($info, $options);
        sleep 5;
    }
    if ($opt->{dcerpcd} && defined($info->{lw}->{daemons}->{dcedaemon})) {
        logVerbose("Attempting restart of dce endpoint mapper daemon");
        $options->{daemon} = $info->{lw}->{daemons}->{dcedaemon};
        daemonRestart($info, $options);
        sleep 5;
    }
    if ($opt->{netlogond} && defined($info->{lw}->{daemons}->{netdaemon})) {
        logVerbose("Attempting restart of netlogon daemon");
        $options->{daemon} = $info->{lw}->{daemons}->{netdaemon};
        daemonRestart($info, $options);
        sleep 5;
    }
    if ($opt->{lwiod} && defined($info->{lw}->{daemons}->{smbdaemon})) {
        logVerbose("Attempting restart of SMB daemon");
        $options->{daemon} = $info->{lw}->{daemons}->{smbdaemon};
        daemonRestart($info, $options);
        sleep 5;
    }
    if ($opt->{eventlogd} && defined($info->{lw}->{daemons}->{eventlogd})) {
        logVerbose("Attempting restart of eventlog daemon");
        $options->{daemon} = $info->{lw}->{daemons}->{eventlogd};
        daemonRestart($info, $options);
        sleep 5;
    }
    if ($opt->{reapsysld} && defined($info->{lw}->{daemons}->{syslogreaper})) {
        logVerbose("Attempting restart of syslog reaper");
        $options->{daemon} = $info->{lw}->{daemons}->{syslogreaper};
        daemonRestart($info, $options);
        sleep 5;
    }
    if ($opt->{lsassd} && defined($info->{lw}->{daemons}->{authdaemon})) {
        logInfo("attempting restart of auth daemon");
        $options->{daemon} = $info->{lw}->{daemons}->{authdaemon};
        if ($info->{lw}->{version} eq "4.1") {
#TODO add code to edit lwiauthd.conf
        } 
        if ($info->{OStype} eq "darwin") {
            killProc("DirectoryService", "USR1", $info);
        }
        daemonRestart($info, $options);
        sleep 5;
    }
    if ($opt->{gpagentd} && defined($info->{lw}->{daemons}->{gpdaemon})) {
        logVerbose("Attempting restart of Group Policy daemon");
        if ($info->{lw}->{version} < 5.3) {
            # not needed in LW 5.3.7724 and later
            $options->{loglevel} = 5 if ($options->{state} eq "debug");
            $options->{loglevel} = 4 if ($options->{state} eq "verbose");
            $options->{loglevel} = 3 if ($options->{state} eq "info"); 
            $options->{loglevel} = 2 if ($options->{state} eq "warning");
            $options->{loglevel} = 1 if ($options->{state} eq "error");
        }
        $options->{daemon} = $info->{lw}->{daemons}->{gpdaemon};
        daemonRestart($info, $options);
        sleep 5;
    }
    if ($opt->{eventfwdd} && defined($info->{lw}->{daemons}->{eventfwdd})) {
        logVerbose("Attempting restart of event forwarder daemon");
        $options->{daemon} = $info->{lw}->{daemons}->{eventfwdd};
        daemonRestart($info, $options);
        sleep 5;
    }
    if ($opt->{lwscd} && defined($info->{lw}->{daemons}->{smartcard})) {
        logVerbose("Attempting restart of smartcard daemon");
        $options->{daemon} = $info->{lw}->{daemons}->{smartcard};
        daemonRestart($info, $options);
        sleep 5;
    }
    if ($opt->{lwscd} && defined($info->{lw}->{daemons}->{pkcs11})) {
        logVerbose("Attempting restart of pkcs11 daemon");
        $options->{daemon} = $info->{lw}->{daemons}->{pkcs11};
        daemonRestart($info, $options);
        sleep 5;
    }

}

sub changeLoggingWithContainer($$$) {
    my $info = shift;
    my $opt = shift;
    my $options = shift;
    my ($startscript, $logopts, $result);
    $opt->{paclog} = "$info->{logpath}/lsassd.log" if ($opt->{lsassd});
    
    foreach my $daemonname (keys(%{$info->{lw}->{daemons}})) {
        my $daemon = $info->{lw}->{daemons}->{$daemonname};
        logDebug("Checking if I need to restart $daemonname daemon $daemon...");
        my $daemonopt=$daemon."d";
        next unless(defined($opt->{$daemonopt}) and $opt->{$daemonopt});
        logInfo("Stopping $daemon daemon for $options->{loglevel} mode.");
        $options->{daemon} = $daemon;
        daemonContainerStop($info, $options);
    };
    foreach my $daemon(qw(netlogon lwio eventlog lsass gpagent eventfwd usermonitor reapsysl lwpcks11 lwsc)){
        logDebug("Checking if I need to restart $daemon daemon...");
        my $daemonopt=$daemon."d";
        next unless(defined($opt->{$daemonopt}) and $opt->{$daemonopt});
        logInfo("Starting $daemon daemon for $options->{loglevel} mode.");
        $options->{daemon} = $daemon;
        daemonContainerStart($info, $options);
    };
}
sub changeLoggingWithLwSm($$$) {
    my $info=shift;
    my $opt = shift;
    my $options=shift;
    my ($startscript, $logopts, $result);
    $opt->{paclog} = "$info->{logpath}/lsassd.log" if ($opt->{lsassd});
    foreach my $daemonname (sort(keys(%{$info->{lw}->{daemons}}))) {
        my $daemon = $info->{lw}->{daemons}->{$daemonname};
        next unless(defined($opt->{$daemon}) and $opt->{$daemon});
        logInfo("Setting $daemonname $daemon for $options->{loglevel} mode.");
        $daemon=~s/d$//;
        if ($daemon eq "dcerpc") {
            if ($options->{loglevel} eq "normal" or $options->{loglevel} eq "error") {
                $logopts = " -f";
            } else {
                $logopts = " -D -f > ".$info->{logpath}."/".$daemon.".log";
            }
        } elsif ($daemon eq "gpagent" or $daemon eq "eventfwd") {
            if ($options->{loglevel} eq "normal" or $options->{loglevel} eq "error") {
                $logopts = ""; #TODO LW 6.0.207 doesn't support --syslog for gpagentd or eventfwd
            } else {
                $logopts = " --loglevel $options->{loglevel} --logfile ".$info->{logpath}."/".$daemon."d.log";
            }
        } else {
            if ($options->{loglevel} eq "normal" or $options->{loglevel} eq "error") {
                $logopts = " --syslog";
            } else {
                $logopts = " --loglevel $options->{loglevel} --logfile ".$info->{logpath}."/".$daemon."d.log";
            }
        }
        logVerbose("$daemon will be run with options '$logopts'");
        $startscript = $info->{lw}->{path}."/".$info->{lw}->{tools}->{regshell}.' set_value "[HKEY_THIS_MACHINE\\Services\\'.$daemon.']" "Arguments" "'.$info->{lw}->{base}.'/sbin/'.$daemon.'d'.$logopts.'"';
        logDebug("running $startscript:");
        System($startscript, 5, 5);
    }
    $options->{daemon} = $info->{lw}->{daemons}->{lwsm};
    daemonRestart($info, $options);
    sleep 5;
}

sub determineOS($) {
    my $info = shift || confess "no info hash passed";
    logDebug("Determining OS Type...");
    my $file={};
    my $uname;

    $info->{pscmd} = "ps -ef";
    if ($^O eq "linux") {
        foreach my $i (("rpm", "dpkg")) {
            $file=findInPath($i, ["/sbin", "/usr/sbin", "/usr/bin", "/bin", "/usr/local/bin", "/usr/local/sbin"]);
            if ((defined($file->{path}))) { # && $file->{perm}=~/x/) {
                $info->{OStype} = "linux-$i";
                logVerbose("System is $info->{OStype}");
            } 
        }
        $info->{timezonefile} = "/etc/sysconfig/clock";
        if (not defined($info->{OStype})) {
            $gRetval |= ERR_OS_INFO;
            logWarning("Could not determine Linux subtype");
            $info->{OStype} = "linux-unknown";
        } elsif ($info->{OStype} eq "linux-dpkg") {
            $info->{OStype} = "linux-deb";
            $info->{timezonefile} = "/etc/timezone";
        }
        logVerbose("Setting Linux paths");
        $info->{svcctl}->{start1} = "/etc/init.d/";
        $info->{svcctl}->{start2} = "daemonname";
        $info->{svcctl}->{start3} = " start";
        $info->{svcctl}->{stop1} = "/etc/init.d/";
        $info->{svcctl}->{stop2} = "daemonname";
        $info->{svcctl}->{stop3} = " stop";
        $info->{svcctl}->{rcpath} = "/etc/rc.d";
        $info->{tcpdump}->{startcmd} = "tcpdump";
        $info->{tcpdump}->{args} = "-s0 -w";
        $info->{tcpdump}->{filter} = "not port 22";
        $info->{tcpdump}->{stopcmd} = "kill";
        $info->{sshd}->{opts} = "-ddd -p 22226";
        $info->{pampath} = "/etc/pam.d";
        foreach my $i (("syslog", "system.log", "messages")) {
            $file = findInPath($i, ["/var/log"]);
            if ((defined($file->{path})) && $file->{type} eq "f") {
                $info->{logfile} = "$i";
                $info->{logpath} = $file->{dir};
            }
        }
        $info->{nsfile} = "/etc/nsswitch.conf";
    } elsif ($^O eq "hpux") {
        $info->{OStype} = "hpux";
        logVerbose("Setting HP-UX paths");
        $info->{release} = `swlist -l bundle`;
        $info->{svcctl}->{start1} = "/sbin/init.d/";
        $info->{svcctl}->{start2} = "daemonname";
        $info->{svcctl}->{start3} = " start";
        $info->{svcctl}->{stop1} = "/sbin/init.d/";
        $info->{svcctl}->{stop2} = "daemonname";
        $info->{svcctl}->{stop3} = " stop";
        $info->{svcctl}->{rcpath} = "/etc/rc.d";
        $info->{tcpdump}->{startcmd} = "nettl -start; nettl";
        $info->{tcpdump}->{args} = "-traceon pduin pduout -e ns_ls_driver -file";
        $info->{tcpdump}->{stopcmd} = "nettl -traceoff\; nettl -stop";
        $info->{sshd}->{opts} = "-ddd -p 22226 ";
        $info->{pampath} = "/etc/pam.conf";
        $info->{logpath} = "/var/adm";
        $info->{logfile} = "messages";
        $info->{nsfile} = "/etc/nsswitch.conf";
        $info->{timezonefile} = "/etc/TIMEZONE";
    } elsif ($^O eq "solaris") {
        $info->{OStype} = "solaris";
        logVerbose("Setting Solaris paths");
        $file = findInPath("svcadm", ["/usr/sbin", "/sbin"]);
        if ((defined($file->{path})) && $file->{type} eq "f") {
            $info->{svcctl}->{start1} = "$file->{path} ";
            $info->{svcctl}->{start2} = "enable -t ";
            $info->{svcctl}->{start3} = "daemonname";
            $info->{svcctl}->{stop1} = "$file->{path}";
            $info->{svcctl}->{stop2} = " disable -t ";
            $info->{svcctl}->{stop3} = "daemonname";
            $info->{pscmd} = "ps -z `/usr/bin/zonename` -f";
            $info->{zonename}=`zonename`;
            logData("Solaris 10 zonename is: ".$info->{zonename});
            if (`pkgcond is_global_zone;echo $?` == 0) {
                logData("Solaris 10 zone type is global.") 
            } elsif (`pkgcond is_whole_root_nonglobal_zone;echo $?` == 0) {
                logData("Solaris 10 zone type is whole root child zone.");
            } elsif (`pkgcond is_sparse_root_nonglobal_zone;echo $?` == 0) {
                logData("Solaris 10 zone type is sparse root child zone.");
            }
        } else {
            $info->{svcctl}->{start1} = "/etc/init.d/";
            $info->{svcctl}->{start2} = "daemonname";
            $info->{svcctl}->{start3} = " start";
            $info->{svcctl}->{stop1} = "/etc/init.d/";
            $info->{svcctl}->{stop2} = "daemonname";
            $info->{svcctl}->{stop3} = " stop";
        }
        $info->{svcctl}->{rcpath} = "/etc/rc.d";
        $info->{tcpdump}->{startcmd} = "snoop";
        $info->{tcpdump}->{args} = "-s0 -o";
        $info->{tcpdump}->{filter} = "not port 22";
        $info->{tcpdump}->{stopcmd} = "kill";
        $info->{sshd}->{opts} = "-ddd -p 22226 ";
        $info->{pampath} = "/etc/pam.conf";
        $info->{logpath} = "/var/adm";
        $info->{logfile} = "messages";
        $info->{nsfile} = "/etc/nsswitch.conf";
        $info->{timezonefile} = "/etc/TIMEZONE";
    } elsif ($^O eq "aix") {
        $info->{OStype} = "aix";
        logData("System release is:");
        logData(`oslevel -r`);
        logVerbose("Setting AIX paths");
        $info->{svcctl}->{start1} = "/etc/rc.d/init.d/";
        $info->{svcctl}->{start2} = "daemonname";
        $info->{svcctl}->{start3} = " start";
        $info->{svcctl}->{stop1} = "/etc/rc.d/init.d/";
        $info->{svcctl}->{stop2} = "daemonname";
        $info->{svcctl}->{stop3} = " stop";
        $info->{svcctl}->{rcpath} = "/etc/rc.d";
        $info->{tcpdump}->{startcmd} = "/usr/sbin/iptrace";
        $info->{tcpdump}->{args} = "-a";
        $info->{tcpdump}->{stopcmd} = "kill";
        $info->{sshd}->{opts} = "-ddd -p 22226 ";
        $info->{pampath} = "/etc/pam.conf";
        $info->{logpath} = "/var/adm";
        $info->{logfile} = "syslog/syslog.log";
        $info->{nsfile} = "/etc/netsvc.conf";
        $info->{timezonefile} = "/etc/environment";
    } elsif ($^O eq "MacOS" or $^O eq "darwin") {
        $info->{OStype} = "darwin";
        logVerbose("Setting darwin paths");
        $info->{svcctl}->{start1} = "launchctl";
        $info->{svcctl}->{start2} = " start";
        $info->{svcctl}->{start3} = ' com.likewisesoftware.daemonname';
        $info->{svcctl}->{stop1} = "launchctl";
        $info->{svcctl}->{stop2} = " stop";
        $info->{svcctl}->{stop3} = ' com.likewisesoftware.daemonname';
        $info->{svcctl}->{rcpath} = "/etc/rc.d";
        $info->{tcpdump}->{startcmd} = "tcpdump";
        $info->{tcpdump}->{args} = "-s0 -w";
        $info->{tcpdump}->{filter} = "not port 22";
        $info->{tcpdump}->{stopcmd} = "kill";
        $info->{sshd}->{opts} = "-ddd -p 22226 ";
        $info->{pampath} = "/etc/pam.d";
        $info->{logpath} = "/var/log";
        $info->{logfile} = "system.log";
        $info->{nsfile} = "/etc/nsswitch.conf";
        $info->{timezonefile} = "/etc/localtime";
    } else {
        $gRetval |= ERR_OS_INFO;
        $info->{OStype} = "unknown";
        logError("ERROR: Could not determine OS information!");
        $info->{timezonefile} = "/etc/localtime";
        $info->{svcctl}->{start1} = "/etc/init.d/";
        $info->{svcctl}->{start2} = "daemonname";
        $info->{svcctl}->{start3} = " start";
        $info->{svcctl}->{stop1} = "/etc/init.d/";
        $info->{svcctl}->{stop2} = "daemonname";
        $info->{svcctl}->{stop3} = " stop";
        $info->{svcctl}->{rcpath} = "/etc/rc.d";
        $info->{tcpdump}->{startcmd} = "tcpdump";
        $info->{tcpdump}->{args} = "-s0 -w";
        $info->{tcpdump}->{filter} = "not port 22";
        $info->{tcpdump}->{stopcmd} = "kill";
        $info->{sshd}->{opts} = "-ddd -p 22226 ";
        $info->{pampath} = "/etc/pam.d";
        $info->{logpath} = "/var/log";
        $info->{logfile} = "messages";
        $info->{nsfile} = "/etc/nsswitch.conf";
    }
    logData("OS: $info->{OStype}");

    $info->{platform} = $Config{myarchname};
    $info->{platform}=~s/-.*$//;
    $info->{osversion} = `uname -r`;
    $info->{uname} = `uname -a`;
    logData("Version: $info->{osversion}");
    logData("Platform: $info->{platform}");
    logData("Full Uname: $info->{uname}");
    logData("System release is:");
    foreach my $i (("lsb-release", "release", "redhat-release", "ubuntu-release", "debian-release", "novell-release", "SuSE-release")) {
        $file = findInPath($i, ["/etc"]);
        if ((defined($file->{path})) && $file->{type} eq "f") {
            readFile($info, $file->{path});
        }
    }
    logData("LD_LIBRARY_PATH is: $ENV{LD_LIBRARY_PATH}");
    logData("LD_PRELOAD is: $ENV{LD_PRELOAD}");
    $info->{logon} = getlogin();
    $info->{name} = getpwuid($<);
    $info->{uid} = getpwnam($info->{name});
    logInfo("Currently running as: $info->{name} with effective UID: $info->{uid}");
    logInfo("Run under sudo from $info->{logon}") if ($info->{logon} ne $info->{name});
    logInfo("Gathered at: ".scalar(localtime));
    $info->{sshd} = findProcess("/sshd", $info);
    $info->{sshd_config} = findInPath("sshd_config", ["/etc/ssh", "/opt/ssh/etc", "/usr/local/etc", "/etc", "/etc/openssh", "/usr/openssh/etc", "/opt/csw/etc", "/services/ssh/etc/"]);
    $info->{krb5conf} = findInPath("krb5.conf", ["/etc/krb5", "/opt/krb5/etc", "/usr/local/etc", "/usr/local/etc/krb5", "/etc", "/opt/csw/etc"]);
    $info->{sudoers} = findInPath("sudoers", ["/etc", "/usr/local/etc", "/opt/etc", "/opt/local/etc", "/opt/usr/local/etc"]);
    $info->{sambaconf} = findInPath("smb.conf", ["/etc/samba", "/etc/smb", "/opt/etc/samba", "/usr/local/etc/samba", "/etc/opt/samba"]);
    $info->{resolvconf} = findInPath("resolv.conf", ["/etc", "/opt/etc"]);
    $info->{hostsfile} = findInPath("hosts", ["/etc"]);
    logData("Found sshd_config at $info->{sshd_config}->{path}") if ($info->{sshd_config}->{path});
    logData("Found krb5.conf at $info->{krb5conf}->{path}") if ($info->{krb5conf}->{path});
    logData("Found sudoers at $info->{sudoers}->{path}") if ($info->{sudoers}->{path});
    logData("Found smb.conf at $info->{sambaconf}->{path}") if ($info->{sambaconf}->{path});
    return $info;
}

sub waitForDomain($$) {
    my $info = shift || confess "no info hash passed to waitForDomain!\n";
    my $opt = shift || confess "no options hash passed to waitForDomain!\n";
    logInfo("Waiting up to 120 seconds for auth daemon to find domains and finish startup.");
    my ($error, $i) = (0,0);
    for ($i = 0; $i < 24; $i++) {
        sleep 5;
        $error = System("$info->{lw}->{path}/$info->{lw}->{tools}->{status} >/dev/null 2>&1");
        last unless $error;
# lw-get-status returns 0 for success, 2 if lsassd hasn't started yet
    }
}

sub getLikewiseVersion($) {

# determine PBIS / Likewise version installed
# look in reverse order, in case a bad upgrade was done
# we can get the current running version

    my $info = shift;
    my $error = 0;
    my $versionFile = findInPath("ENTERPRISE_VERSION", ["/opt/pbis/data", "/opt/likewise/data", "/usr/centeris/data", "/opt/centeris/data"]);
    $versionFile = findInPath("VERSION", ["/opt/pbis/data", "/opt/likewise/data", "/usr/centeris/data", "/opt/centeris/data"]) unless(defined($versionFile->{path}) && $versionFile->{path});
    if (defined($versionFile->{path})) {
        open(VF, "<$versionFile->{path}");
        while (<VF>) {
            /VERSION=(.*)/;
            $info->{lw}->{version} = $1 if ($1 and not defined($info->{lw}->{version}));
        }
        close VF;
        my @tmparray = split(/\./, $info->{lw}->{version});
        $info->{lw}->{majorVersion} = $tmparray[0];
        logDebug("PBIS $info->{lw}->{majorVersion} is $info->{lw}->{version}.");
    } else {
        logInfo("No Version File found, determining version from binaries installed");
        my $lwsmd = findInPath("lwsmd", ["/opt/pbis/sbin"]);
        my $lwsvcd = findInPath("lwsmd", ["/opt/likewise/sbin/"]);
        my $lwregd = findInPath("lwregd", ["/opt/likewise/sbin/"]);
        my $lwiod = findInPath("lwiod", ["/opt/likewise/sbin/"]);
        my $lwrdrd = findInPath("lwrdrd", ["/opt/likewise/sbin/"]);
        my $npcmuxd = findInPath("npcmuxd", ["/opt/likewise/sbin/"]);
        my $winbindd = findInPath("winbindd", ["/opt/centeris/sbin/", "/usr/centeris/sbin"]);
        if (defined($lwsmd->{path})) {
            $info->{lw}->{version} = "7.0.0";
        } elsif  (defined($lwsvcd->{path})) {
            $info->{lw}->{version} = "6.0.0";
        } elsif (defined($lwregd->{path})) {
            $info->{lw}->{version} = "5.3.0";
        } elsif (defined($lwiod->{path})) {
            $info->{lw}->{version} = "5.2.0";
        } elsif (defined($lwrdrd->{path})) {
            $info->{lw}->{version}= "5.1.0";
        } elsif (defined($npcmuxd->{path})) {
            $info->{lw}->{version}= "5.0.0";
        } elsif (defined($winbindd->{path})) {
            $info->{lw}->{version}= "4.1";
        }
    }
    my $gporefresh = findInPath("gporefresh", ["/opt/centeris/bin/", "/usr/centeris/bin", "/opt/likewise/bin", "/opt/pbis/bin"]);
    if ($info->{lw}->{version}=~/^8\.\d+\./) {
        $info->{lw}->{base} = "/opt/pbis";
        $info->{lw}->{path} = "/opt/pbis/bin";
        $info->{lw}->{daemons}->{smbdaemon} = "lwio";
        $info->{lw}->{daemons}->{authdaemon} = "lsass";
        $info->{lw}->{daemons}->{gpdaemon} = "gpagent" if (defined($gporefresh->{path}));
        $info->{lw}->{daemons}->{dcedaemon} = "dcerpc";
        $info->{lw}->{daemons}->{netdaemon} = "netlogon";
        $info->{lw}->{daemons}->{eventlogd} = "eventlog";
        $info->{lw}->{daemons}->{eventfwdd} = "eventfwd";
        $info->{lw}->{daemons}->{syslogreaper} = "reapsysl";
        $info->{lw}->{daemons}->{registry} = "lwreg";
        $info->{lw}->{daemons}->{lwsm} = "lwsm";
        $info->{lw}->{daemons}->{usermonitor} = "usermonitor";
        $info->{lw}->{daemons}->{smartcard} = "lwsc";
        $info->{lw}->{daemons}->{pkcs11} = "lwpkcs11";
        $info->{lw}->{logging}->{command} = "lwsm set-log-level";
        $info->{lw}->{logging}->{tapcommand} = "lwsm tap-log";
        $info->{lw}->{logging}->{registry} = "";
        $info->{lwsm}->{control}="lwsm";
        $info->{lwsm}->{type}="container";
        $info->{lwsm}->{initname}="pbis";
        $info->{lw}->{tools}->{findsid} = "find-objects --by-sid";
        $info->{lw}->{tools}->{userlist} = "enum-users --level 2";
        $info->{lw}->{tools}->{grouplist} = "enum-groups --level 1";
        $info->{lw}->{tools}->{userbyname} = "find-objects --user --by-name";
        $info->{lw}->{tools}->{userbyid} = "find-objects --user --by-unix-id";
        $info->{lw}->{tools}->{groupbyname} = "find-objects --group --by-name";
        $info->{lw}->{tools}->{groupbyid} = "find-objects --group --by-unix-id";
        $info->{lw}->{tools}->{groupsforuser} = "list-groups-for-user --show-sid";
        $info->{lw}->{tools}->{groupsforuid} = "list-groups-for-user --show-sid --uid";
        $info->{lw}->{tools}->{dctime} = "get-dc-time";
        $info->{lw}->{tools}->{config} = "config --dump";
        $info->{lw}->{tools}->{status} = "get-status";
        $info->{lw}->{tools}->{regshell} = "regshell";
        logData("PBIS Version $info->{lw}->{version} installed");
    }  elsif ($info->{lw}->{version}=~/^7\.\d+\./) {
        $info->{lw}->{base} = "/opt/pbis";
        $info->{lw}->{path} = "/opt/pbis/bin";
        $info->{lw}->{daemons}->{smbdaemon} = "lwio";
        $info->{lw}->{daemons}->{authdaemon} = "lsass";
        $info->{lw}->{daemons}->{gpdaemon} = "gpagent" if (defined($gporefresh->{path}));
        $info->{lw}->{daemons}->{dcedaemon} = "dcerpc";
        $info->{lw}->{daemons}->{netdaemon} = "netlogon";
        $info->{lw}->{daemons}->{eventlogd} = "eventlog";
        $info->{lw}->{daemons}->{eventfwdd} = "eventfwd";
        $info->{lw}->{daemons}->{syslogreaper} = "reapsysl";
        $info->{lw}->{daemons}->{registry} = "lwreg";
        $info->{lw}->{daemons}->{lwsm} = "lwsm";
        $info->{lw}->{daemons}->{usermonitor} = "usermonitor";
        $info->{lw}->{daemons}->{smartcard} = "lwsc";
        $info->{lw}->{daemons}->{pkcs11} = "lwpkcs11";
        $info->{lw}->{logging}->{command} = "lwsm set-log-level";
        $info->{lw}->{logging}->{tapcommand} = "lwsm tap-log";
        $info->{lw}->{logging}->{registry} = "";
        $info->{lwsm}->{control}="lwsm";
        $info->{lwsm}->{type}="container";
        $info->{lwsm}->{initname}="pbis";
        $info->{lw}->{tools}->{findsid} = "find-objects --by-sid";
        $info->{lw}->{tools}->{userlist} = "enum-users --level 2";
        $info->{lw}->{tools}->{grouplist} = "enum-groups --level 1";
        $info->{lw}->{tools}->{userbyname} = "find-objects --user --by-name";
        $info->{lw}->{tools}->{userbyid} = "find-objects --user --by-unix-id";
        $info->{lw}->{tools}->{groupbyname} = "find-objects --group --by-name";
        $info->{lw}->{tools}->{groupbyid} = "find-objects --group --by-unix-id";
        $info->{lw}->{tools}->{groupsforuser} = "list-groups-for-user --show-sid";
        $info->{lw}->{tools}->{groupsforuid} = "list-groups-for-user --show-sid --uid";
        $info->{lw}->{tools}->{dctime} = "get-dc-time";
        $info->{lw}->{tools}->{config} = "config --dump";
        $info->{lw}->{tools}->{status} = "get-status";
        $info->{lw}->{tools}->{regshell} = "regshell";
        logData("PBIS Version $info->{lw}->{version} installed");
    } elsif ($info->{lw}->{version}=~/^6\.5\./) {
        $info->{lw}->{base} = "/opt/pbis";
        $info->{lw}->{path} = "/opt/pbis/bin";
        $info->{lw}->{daemons}->{smbdaemon} = "lwio";
        $info->{lw}->{daemons}->{authdaemon} = "lsass";
        $info->{lw}->{daemons}->{gpdaemon} = "gpagent" if (defined($gporefresh->{path}));
        $info->{lw}->{daemons}->{dcedaemon} = "dcerpc";
        $info->{lw}->{daemons}->{netdaemon} = "netlogon";
        $info->{lw}->{daemons}->{eventlogd} = "eventlog";
        $info->{lw}->{daemons}->{eventfwdd} = "eventfwd";
        $info->{lw}->{daemons}->{syslogreaper} = "reapsysl";
        $info->{lw}->{daemons}->{registry} = "lwreg";
        $info->{lw}->{daemons}->{lwsm} = "lwsm";
        $info->{lw}->{daemons}->{smartcard} = "lwsc";
        $info->{lw}->{daemons}->{pkcs11} = "lwpkcs11";
        $info->{lw}->{daemons}->{startcmd} = "--start-as-daemon";
        $info->{lw}->{logging}->{command} = "lwsm set-log-level";
        $info->{lw}->{logging}->{tapcommand} = "lwsm tap-log";
        $info->{lw}->{logging}->{registry} = "";
        $info->{lwsm}->{control}="lwsm";
        $info->{lwsm}->{type}="container";
        $info->{lwsm}->{initname}="pbis";
        $info->{lw}->{tools}->{findsid} = "find-objects --by-sid";
        $info->{lw}->{tools}->{userlist} = "enum-users --level 2";
        $info->{lw}->{tools}->{grouplist} = "enum-groups --level 1";
        $info->{lw}->{tools}->{userbyname} = "find-objects --user --by-name";
        $info->{lw}->{tools}->{userbyid} = "find-objects --user --by-unix-id";
        $info->{lw}->{tools}->{groupbyname} = "find-objects --group --by-name";
        $info->{lw}->{tools}->{groupbyid} = "find-objects --group --by-unix-id";
        $info->{lw}->{tools}->{groupsforuser} = "list-groups-for-user --show-sid";
        $info->{lw}->{tools}->{groupsforuid} = "list-groups-for-user --show-sid --uid";
        $info->{lw}->{tools}->{dctime} = "get-dc-time";
        $info->{lw}->{tools}->{config} = "config --dump";
        $info->{lw}->{tools}->{status} = "get-status";
        $info->{lw}->{tools}->{regshell} = "regshell";
        logData("PBIS Version 6.5 installed");
    } elsif ($info->{lw}->{version} == "6.0.0") {
        $info->{lw}->{base} = "/opt/likewise";
        $info->{lw}->{path} = "/opt/likewise/bin";
        $info->{lw}->{daemons}->{smbdaemon} = "lwiod";
        $info->{lw}->{daemons}->{authdaemon} = "lsassd";
        $info->{lw}->{daemons}->{gpdaemon} = "gpagentd" if (defined($gporefresh->{path}));
        $info->{lw}->{daemons}->{dcedaemon} = "dcerpcd";
        $info->{lw}->{daemons}->{netdaemon} = "netlogond";
        $info->{lw}->{daemons}->{eventlogd} = "eventlogd";
        $info->{lw}->{daemons}->{eventfwdd} = "eventfwdd";
        $info->{lw}->{daemons}->{syslogreaper} = "reapsysld";
        $info->{lw}->{daemons}->{registry} = "lwregd";
        $info->{lw}->{daemons}->{lwsm} = "lwsmd";
        $info->{lw}->{daemons}->{startcmd} = "--start-as-daemon";
        $info->{lw}->{logging}->{smbdaemon} = "lwio-set-log-level";
        $info->{lw}->{logging}->{authdaemon} = "lw-set-log-level";
        $info->{lw}->{logging}->{netdaemon} = "lwnet-set-log-level";
        $info->{lw}->{logging}->{eventfwdd} = "evtfwd-set-log-level";
        $info->{lw}->{logging}->{syslogreaper} = "rsys-set-log-level";
        $info->{lw}->{logging}->{gpagent} = "gp-set-log-level";
        $info->{lw}->{logging}->{registry} = "";
        $info->{lwsm}->{control}="lwsm";
        $info->{lwsm}->{type}="standalone";
        $info->{lwsm}->{initname}="likewise-open";
        $info->{lw}->{tools}->{findsid} = "lw-find-by-sid";
        $info->{lw}->{tools}->{userlist} = "lw-enum-users --level 2";
        $info->{lw}->{tools}->{grouplist} = "lw-enum-groups --level 1";
        $info->{lw}->{tools}->{userbyname} = "lw-find-user-by-name --level 2";
        $info->{lw}->{tools}->{userbyid} = "lw-find-user-by-id --level 2";
        $info->{lw}->{tools}->{groupbyname} = "lw-find-group-by-name --level 1";
        $info->{lw}->{tools}->{groupbyid} = "lw-find-group-by-id --level 1";
        $info->{lw}->{tools}->{groupsforuser} = "lw-list-groups-for-user --show-sid";
        $info->{lw}->{tools}->{groupsforuid} = "lw-list-groups-for-user --show-sid --uid";
        $info->{lw}->{tools}->{dctime} = "lw-get-dc-time";
        $info->{lw}->{tools}->{status} = "lw-get-status";
        $info->{lw}->{tools}->{config} = "lwconfig --dump";
        $info->{lw}->{tools}->{regshell} = "lwregshell";
        logData("Likewise Version 6.0 installed");
    } elsif ($info->{lw}->{version} == "5.4.0") {
        $info->{lw}->{base} = "/opt/likewise";
        $info->{lw}->{path} = "/opt/likewise/bin";
        $info->{lw}->{daemons}->{smbdaemon} = "lwiod";
        $info->{lw}->{daemons}->{authdaemon} = "lsassd";
        $info->{lw}->{daemons}->{gpdaemon} = "gpagentd" if (defined($gporefresh->{path}));
        $info->{lw}->{daemons}->{dcedaemon} = "dcerpcd";
        $info->{lw}->{daemons}->{netdaemon} = "netlogond";
        $info->{lw}->{daemons}->{eventlogd} = "eventlogd";
        $info->{lw}->{daemons}->{eventfwdd} = "eventfwdd";
        $info->{lw}->{daemons}->{syslogreaper} = "reapsysld";
        $info->{lw}->{daemons}->{registry} = "lwregd";
        $info->{lw}->{daemons}->{lwsm} = "lwsmd";
        $info->{lw}->{daemons}->{startcmd} = "--start-as-daemon";
        $info->{lw}->{logging}->{smbdaemon} = "lwio-set-log-level";
        $info->{lw}->{logging}->{authdaemon} = "lw-set-log-level";
        $info->{lw}->{logging}->{netdaemon} = "lwnet-set-log-level";
        $info->{lw}->{logging}->{eventfwdd} = "evtfwd-set-log-level";
        $info->{lw}->{logging}->{syslogreaper} = "rsys-set-log-level";
        $info->{lw}->{logging}->{registry} = "reg-set-log-level";
        $info->{lwsm}->{control}="lwsm";
        $info->{lwsm}->{type}="standalone";
        $info->{lwsm}->{initname}="likewise-open";
        $info->{lw}->{tools}->{findsid} = "lw-find-by-sid";
        $info->{lw}->{tools}->{userlist} = "lw-enum-users --level 2";
        $info->{lw}->{tools}->{grouplist} = "lw-enum-groups --level 1";
        $info->{lw}->{tools}->{userbyname} = "lw-find-user-by-name --level 2";
        $info->{lw}->{tools}->{userbyid} = "lw-find-user-by-id --level 2";
        $info->{lw}->{tools}->{groupbyname} = "lw-find-group-by-name --level 1";
        $info->{lw}->{tools}->{groupbyid} = "lw-find-group-by-id --level 1";
        $info->{lw}->{tools}->{groupsforuser} = "lw-list-groups-for-user --show-sid";
        $info->{lw}->{tools}->{groupsforuid} = "lw-list-groups-for-user --show-sid --uid";
        $info->{lw}->{tools}->{dctime} = "lw-get-dc-time";
        $info->{lw}->{tools}->{config} = "lwconfig --dump";
        $info->{lw}->{tools}->{status} = "lw-get-status";
        logData("Likewise Version 5.4 installed");
    } elsif ($info->{lw}->{version} == "5.3.0") {
        $info->{lw}->{base} = "/opt/likewise";
        $info->{lw}->{path} = "/opt/likewise/bin";
        $info->{lw}->{daemons}->{lwsm} = "lwsmd";
        $info->{lw}->{daemons}->{smbdaemon} = "lwiod";
        $info->{lw}->{restart}->{smbdaemon} = "lwsm restart lwio";
        $info->{lw}->{daemons}->{authdaemon} = "lsassd";
        $info->{lw}->{restart}->{authdaemon} = "lwsm restart lsass";
        $info->{lw}->{daemons}->{gpdaemon} = "gpagentd" if (defined($gporefresh->{path}));
        $info->{lw}->{restart}->{gpdaemon} = "lwsm restart gpagentd" if (defined($gporefresh->{path}));
        $info->{lw}->{daemons}->{dcedaemon} = "dcerpcd";
        $info->{lw}->{restart}->{dcedaemon} = "lwsm restart dcerpc";
        $info->{lw}->{daemons}->{netdaemon} = "netlogond";
        $info->{lw}->{restart}->{netdaemon} = "lwsm restart netlogon";
        $info->{lw}->{daemons}->{eventlogd} = "eventlogd";
        $info->{lw}->{restart}->{eventlogd} = "lwsm restart eventlog";
        $info->{lw}->{daemons}->{eventfwdd} = "eventfwdd";
        $info->{lw}->{restart}->{eventfwdd} = "lwsm restart eventfwd";
        $info->{lw}->{daemons}->{syslogreaper} = "reapsysld";
        $info->{lw}->{restart}->{syslogreaper} = "lwsm restart reapsysl";
        $info->{lw}->{daemons}->{registry} = "lwregd";
        $info->{lw}->{restart}->{registry} = "lwsm restart lwreg";
        $info->{lw}->{daemons}->{smartcard} = "lwscd";
        $info->{lw}->{restart}->{smartcard} = "lwsm restart lwscd";
        $info->{lw}->{daemons}->{pkcs11} = "lwpkcs11d";
        $info->{lw}->{restart}->{pkcs11} = "lwsm restart lwpkcs11";
        $info->{lw}->{daemons}->{startcmd} = "--start-as-daemon";
        $info->{lw}->{logging}->{smbdaemon} = "lwio-set-log-level";
        $info->{lw}->{logging}->{authdaemon} = "lw-set-log-level";
        $info->{lw}->{logging}->{netdaemon} = "lwnet-set-log-level";
        $info->{lw}->{logging}->{eventfwdd} = "evtfwd-set-log-level";
        $info->{lw}->{logging}->{syslogreaper} = "rsys-set-log-level";
        $info->{lwsm}->{control}="init";
        $info->{lwsm}->{type}="standalone";
        $info->{lw}->{tools}->{findsid} = "lw-find-by-sid";
        $info->{lw}->{tools}->{userlist} = "lw-enum-users --level 2";
        $info->{lw}->{tools}->{grouplist} = "lw-enum-groups --level 1";
        $info->{lw}->{tools}->{userbyname} = "lw-find-user-by-name --level 2";
        $info->{lw}->{tools}->{userbyid} = "lw-find-user-by-id --level 2";
        $info->{lw}->{tools}->{groupbyname} = "lw-find-group-by-name --level 1";
        $info->{lw}->{tools}->{groupbyid} = "lw-find-group-by-id --level 1";
        $info->{lw}->{tools}->{groupsforuser} = "lw-list-groups-for-user --show-sid";
        $info->{lw}->{tools}->{groupsforuid} = "lw-list-groups-for-user --show-sid --uid";
        $info->{lw}->{tools}->{dctime} = "lw-get-dc-time";
        $info->{lw}->{tools}->{config} = "cat /etc/likewise/lsassd.conf";
        $info->{lw}->{tools}->{status} = "lw-get-status";
        logData("Likewise Version 5.3 installed");
    } elsif ($info->{lw}->{version} == "5.2.0") {
        $info->{lw}->{base} = "/opt/likewise";
        $info->{lw}->{path} = "/opt/likewise/bin";
        $info->{lw}->{daemons}->{smbdaemon} = "lwiod";
        $info->{lw}->{daemons}->{authdaemon} = "lsassd";
        $info->{lw}->{daemons}->{gpdaemon} = "gpagentd" if (defined($gporefresh->{path}));
        $info->{lw}->{daemons}->{dcedaemon} = "dcerpcd";
        $info->{lw}->{daemons}->{netdaemon} = "netlogond";
        $info->{lw}->{daemons}->{eventlogd} = "eventlogd";
        $info->{lw}->{daemons}->{startcmd} = "--start-as-daemon";
        $info->{lw}->{logging}->{smbdaemon} = "lwio-set-log-level";
        $info->{lw}->{logging}->{authdaemon} = "lw-set-log-level";
        $info->{lw}->{logging}->{eventfwdd} = "evtfwd-set-log-level";
        $info->{lwsm}->{control}="init";
        $info->{lwsm}->{type}="standalone";
        $info->{lw}->{tools}->{findsid} = "lw-find-by-sid";
        $info->{lw}->{tools}->{userlist} = "lw-enum-users --level 2";
        $info->{lw}->{tools}->{grouplist} = "lw-enum-groups --level 1";
        $info->{lw}->{tools}->{userbyname} = "lw-find-user-by-name --level 2";
        $info->{lw}->{tools}->{userbyid} = "lw-find-user-by-id --level 2";
        $info->{lw}->{tools}->{groupbyname} = "lw-find-group-by-name --level 1";
        $info->{lw}->{tools}->{groupbyid} = "lw-find-group-by-id --level 1";
        $info->{lw}->{tools}->{groupsforuser} = "lw-list-groups";
        $info->{lw}->{tools}->{dctime} = "lw-get-dc-time";
        $info->{lw}->{tools}->{config} = "cat /etc/likewise/lsassd.conf";
        $info->{lw}->{tools}->{status} = "lw-get-status";
        logData("Likewise Version 5.2 installed");
    } elsif ($info->{lw}->{version} == "5.1.0") {
        $info->{lw}->{base} = "/opt/likewise";
        $info->{lw}->{path} = "/opt/likewise/bin";
        $info->{lw}->{daemons}->{smbdaemon} = "lwrdrd";
        $info->{lw}->{daemons}->{authdaemon} = "lsassd";
        $info->{lw}->{daemons}->{gpdaemon} = "gpagentd" if (defined($gporefresh->{path}));
        $info->{lw}->{daemons}->{dcedaemon} = "dcerpcd";
        $info->{lw}->{daemons}->{netdaemon} = "netlogond";
        $info->{lw}->{daemons}->{eventlogd} = "eventlogd";
        $info->{lw}->{daemons}->{startcmd} = "2>&1 &";
        $info->{lw}->{logging}->{smbdaemon} = "lw-smb-set-log-level";
        $info->{lw}->{logging}->{authdaemon} = "lw-set-log-level";
        $info->{lwsm}->{control}="init";
        $info->{lwsm}->{type}="standalone";
        $info->{lw}->{tools}->{userlist} = "lw-enum-users --level 2";
        $info->{lw}->{tools}->{grouplist} = "lw-enum-groups --level 1";
        $info->{lw}->{tools}->{userbyname} = "lw-find-user-by-name --level 2";
        $info->{lw}->{tools}->{userbyid} = "lw-find-user-by-id --level 2";
        $info->{lw}->{tools}->{groupbyname} = "lw-find-group-by-name --level 1";
        $info->{lw}->{tools}->{groupbyid} = "lw-find-group-by-id --level 1";
        $info->{lw}->{tools}->{groupsforuser} = "lw-list-groups";
        $info->{lw}->{tools}->{dctime} = "lw-get-dc-time";
        $info->{lw}->{tools}->{config} = "cat /etc/likewise/lsassd.conf";
        $info->{lw}->{tools}->{status} = "lw-get-status";
        logData("Likewise Version 5.1 installed");
    } elsif ($info->{lw}->{version} == "5.0.0") {
        $info->{lw}->{base} = "/opt/likewise";
        $info->{lw}->{path} = "/opt/likewise/bin";
        $info->{lw}->{daemons}->{smbdaemon} = "npcmuxd";
        $info->{lw}->{daemons}->{authdaemon} = "lsassd";
        $info->{lw}->{daemons}->{gpdaemon} = "gpagentd" if (defined($gporefresh->{path}));
        $info->{lw}->{daemons}->{dcedaemon} = "dcerpcd";
        $info->{lw}->{daemons}->{netdaemon} = "netlogond";
        $info->{lw}->{daemons}->{eventlogd} = "eventlogd";
        $info->{lw}->{daemons}->{startcmd} = "2>&1 &";
        $info->{lw}->{logging}->{authdaemon} = "lw-set-log-level";
        $info->{lwsm}->{control}="init";
        $info->{lwsm}->{type}="standalone";
        $info->{lw}->{tools}->{userlist} = "lw-enum-users --level 2";
        $info->{lw}->{tools}->{grouplist} = "lw-enum-groups --level 1";
        $info->{lw}->{tools}->{userbyname} = "lw-find-user-by-name --level 2";
        $info->{lw}->{tools}->{userbyid} = "lw-find-user-by-id --level 2";
        $info->{lw}->{tools}->{groupbyname} = "lw-find-group-by-name --level 1";
        $info->{lw}->{tools}->{groupbyid} = "lw-find-group-by-id --level 1";
        $info->{lw}->{tools}->{groupsforuser} = "lw-list-groups";
        $info->{lw}->{tools}->{dctime} = "lw-get-dc-time";
        $info->{lw}->{tools}->{config} = "cat /etc/likewise/lsassd.conf";
        $info->{lw}->{tools}->{status} = "lw-get-status";
        logData("Likewise Version 5.0 installed");
    } elsif ($info->{lw}->{version} == "4.1") {
        if ($info->{OStype}=~/linux/) {
            $info->{lw}->{base} = "/usr/centeris";
            $info->{lw}->{path} = "/usr/centeris/bin";
        } else {
            $info->{lw}->{base} = "/opt/centeris";
            $info->{lw}->{path} = "/opt/centeris/bin";
        }
        $info->{lw}->{daemons}->{smbdaemon} = "centeris.com-npcmuxd";
        $info->{lw}->{daemons}->{authdaemon} = "winbindd";
        $info->{lw}->{daemons}->{gpdaemon} = "centeris.com-gpagentd" if (defined($gporefresh->{path}));
        $info->{lw}->{daemons}->{dcedaemon} = "centeris.com-dcerpcd";
        $info->{lw}->{daemons}->{startcmd} = "2>&1 &";
        $info->{lwsm}->{control}="init";
        $info->{lwsm}->{type}="standalone";
        $info->{lw}->{tools}->{userlist} = "lwiinfo -U";
        $info->{lw}->{tools}->{grouplist} = "lwiinfo -G";
        $info->{lw}->{tools}->{userbyname} = "lwiinfo -i";
        $info->{lw}->{tools}->{userbyid} = "lwiinfo --uid-info";
        $info->{lw}->{tools}->{groupbyname} = "lwiinfo -g";
        $info->{lw}->{tools}->{groupbyid} = "lwiinfo --gid-info";
        $info->{lw}->{tools}->{status} = "lwiinfo -pmt";
        $info->{lw}->{tools}->{config} = "cat /etc/centeris/lwiauthd.conf";
        logData("Likewise Version 4.1 installed");
    }
    readFile($info, $versionFile->{path});
    if ($versionFile->{path}=~/ENTERPRISE/) {
        logDebug("PBIS Enterprise installed, printing Platform information");
        my $platformFile=$versionFile->{path};
        $platformFile=~s/ENTERPRISE_//g;
        readFile($info, $platformFile);
        $info->{emailaddress} = 'pbis-support@beyondtrust.com';
    }

    if (not defined($gporefresh->{path})) {
# PBIS / Likewise Open doesn't include gporefresh or the following daemons, so mark them undef,
# This way, we won't attempt to restart them later, or do anything with them.
# Reduces errors printed to screen.
        undef $info->{lw}->{daemons}->{gpdaemon};
        undef $info->{lw}->{daemons}->{eventfwd};
        undef $info->{lw}->{daemons}->{syslogreaper};
        undef $info->{lw}->{daemons}->{usermonitor};
        $info->{emailaddress} = 'pbis-support@beyondtrust.com';
    }
}

sub outputReport($$) {
    my $info=shift || confess "no info hash passed to reporting!\n";
    my $opt=shift || confess "no options hash passed to reporting!\n";

    sectionBreak("Gathering Logfiles");
    my ($tarballfile, $error, $appendfile);
    if (-d $opt->{tarballdir} && (-w $opt->{tarballdir})) { 
        $tarballfile = $opt->{tarballdir}."/".$opt->{tarballfile};
    } elsif (-w "./") {
        $tarballfile = "./".$opt->{tarballfile};
    } else {
        $gRetval |= ERR_FILE_ACCESS;
        logError("Can't write log tarball $opt->{tarballfile}$opt->{tarballext}!");
        logError("both $opt->{tarballdir} and ./ are non-writable!");
        return $gRetval;
    }
    if (-e $tarballfile.$opt->{tarballext}) {
        logWarning("WARNING: file $tarballfile".$opt->{tarballext}." exists, adding ext...");
        for ($error=0; $error<99; $error++) {
            my $num = sprintf("%02d", $error);
            unless (-e $tarballfile."-$num".$opt->{tarballext}) {
                $tarballfile = $tarballfile."-$num".$opt->{tarballext};
                last;
            }
        }
    } else {
        $tarballfile = $tarballfile.$opt->{tarballext};
    }
    if ($tarballfile=~/\.gz$/) {
#now that we know that the gz file is safe to create,
#strip the .gz extension, so it can be gzipped later
        logDebug("Creating tarball as tar only, will gzip at end.");
        $tarballfile=~s/\.gz$//;
    }
    logVerbose("Creating tarball $tarballfile and adding logs");
    if ($opt->{restart} or $info->{lwsm}->{type} eq "container") {
        if ($opt->{lsassd}) {
            logInfo("Adding auth daemon log");
            $appendfile = $info->{logpath}."/".$info->{lw}->{daemons}->{authdaemon}.".log";
            tarFiles($info, $opt, $tarballfile, $appendfile);
        }
        if ($opt->{lwiod}) {
            logInfo("Adding SMB Daemon log");
            $appendfile = $info->{logpath}."/".$info->{lw}->{daemons}->{smbdaemon}.".log";
            tarFiles($info, $opt, $tarballfile, $appendfile);
        }
        if ($opt->{netlogond}) {
            logInfo("adding netlogond log");
            $appendfile = $info->{logpath}."/".$info->{lw}->{daemons}->{netdaemon}.".log";
            tarFiles($info, $opt, $tarballfile, $appendfile);
        }
        if ($opt->{gpagentd}) {
            logInfo("Adding gpagentd log");
            $appendfile = $info->{logpath}."/".$info->{lw}->{daemons}->{gpdaemon}.".log";
            tarFiles($info, $opt, $tarballfile, $appendfile);
        }
        if ($opt->{eventlogd}) {
            logInfo("Adding eventlogd log");
            $appendfile = $info->{logpath}."/".$info->{lw}->{daemons}->{eventlogd}.".log";
            tarFiles($info, $opt, $tarballfile, $appendfile);
        }
        if ($opt->{eventfwdd}) {
            logInfo("Adding eventfwdd log");
            $appendfile = $info->{logpath}."/".$info->{lw}->{daemons}->{eventfwdd}.".log";
            tarFiles($info, $opt, $tarballfile, $appendfile);
        }
        if ($opt->{lwregd}) {
            logInfo("Adding registry log");
            $appendfile = $info->{logpath}."/".$info->{lw}->{daemons}->{registry}.".log";
            tarFiles($info, $opt, $tarballfile, $appendfile);
        }
        if ($opt->{lwsmd}) {
            logInfo("Adding service controller log");
            $appendfile = $info->{logpath}."/".$info->{lw}->{daemons}->{lwsm}.".log";
            tarFiles($info, $opt, $tarballfile, $appendfile);
        }
        if ($opt->{reapsysld}) {
            logInfo("Adding syslog reaper log");
            $appendfile = $info->{logpath}."/".$info->{lw}->{daemons}->{syslogreaper}.".log";
            tarFiles($info, $opt, $tarballfile, $appendfile);
        }
    }
    tarFiles($info, $opt, $tarballfile, "/Library/Logs/DirectoryService/DirectoryService.debug.log") if ($info->{OStype} eq "darwin");
    if ($opt->{messages}) {
        logInfo("Adding $info->{logpath}/$info->{logfile}");
        $appendfile = $info->{logpath}."/".$info->{logfile};
        tarFiles($info, $opt, $tarballfile, $appendfile);
    }
    if ($opt->{automounts}) {
        logInfo("Adding autofs files from PBIS GPO");
        tarFiles($info, $opt, $tarballfile, "/etc/lwi_automount/*");
        logInfo("Now adding /etc/auto* as well");
        tarFiles($info, $opt, $tarballfile, "/etc/auto*");
    }
    if ($opt->{sambalogs}) {
        logInfo("Adding files from $info->{logpath}/samba");
        tarFiles($info, $opt, $tarballfile, "$info->{logpath}/samba/*");
        if ($info->{logpath} ne "/var/log") {
            logDebug("Adding files from /var/log/samba also");
            tarFiles($info, $opt, $tarballfile, "/var/log/samba/*");
        }
        logInfo("Adding samba configuration files");
        tarFiles($info, $opt, $tarballfile, "$info->{sambaconf}->{dir}/*"); 
    }
    if ($opt->{ssh}) {
        logInfo("Adding sshd_config");
        tarFiles($info, $opt, $tarballfile, $info->{sshd_config}->{path}) if ($info->{sshd_config}->{path});
        logError("Can't find sshd_config to add to tarball!") unless ($info->{sshd_config}->{path});
        tarFiles($info, $opt, $tarballfile, $info->{logpath}."/sshd-pbis.log");
    }
    logInfo("Adding pam files");
    tarFiles($info, $opt, $tarballfile, $info->{pampath});
    logInfo("Adding krb5.conf");
    tarFiles($info, $opt, $tarballfile, $info->{krb5conf}->{path}) if ($info->{krb5conf}->{path});
    logError("Can't find krb5.conf to add to tarball!") unless ($info->{krb5conf}->{path});
    if ($opt->{sudo}) {
        logInfo("Adding sudoers");
        tarFiles($info, $opt, $tarballfile, $info->{sudoers}->{path}) if ($info->{sudoers}->{path});
        logError("Can't find sudoers to add to tarball!") unless ($info->{sudoers}->{path});
    }
    if ($opt->{tcpdump}) {
        logInfo("Adding tcpdump capture...");
        tarFiles($info, $opt, $tarballfile, $opt->{capturefile});
    }
# Files to add under all circumstances
    logInfo("Adding PBIS Configuration...");
    tarFiles($info, $opt, $tarballfile, "/etc/likewise");
    tarFiles($info, $opt, $tarballfile, "/etc/pbis");
    if (defined($opt->{gatherdb}) and $opt->{gatherdb} = 1) {
        logInfo("Adding Likewise DB folder (this may take a while if the eventlog is large)...");
        tarFiles($info, $opt, $tarballfile, "/var/lib/likewise/db");
        tarFiles($info, $opt, $tarballfile, "/var/lib/pbis/db");
    }
    if (defined($opt->{gpo}) and $opt->{gpo} = 1) {
        logInfo("Adding PBIS Group Policy cache...");
        tarFiles($info, $opt, $tarballfile, "/var/lib/likewise/grouppolicy");
        tarFiles($info, $opt, $tarballfile, "/var/lib/pbis/grouppolicy");
    }
    tarFiles($info, $opt, $tarballfile, $info->{nsfile});
    tarFiles($info, $opt, $tarballfile, $info->{timezonefile});
    if (defined($info->{hostsfile}->{path})) {
        logInfo("Adding $info->{hostsfile}->{path}...");
        tarFiles($info, $opt, $tarballfile, $info->{hostsfile}->{path});
    }
    if (defined($info->{resolvconf}->{path})) {
        logInfo("Adding $info->{resolvconf}->{path}...");
        tarFiles($info, $opt, $tarballfile, $info->{resolvconf}->{path});
    }
    logInfo("Finished adding files, now for our output");
    if ($opt->{logfile} eq "-") {
        logError("Can't add STDOUT to tarball - you need to run with an actual logfile!");
    } else {
        tarFiles($info, $opt, $tarballfile, $opt->{logfile});
    }
    if ($opt->{tarballext}=~/\.gz$/) {
        logInfo("Output gathered, now gzipping for email");
        $error = System("gzip $tarballfile");
        if ($error) {
            $gRetval |= ERR_SYSTEM_CALL;
            logError("Can't gzip $tarballfile - $!");
        } else {
            $tarballfile = "$tarballfile.gz";
        }
    }
    logData("All data gathered successfully in $tarballfile.");
    logData("Please email $tarballfile to $info->{emailaddress} to help diagnose your problem");
}

sub runTests($$) {
    my $info = shift || confess "no info hash passed to test runner!\n";
    my $opt = shift || confess "no options hash passed to test runner!\n";
    my $data;
# Run tests that run every time no matter what

    sectionBreak("lw-get-status");
    runTool($info, $opt, "$info->{lw}->{tools}->{status}", "print");

    sectionBreak("configuration");
    runTool($info, $opt, "$info->{lw}->{tools}->{config}", "print");

    sectionBreak("Running User");
    getUserInfo($info, $opt, $info->{logon});

    sectionBreak("DC Times");
    my $status = runTool($info, $opt, "$info->{lw}->{tools}->{status}", "grep", "Domain:");
    chomp $status;
    my @domains;
    while ($status =~ /DNS Domain:\s+(.*)/g) {
        my $domain=$1;
        sectionBreak("Current DC Time for $domain.");
        $domain=~/[^\s]+$/;
        runTool($info, $opt, "$info->{lw}->{tools}->{dctime} $domain", "print");
        push(@domains, $domain);
    }
# run optional tests

    if ($opt->{dns}) {
        sectionBreak("DNS Tests");
        my $site = runTool($info, $opt, "$info->{lw}->{tools}->{status}", "grep", 'Site:\s+(.*)');
        my @status = split(/\s+/, $site);
        my %dclist;
        foreach my $site (@status) {
            logInfo("We are in site: $site.");
            foreach my $domain (@domains) {
                sectionBreak("DNS Info for $site in $domain.");
                foreach my $search (("_ldap._tcp", "_gc._tcp", "_kerberos._tcp", "_kerberos._udp")) {
                    logData("Results for $search.$domain:");
                    my @records = dnsSrvLookup("$search.$domain");
                    foreach (@records) {
                        logData("  $_");
                        $dclist{$_}=1;
                    }
                    logData("Results for $search.$site._sites.$domain:");
                    @records = dnsSrvLookup("$search.$site._sites.$domain");
                    foreach (@records) {
                        logData("  $_");
                        $dclist{$_}=1;
                    }                
                }
            }
        }
        foreach my $dc (sort(keys(%dclist))) {
            foreach (dnsLookup($dc, "A")) {
                chomp;
                logData($_);
            }
        }
    }
    if ($opt->{users}) {
        sectionBreak("Enum Users");
        runTool($info, $opt, "$info->{lw}->{tools}->{userlist}", "print");
    }
    if ($opt->{groups}) {
        sectionBreak("Enum Groups");
        runTool($info, $opt, "$info->{lw}->{tools}->{grouplist}", "print");
    }

    if ($opt->{ssh}) {
        sectionBreak("SSH Test");
        my $user;
        if ($opt->{sshuser}) {
            $user = $opt->{sshuser};
        } else {
            logData("Testing SSH as an AD user - please enter a username here:");
            $user = <STDIN>;
            chomp $user;
            $opt->{sshuser} = $user;
        }
        $user=safeUsername($user);
        logVerbose("Looking up $user prior to test");
        getUserInfo($info, $opt, $user);
        my $sshcommand = "ssh -vvv -p 22226 -l $user localhost '$opt->{sshcommand}' 2>&1";
        my $sshdcommand = $info->{sshd}->{cmd}." -ddd -p 22226 > $info->{logpath}/sshd-pbis.log 2>&1";
        #$sshdcommand.=$info->{sshd}->{opts}.' 2>&1';

        logData("Running sshd as $sshdcommand");
        my $data1 = System($sshdcommand." & ",1 ,30);
        if ($?) {
            $gRetval |= ERR_SYSTEM_CALL;
            logError("Error launching sshd!");
        } else {
            sleep 1;
            logData("Running ssh as: $sshcommand");
            $data = `$sshcommand`;
            if ($?) {
                $gRetval |= ERR_SYSTEM_CALL;
                logError("Error running ssh as $user!");
            }
        }
        
        logData($data);
    } elsif ($opt->{sshuser} || $info->{uid} ne "0") {
        my $user = $opt->{sshuser};
        $user = $info->{logon} if (not defined($opt->{sshuser}));
        sectionBreak("User Lookup");
        getUserInfo($info, $opt, $user);
    }

    if ($opt->{gpo}) {
        sectionBreak("GPO Tests");
        $opt->{dns} = 1; #do dns tests as well if we're testing GPO, they often are related
        runTool($info, $opt, "gporefresh", "print");
    }

    if ($opt->{smb}) {
        sectionBreak("SMB Tests");
#TODO Write
    }

    if ($opt->{sudo}) {
        sectionBreak("Sudo Test");
        logWarning("Opening a bash shell in this window.");
        logWarning("Perform the sudoers tests required, then type 'exit'");
#TODO Fix this sudoers test not working
        logError("Some output may not print to screen - this is OK");
        my $file = findInPath("bash", ["/bin", "/usr/bin", "/usr/local/bin"]);
        $data = `$file->{path}`;
        logData($data);
    }

    if ($opt->{othertests}) {
        sectionBreak("Other Tests");
        logWarning("Please run any manual tests required now (interactive logon, sudo, su, etc.)");
        logWarning("Type 'done' and hit Enter when complete...");
        my $complete = "";
        while (not ($complete=~/done/i)) {
            $complete=readline(*STDIN);
            chomp $complete;
        }
    }

    if ($opt->{delay}) {
        sectionBreak("Delay for testing");
        logData("Please run any manual tests required now (interactive logon, sudo, su, etc.)");
        logData("This program will continue in $opt->{delaytime} seconds...");
        while ($opt->{delaytime} > 30) {
            $opt->{delaytime} = $opt->{delaytime}-30;
            sleep 30;
            logData("This program will continue in $opt->{delaytime} seconds...");
        }
        sleep $opt->{delaytime};
    }

    if ($opt->{authtest}) {
        sectionBreak("PAC info");
        my ($logfile, $error);
        $data = "";
        $logfile = $opt->{paclog};
        #$logfile = "$info->{logpath}/$info->{logfile}" if ($opt->{syslog});
        #$logfile = "$info->{logpath}/lsassd.log" if ($opt->{restart} and $opt->{lsassd});
        logWarning("No logfile to read!") unless ($logfile);
        my %sids;
        open(LF, "<$logfile") || logError("Can't open $logfile - $!");
        while(<LF>) {
            if (/PAC (\w+ )?membership is (S-1-5[^\s]+)/) {
                logDebug("Found SID $2");
                $sids{$2} = 1;
            }
        }
        foreach my $sid (sort(keys(%sids))) {
            $data=runTool($info, $opt, "$info->{lw}->{tools}->{findsid} $sid", "return");
            if ($data) {
                logData($data);
                $error = 1;
                $data = "";
            } else {
                logError("Error getting user by SID $sid!");
            }
        }
        close LF;
        logWarning("Couldn't find any PAC information to review") unless ($error);
    }
    if ($opt->{psoutput}) {
        sectionBreak("ps output");
        $data = `$info->{pscmd}`;
        logData($data);
    }
}

# Main Functions End
#####################################

###########################################
# controller subroutine starts here
sub main() {

    Getopt::Long::Configure('no_ignore_case', 'no_auto_abbrev') || confess;

    my @time = localtime();
    $time[5]+=1900;
    my $datestring = $time[5].sprintf("%02d", $time[4]+1).sprintf("%02d", $time[3]).sprintf("%02d", $time[2]).sprintf("%02d", $time[1]);
    my $info = {};
    $info->{emailaddress} = 'openproject@beyondtrust.com';

    my $opt = { netlogond => 1,
        users => 1,
        groups => 1,
        lsassd => 1,
        lwiod => 1,
        netlogond => 1,
        gpagentd => 1,
        messages => 1,
        syslog => 1,
        capturefile => "/tmp/pbis-cap",
        loglevel => "info",
        logfile => "/tmp/pbis-support.log",
        tarballdir => "/tmp",
        tarballfile => "pbis-support-$datestring",
        tarballext => ".tar.gz",
        sshcommand => "exit",
        sudocmd => "ls -l /var/lib/pbis/db",
        delaytime => 180,
        gatherdb => 1,
        psoutput => 1,
    };

    my $ok = GetOptions($opt,
        'help|h|?',
        'logfile|log|l=s',
        'loglevel|V=s',
        'verbose|v+',
        'tarballdir|t=s',
        'ssh!',
        'gpo|grouppolicy!',
        'gatherdb|db!',
        'dns!',
        'users|u!',
        'groups|g!',
        'sudo!',
        'automounts|autofs!',
        'tcpdump|capture|snoop|iptrace|nettl|c!',
        'capturefile=s',
        'othertests|other|o!',
        'lsassd|winbindd!',
        'lwiod|lwrdrd|npcmuxd!',
        'netlogond!',
        'gpagentd!',
        'dcerpcd!',
        'eventlogd!',
        'eventfwdd!',
        'lwregd|regdaemon!',
        'lwsmd|svcctl|lwsm|svcctld!',
        'reapsysld|syslogreaper|reaper!',
        'lwscd|smartcard|lwsc',
        'lwpcks11d|lwpcks11',
        'messages!',
        'sambalogs!',
        'restart|r!',
        'syslog!',
        'sshcommand=s',
        'sshuser=s',
        'authtest!',
        'sudopasswd=s',
        'sudocmd=s@',
        'psoutput|ps!',
        'delay!',
        'delaytime|dt=s',
    );
    my $more = shift @ARGV;
    my $errors;

    if (not defined $opt->{restart}) {
        $opt->{restart} = 1 if not $opt->{syslog};
        $opt->{restart} = 0 if $opt->{syslog};
    } else {
        $opt->{syslog} = 0 if $opt->{restart};
        $opt->{syslog} = 1 if not $opt->{restart};
    }

    if ($opt->{help} or not $ok) {
        $gRetval |= ERR_OPTIONS;
        print usage($opt, $info);
    }

    if ($opt->{sudo} or $opt->{ssh} or $opt->{other}) {
        $opt->{authtest} = 1;
    }

    my @requireOptions = qw(logfile );
    foreach my $optName (@requireOptions) {
        if (not $opt->{$optName}) {
            $errors .= "Missing required --".$optName." option.\n";
        }
    }
    if ($more) {
        $errors .= "Too many arguments.\n";
    }
    if ($errors) {
        $gRetval |= ERR_OPTIONS;
        print $errors.usage($opt, $info);
    }

    exit $gRetval if $gRetval;

    if (defined($opt->{logfile}) && $opt->{logfile} ne "-") {
        open(OUTPUT, ">$opt->{logfile}") || die "can't open logfile $opt->{logfile}\n";
        $gOutput = \*OUTPUT;
        logInfo("Initializing logfile $opt->{logfile}.");
    } else { 
        $gOutput = \*STDOUT;
        logInfo("Logging to STDOUT.");
        logError("Will not be able to capture the output log! You should cancel and restart with a different logfile.");
        sleep 5;
    }

    if (defined($opt->{gpagentd} and $opt->{gpagentd} == 1)) {
        $opt->{gpo} = 1;  #turn on GPO testing since we're doing gpagentd logging.
    }

    if (defined($opt->{verbose})) {
        $gDebug = $opt->{verbose};
        logData("Logging at level $gDebug");
    }

    if ($gDebug<1 or $gDebug > 5) {
        $gDebug = 1 if ($gDebug < 1);
        $gDebug = 5 if ($gDebug > 5);
        logWarning("Log Level not specified.");
    }

    if (defined($opt->{loglevel}) && not defined($opt->{verbose})) {
        $gDebug = 5 if ($opt->{loglevel}=~/^debug$/i);
        $gDebug = 4 if ($opt->{loglevel}=~/^verbose$/i);
        $gDebug = 3 if ($opt->{loglevel}=~/^info$/i);
        $gDebug = 2 if ($opt->{loglevel}=~/^warning$/i);
        $gDebug = 1 if ($opt->{loglevel}=~/^error$/i);
        $gDebug = $opt->{loglevel} if ($opt->{loglevel}=~/^\d+$/);
        logWarning("Logging at $opt->{loglevel} level.");
    }

    $opt->{tarballdir}=~s/\/$//;

    unless (-d $opt->{tarballdir}) {
        $gRetval |= ERR_OPTIONS;
        $gRetval |= ERR_FILE_ACCESS;
        logError("$opt->{tarballdir} is not a directory!");
    }

    exit $gRetval if $gRetval;


    sectionBreak("OS Information");
    logDebug("Determining OS info");
    determineOS($info);
    
    sectionBreak("PBIS Version");
    logDebug("Determining PBIS version");
    getLikewiseVersion($info);

    sectionBreak("Options Passed");
    logData(fileparse($0)." version $gVer");
    foreach my $el (keys(%{$opt})) {
        logData("$el = ".&getOnOff($opt->{$el}));
    }

    if (defined $opt->{tcpdump} && $opt->{tcpdump}) {
        sectionBreak("Starting tcpdump");
        tcpdumpStart($info, $opt);
    }

    sectionBreak("Daemon restarts");
    logDebug("Turning up logging levels");
    changeLogging($info, $opt, "debug");
    logWarning("Sleeping for 60 seconds to let Domains be found");
    waitForDomain($info, $opt);

    runTests($info, $opt);

    sectionBreak("Daemon restarts");
    logDebug("Turning logging levels back to normal");
    changeLogging($info, $opt, "normal");
    
    if (defined $opt->{tcpdump} && $opt->{tcpdump}) {
        sectionBreak("Stopping tcpdump");
        tcpdumpStop($info, $opt);
    }

    outputReport($info, $opt);
}

=head1 (C) BeyondTrust Software

=head1 Description

usage: pbis-support.pl [tests] [log choices] [options]

  This is the BeyondTrust Software PBIS (Open/Enterprise) support tool.
  It creates a log as specified by the options, and creates 
  a gzipped tarball in for emailing to the PBIS support team.

  The options are broken into three (3) groups: Tests, Logs,
  and Options.  Any "on/off" flag has a "--no" option available,
  for example: "--nossh" and "--ssh". The "no" obviously
  negates the flag's operation

  Examples:

    pbis-support.pl --ssh --lsassd --nomessages --restart -l pbis.log


=head2 Usage


    Tests to be performed:

    --(no)ssh (default = off)
    Test ssh logon interactively and gather logs
    --sshcommand <command> (default = 'exit')
    --sshuser <name> (instead of interactive prompt)
    --(no)gpo --grouppolicy (default = off)
    Perform Group Policy tests and capture Group Policy cache
    -u --(no)users (default = on)
    Enumerate all users
    -g --(no)groups (default = on)
    Enumerate all groups
    --autofs --(no)automounts (default = off)
    Capture /etc/lwi_automount in tarball
    --(no)dns (default = off)
    DNS lookup tests
    -c --(no)tcpdump (--capture) (default = off)
    Capture network traffic using OS default tool
    (tcpdump, nettl, snoop, etc.)
    --capturefile <file> (default = /tmp/lw-cap)
    --(no)smb (default = off)
    run smbclient against local samba server
    -o --(no)othertests (--other) (default = off)
    Pause to allow other tests (interactive logon,
    multiple ssh tests, etc.) to be run and logged.
    --(no)delay (default = off)
    Pause the script for 90 seconds to gather logging
    data, for example from GUI logons.
    -dt --delaytime <seconds> (default = 90)

    Log choices: 

    --(no)lsassd (--winbindd) (default = on)
    Gather lsassd debug logs
    --(no)lwiod (--lwrdrd | --npcmuxd) (default = on)
    Gather lwrdrd debug logs
    --(no)netlogond (default = on)
    Gather netlogond debug logs
    --(no)gpagentd (default = on)
    Gather gpagentd debug logs
    --(no)eventlogd (default = off)
    Gather eventlogd debug logs
    --(no)eventfwdd (default = off)
    Gather eventfwdd debug logs
    --(no)reapsysld (default = off)
    Gather reapsysld debug logs
    --(no)regdaemon (default = off)
    Gather regdaemon debug logs
    --(no)lwsm (default = off)
    Gather lwsm debug logs
    --(no)smartcard (default = off)
    Gather smartcard daemon debug logs
    --(no)messages (default = on)
    Gather syslog logs
    --(no)gatherdb (default = off)
    Gather PBIS Databases
    --(no)sambalogs (default = off)
    Gather logs and config for Samba server
    -ps --(no)psoutput (default = off)
    Gather's full process list from this system

    Options: 

    -r --(no)restart (default = on)
    Allow restart of the PBIS daemons to separate logs
    --(no)syslog (default = off)
    Allow editing syslog.conf during the debug run if not
    restarting daemons (exclusive of -r)
    -V --loglevel {error,warning,info,verbose,debug}
    Changes the logging level. (default = info )
    -l --log --logfile <path> (default = /tmp/pbis-support.log )
    Choose the logfile to write data to.
    -t --tarballdir <path> (default = /tmp )
    Choose where to create the gzipped tarball of log data

    Examples:

    pbis-support.pl --ssh --lsassd --nomessages --restart -l pbis.log





=head1 Programmer's data

=head2 Data Structures:

$gRetval is a bitstring used to hold error values.  The small subs at the top ERR_XXX_XXX = { 2;} 
are combined with the existing value via bitwise OR operations. This gives us a cumulative count
of the errors that have happened during the run of the program, and a good exit status if non-0

$gDebug is used to determine which level to log at.  higher is more verbose, with 5 being the
current max (debug).

=head3 Defined subroutines are:

usage($opt, $info) - outputs help status with intelligent on/off values based on defaults and flags passed.

changeLogging($info, $opt, $state) - changes logging for all daemons in $opt to $state

changeLoggingWithLwSm($info, $opt, $state) - called by changeLogging() if LW 6 or greater, to use LWSM for state changes

daemonRestart($info, $options) - restarts daemon in $options to state in $options

determineOS($info) - updates the $info hash with OS specific paths and commands

dnsSrvLookup($lookup) - looks up $lookup via DNS by best means available on system

findInPath($file, $path) searches array REF $path for $file - more detail below

findProcess($process, $info) returns hash structure of a process' information from PS

GetErrorCodeFromChildError($error) - gets error status from child spawned by System();

getLikewiseVersion($info) - determines LW version, sets variables specific to that version, daemon name, tool command, etc.

getOnOff($test) - returns "on" or "off" to boolean $test, returns $test for string/numeric values (non-0/1)

getUserInfo($info, $opt, $name) - gets NSS and PBIS information about $name

killProc($process, $signal, $info) - send any signal to any process by name or PID

killProc2($signal, $process) - sends the actual kill signal to the PID, no name allowed

lineDelete($file, $line) removes $line from $file by commenting out, only if it was added by lineInsert

lineInsert($file, $line) inserts $line to end of $file if not already there.

logData($line) - logs $line regardless of level

logError($line) - logs $line at error (1) level

logWarning($line) - logs $line at warning (2) level or lower

logInfo($line) - logs $line at info (3) level or lower

logVerbose($line) - logs $line at verbose (4) level or lower

logDebug($line) - logs $line at debug (5) level or lower

outputReport($info, $opt) - determines the pieces to gather based on flags

readFile($info, $filename) reads $filename, print to screen and log, parse if neccessary

runTests($info, $opt) - runs the actual tests based on flags

runTool($info, $opt, $tool) - runs $tool from /opt/pbis/bin directory, capturing output and error state, printing to log/screen

safeRegex($regex) - escape handling for safe regular expression matching in user-input strings.

safeUsername($name) - escape handling for safe username processing in System() calls.

sectionBreak($title) - prints $title as a new section to the log and screen

System($command; $print, $timeout) a system() analogue including child success code handling

tarFiles($info, $opt, $tarball, $file) adds $file to $tarball - creating new $tarball if neccesary

tcpdumpStart($info, $top) - starts tcpdump analogue for the OS

tcpdumpStop($info, $opt) - stops tcpdump analogue for the OS

waitForDomain($info, $opt) - when restarting daemons, waits for domain enumeration to complete, so that reported data is accurate

=head3 Hash Structures

$opt contains each of the command-line options passed.
$info contains information about the system the tool is running on.

=head4 hash REF opt

$opt is a hash reference, with keys as below, grouped for ease of reading (no groups in the hash structure, each key exists at top level):
    (Group: Daemons and logs)
        netlogond
        lsassd
        lwiod
        gpagentd
        eventfwdd
        reapsysld
        lwregd
        lwpcks11d
        lwscd
        lwsmd
        messages
    (Group: Restart Options)
        syslog
        restart
    (Group: Tests to run)
        users
        groups
        gpo
        ssh
        sudo
        othertests
        delay
        authtest
    (Group: Extra Info to gather)
        automounts
        tcpdump
        gatherdb
    (Group: file locations and options)
        capturefile (string)
        loglevel (string or number)
        logfile (string - loglevel and logfile affect screen output, not commands to daemons)
        tarballdir (string)
        tarballfile (string)
        tarballext (string)
        sshcommand (string in case anything other than ssh auth is requested to be tested.  default is "exit")
        sshuser (string, if not entered, program will prompt for one)
    delaytime (number)
        help

=head4 hash REF info 

$info is a hash reference, with keys as below. This is a multi-level hash as described below
    OStype (string describing OS: solaris, darwin, linux-rpm, etc.)
    svccontrol (The program calls start/stop as: System($info->{svcctl}->{start1}.$info->{svcctl}->{start2}.$info->{svcctl}->{start3}), allowing OSType to determine startup/shutdown)
        start1 (string: first part of init script to start, like "/etc/init.d/")
        start2 (string: second part of init script to start, like "lsassd")
        start3 (string: 3rd part of init script to start, like " start")
        stop1 (string: first part of init script to stop, like "/etc/init.d/")
        stop2 (string: second part of init script to stop, like "lsassd")
        stop3 (string: 3rd part of init script to stop, like " stop")
        rcpath (string: path to rc scripts: "/etc/rc.d")
    pampath (string: path to pam files or file, "/etc/pam.d")
    logpath (string: path to log files, "/var/log")
    logfile (string: system's default log, such as "messages" or "system.log" )
    nsfile (string: full path to nsswitchc.conf or other)
    timezonefile (string: path to system's timezone information)
    platform (string: like "i386" or similar, removes anything after a "-" as returned from Config{platform})
    osversion (string: "uname -r" output)
    uname (string: "uname" output)
    logon (string: login name of user who called the tool (sudo does not mask this))
    name (string: real name of user program is running under (root under sudo))
    uid (number: the effective uidNumber of the user running the program)
    pscmd (string: the command for PS output, since Solaris 10 needs child zones skipped)
    sshd_config (hash reference to "findInPath()" hash value for /etc/sshd_config, or whereever it exists)
        info
        path
        dir
        name
        type
        perm
    krb5conf (hash reference to "findInPath()" hash value for /etc/krb5.conf, or wheereever it exists)
        info
        path
        dir
        name
        type
        perm
    sudoers (hash reference to "findInPath()" hash value for /etc/sudoers, or whereever it exists)
        info
        path
        dir
        name
        type
        perm
    lw (hash reference to Likewise/PBIS-specific information)
        base (string: normally "/opt/pbis")
        path (string: path to bin dir: "/opt/pbis/bin")
        version (string: version of PBIS/Likewise (7.0, 6.5, 6.0, 5.3, 5.1, 4.1))
        smblog (string: name of tool to change smb logging level, which changes from 5.1 to 5.2, doesn't exist previously)
        daemons (hash ref: list of daemons installed)
            smbdaemon (string: name of IO daemon - "lwio", "lwrdr", "npcmuxd")
            authdaemon (string: name of auth daemon - "lsassd" or "winbindd");
            gpdaemon (string: name of group policy daemon "gpagentd" or "centeris.com-gpagentd")
            dcedaemon (string: name of dcerpc endpoint mapper "dcerpcd" or "centeris.com-dcerpcd")
            netdaemon (string: name of netlogon daemon "netlogond" or undef)
            eventlogd (string: name of eventlog daemon "eventlogd or undef)
            eventfwdd (string: name of event forwarder daemon "eventfwdd" or undef)
            syslogreaper (string: name of syslog reaper daemon "reapsysld" or undef)
            registry (string: name of registry daemon "lwregd" or undef)
            lwsm (string: name of service daemon "lwsm" or undef)
            startcmd (string: method to start daemon as daemon from cmd by hand (not via init))
        logging (hash ref: commands for setting up logging)
            command (string for how to change log level live)
            tapcommand (string for tap logging (preferred))
            registry (string for registry commands to change log level for restarts)
        lwsm (hash ref: type of service control)
            control (string: command used to control jobs - "lwsm")
            type (string - "container" "standalone")
            initname (string - path of /etc/init.d/ job for autostart in 6.5)
        tools (hash ref: list of tools available for versioning)
            findsid (string)
            userlist (string)
            grouplist (string)
            userbyname (string)
            userbyid (string)
            groupbyname (string)
            groupbyid (string)
            status (string)
            groupsforuser (string)
            groupsforuid (string)
            regshell (string)
        daemons (hash ref: daemons which have been restarted in debug mode)
            {daemonname} (hash ref: key name may be "netlogond", "lwiod", etc.)
                pid (number: the PID of the daemon launched)
                handle (anonymous ref to the handle used to launch the daemon)
    logedit (hash ref: the file changed if --restart was negated)
        file (findInPath() hash ref)
            info
            path
            dir
            name
            type
            perm
        line (string: the actual line being inserted or commented out)
    tcpdumpfile (string: the location we're storing the tcpdump file we are creationg from the options passed. in $info, rather than $opt, due to centos' funny handling of "pcap" user requiring us to figure out where to store the file)

=head4 findInPath()

findInPath() returns a hash reference with following keys:
    info (array ref: structure from the "stat()" on the file)
    path (string: full path to file)
    dir (string: full path to the dir the file lives in)
    name (string: name of the file itself: "$file->{dir}/$file->{name}" eq "$file->{path}")
    type (character: c,d,f for character, directory, or file)
    perm (character: r,x,w for readable, executable, writable (checked in that order, as writable matters most to our tests. only keep one perm for this test, since stat holds all))

=head4 findProcess()

findProcess() returns a hash reference with the following keys:
    cmd (string: the value of $0 according to "ps -ef" or its OS-specific equivalent)
    bin (string: the best we can determine that the binary name of the process is)
    pid (integer: the PID of the process, for use for killProc2(), for instance)

=cut
