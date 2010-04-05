#!/usr/bin/perl
# Copyright 2008 Likewise Software
# by Robert Auch
# gather information for emailing to support.
#
#
#
# run "perldoc likewise-support.pl" for documentation
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
#
# Data structures explained at bottom of file
#
# TODO:
# syslog-ng editing to allow non-restarts
# gpo testing
# DNS testing
# create tcpdump
# do smbclient tests
# edit lwiauthd and smb.conf for log level = 10 in [global] section properly
# samba test / review
# ps dump?
# gather nscd information?

use strict;
#use warnings;

use Getopt::Long;
use File::Basename;
use Carp;
use FindBin;
my $ver="1.1.3";
my $debug=0;  #the system-wide log level. Off by default, changable by switch --loglevel

my $retval=0; #used to determine exit status of program with bitmasks below:
sub ERR_OPTIONS ()      { 1; }
sub ERR_OS_INFO ()      { 2; }
sub ERR_FILE_ACCESS ()  { 4; }
sub ERR_SYSTEM_CALL ()  { 8; }
sub ERR_DATA_INPUT  ()  { 16; }

my $output=\*STDOUT;

sub main();
main();
exit $retval;

sub usage($)
{
    my $opt = shift || confess "no options hash passed to usage!\n";
    my $scriptName = fileparse($0);

    my $helplines ="
$scriptName version $ver
(C)2009, Likewise Software

usage: $scriptName [tests] [log choices] [options]

  this is our new support tool.  It creates a log as specified by
  the options, and creates a gzipped tarball in:
  $opt->{tarballdir}/$opt->{tarballfile}$opt->{tarballext}, for emailing to 
  support\@likewise.com

  Tests to be performed:

    --(no)ssh (default = ".&getOnOff($opt->{ssh}).")
        Test ssh logon interactively and gather logs
        --sshcommand <command> (default = '".$opt->{sshcommand}."')
    --sshuser <name> (instead of interactive prompt)
    --(no)gpo (default = ".&getOnOff($opt->{gpo}).")
        Perform Group Policy tests
    --(no)users (default = ".&getOnOff($opt->{users}).")
        Enumerate all users
    --(no)groups (default = ".&getOnOff($opt->{groups}).")
        Enumerate all groups
    --(no)automounts (default = ".&getOnOff($opt->{automounts}).")
        Capture /etc/lwi_automount in tarball
    --(no)dns (default = ".&getOnOff($opt->{dns}).")
        DNS lookup tests
    -c --(no)tcpdump (--capture) (default = ".&getOnOff($opt->{tcpdump}).")
        Capture network traffic using OS default tool
        (tcpdump, nettl, snoop, etc.)
        --capturefile <file> (default = $opt->{capturefile})
    --(no)smb (default = ".&getOnOff($opt->{smb}).")
        run smbclient against local samba server
    --(no)other (default = ".&getOnOff($opt->{othertests}).")
        Pause to allow other tests (interactive logon,
        multiple ssh tests, etc.) to be run and logged.

  Log choices: 

    --(no)lsassd (--winbindd) (default = ".&getOnOff($opt->{lsassd}).")
        Gather lsassd debug logs
    --(no)lwiod (--lwrdrd | --npcmuxd) (default = ".&getOnOff($opt->{lwiod}).")
        Gather lwrdrd debug logs
    --(no)netlogond (default = ".&getOnOff($opt->{netlogond}).")
        Gather netlogond debug logs
    --(no)gpagentd (default = ".&getOnOff($opt->{gpagentd}).")
        Gather gpagentd debug logs
    --(no)messages (default = ".&getOnOff($opt->{messages}).")
        Gather syslog logs
    --(no)sambalogs (default = ".&getOnOff($opt->{sambalogs}).")
        Gather logs and config for Samba server

  Options: 

    -r --(no)restart (default = ".&getOnOff($opt->{restart}).")
        Allow restart of the Likewise daemons to separate logs
    --(no)syslog (default = ".&getOnOff($opt->{syslog}).")
        Allow editing syslog.conf during the debug run if not
        restarting daemons (exclusive of -r)
    -v --loglevel {error,warning,info,verbose,debug}
        Changes the logging level. Error skips even section headers,
        so use with caution. (default = $opt->{loglevel} )
    -l --logfile <path> (default = $opt->{logfile} )
        Choose the logfile to write data to.
    -t --tarballdir <path> (default = $opt->{tarballdir} )
        Choose where to create the gzipped tarball of log data

  Examples:

    $scriptName --ssh --lsassd --nomessages --restart -l likewise.log

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
    logger("Stopping $options->{daemon}...", 3);

    if ($info->{$options->{daemon}}->{pid}) {
        #script was started manually (not via startups script)
        # cause we don't store the pid that way
        logger("killing process $options->{daemon} by pid $info->{$options->{daemon}}->{pid}", 3);
        $result = killProc($info->{$options->{daemon}}->{pid}, 9);
        if ($result & ERR_SYSTEM_CALL) {
            logger("Failed kill by ID, trying by name $options->{daemon}", 4);
            $result = killProc("$options->{daemon}", 9);
            if ($result & ERR_SYSTEM_CALL) {
                logger("Couldn't stop or kill $options->{daemon} ", 1);
                logger("Manually stop or kill $options->{daemon} or it will continue running with debugging on.", 1);
            }
        }
    } else {
        $startscript= "$info->{initpath}/$options->{daemon} stop";
        $startscript = $info->{initpath}." stop com.likewisesoftware.$options->{daemon}" if ($info->{initorder} eq "reverse");
        $result = System("$startscript > /dev/null 2>&1");

        if ($result || ($info->{OStype} eq "darwin")) {
            logger("Process $options->{daemon} failed to stop, attempting kill", 2);
            if (defined($info->{$options->{daemon}}->{pid})) {
                logger("killing process $options->{daemon} by pid $info->{$options->{daemon}}->{pid}", 4);
                $result = killProc($info->{$options->{daemon}}->{pid}, 9);
            } else {
                logger("killing process $options->{daemon} with pkill", 4);
                $result = killProc($options->{daemon}, 9);
            }
            if ($result) {
                $retval |= ERR_SYSTEM_CALL;
                logger("Couldn't stop or kill $options->{daemon}", 1);
            }
        }
    }
    if (not defined($options->{loglevel}) or $options->{loglevel} eq "normal") {
        #restart using init scripts, no special logging
        logger("Starting $options->{daemon}...", 3);
        $startscript = $info->{initpath}."/".$options->{daemon}." start";
        $startscript = $info->{initpath}." start com.likewisesoftware.".$options->{daemon} if ($info->{initorder} eq "reverse");
        $result = System("$startscript > /dev/null 2>&1");
        if ($result) {
            $retval |= ERR_SYSTEM_CALL;
            logger("Failed to start $options->{daemon}", 1);
            logger("System may be in an unusable state!!", 1);
        }
    } else {
        if ($info->{lw}->{version} eq "5.2") {
            $logopts = " --loglevel $options->{loglevel} --logfile ".$info->{logpath}."/".$options->{daemon}.".log --start-as-daemon";
        } else {
            $logopts = " --loglevel $options->{loglevel} > ".$info->{logpath}."/".$options->{daemon}.".log 2>&1 &";
        }
        $startscript = $info->{lw}->{base}."/sbin/".$options->{daemon}.$logopts;
        $result = open($info->{$options->{daemon}}->{handle}, "$startscript > /dev/null|");
        if ($result) {
            $result++ unless ($info->{OStype} eq "aix");
            $info->{$options->{daemon}}->{pid} = $result;
            logger("pid for $options->{daemon} = $result", 4);
        } else {
            $retval |= ERR_SYSTEM_CALL;
            logger("Failed to start $options->{daemon}", 1);
            logger("System may be in an unusable state!!", 1);
        }
    }
    return $info;
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
        logger("No username passed for user info lookup!", 1);
        $retval |= ERR_DATA_INPUT;
        return $retval;
    }
    logger("getent passwd $name: ", 2);
    logger(join(":", getpwnam($name)), 2);
    logger("", 2);
    return $retval if ($name=~/root/i);  #no need to do AD lookups for root
    logger("likewise direct lookup:", 2);
    if ($info->{lw}->{version} =~/^5./) {
        $data = runTool($info, $opt, "lw-find-user-by-name --level 2 $name");
        logger($data, 2);

    } elsif ($info->{lw}->{version} eq "4.1") {
        $data = runTool($info, $opt, "lwiinfo -i $name");
        logger($data, 2);
    }

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
    my $file= {};

    foreach my $path (@$paths) {
        if (-e "$path/$filename") {
            $file->{info} = stat(_);
            $file->{path} = "$path/$filename";
            $file->{type}="d" if (-d _);
            $file->{type}="f" if (-f _);
            $file->{type}="c" if (-c _);
            $file->{perm}="r" if (-r _);
            $file->{perm}="x" if (-x _);
            $file->{perm}="w" if (-w _);
            $file->{name}=$filename;
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
    if ($file->{perm} ne "w") {
        $retval |= ERR_FILE_ACCESS;
        logger("could not read from $file->{path} to see if '$line' already exists", 1);
        return $retval;
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
    logger($data, 5) if ($data);
    if (defined($error) && $error ne "found") {
        logger("Could not find '$line' in $file->{path}.", 3);
    }
}

sub lineInsert($$) {
    my $file = shift || confess "ERROR: No file hash to insert line into!\n";
    my $line = shift || confess "ERROR: no line to insert into $file!\n";
    my $error;
    if ($file->{perm} ne "w") {
        $retval |= ERR_FILE_ACCESS;
        logger("could not read from $file->{path} to see if '$line' already exists", 1);
        return $retval;
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
    logger($data, 5) if ($data);
    if (defined($error) && $error ne "found") {
        open(FH, ">>$file->{path}");
        $error = print FH "$line\n";
        unless ($error) {
            $retval |= ERR_FILE_ACCESS;
            logger("Could not append $line to $file->{path} - $error - $!\n", 1);
        }
        close FH;
    }
}

sub logger($$) {
    # Writes to the global $output file handle
    # adds "ERROR:, VERBOSE: or DEBUG:" to appropriate line levels for 
    # easier parsing of logfile later.
    # does not add "WARNING:" to level 2 because that's used
    # for normal output as well
	my $line=shift; # now ok to pass empty line to logger ||confess "ERROR: No line to log passed to logger!\n";
	my $level=shift ||confess "ERROR: No verbosity level passed to logger!\n";

    return $retval if ($level>$debug);

    $line = " " if (not defined($line));

    my $error=0;
    chomp $line;
	if ($level<=1) {
        $line="ERROR: ".$line;
		$error = print STDERR "$line\n";
        $retval |= ERR_FILE_ACCESS unless $error;
        #TODO
        # figure out how to not print this line twice if $output = \*STDOUT
    
	} elsif ($level == 4) {
        $line="VERBOSE: ".$line;
    } elsif ($level>4) {
        $line="DEBUG: ".$line;
    }
    $error = print $output "$line\n";
    $retval |= ERR_FILE_ACCESS unless $error;
    if ($output != \*STDOUT) {
        print "$line\n";
    }
	return $retval;
}

sub killProc($$) {
    my $process = shift || confess "ERROR: no process to kill!\n";
    my $signal = shift || confess "ERROR: No signal to send to $process!\n";

    if (not ($process =~/^\d+$/)) {
        logger("Passed $process by name, figuring out its PID...", 5);
        my @lines;
        open(PS, "ps -e|");
        my $catch;
        while (<PS>) {
            chomp;
            if ($_=~/$process/) {
                /(\d+)/;
                $catch=$1;
            }
            last if $catch;
        }
        if ($catch) {
            logger("Found $process with pid $catch.", 4);
            $process = $catch;
        } else {
            $retval |= ERR_SYSTEM_CALL;
            logger("Could not pkill $process - it does not appear to be running!", 1);
            return $retval;
        }
    }
    logger("Attemping to kill PID $process with signal $signal", 3);
    my $error = kill($signal, $process);
    unless ($error) {
        # kill returns number of processes killed.
        $retval |= ERR_SYSTEM_CALL;
        logger("Could not kill PID $process with signal $signal - $!", 1);
        return $retval;
    } else {
        logger("successfully killed $error processes", 4);
        return 0;
    }
}

sub runTool($$$) {
    my $info = shift || confess "no info hash passed to runTool!\n";
    my $opt = shift || confess "no opt hash passed to runTool!\n";
    my $tool = shift || confess "no tool to run passed to runTool!\n";

    my $data=`$info->{lw}->{path}/$tool`;
    if ($?) {
        $retval |= ERR_SYSTEM_CALL;
        logger("Error running $tool!", 1);
    }
    return $data;
}

sub safeRegex($) {
    my $line = shift || confess "no line to clean up for regex matching!\n";
    my $regex = $line;
    $regex=~s/([\*\[\]\-\(\)\.\?\/\^\\])/\\$1/g;
#    $regex=~s/\([][^$\/]\)/\\\1/g;
    logger("Cleaned up '$line' as '$regex'", 5);
    return $regex;
}

sub sectionBreak($) {
    my $title = shift || confess "no title to print section break to!\n";
    logger(" ", 2);
    logger("############################################", 2);
    logger("# Section $title", 2);
    logger("# ".scalar(localtime()), 2);
    logger("# ", 2);
    return 0;
}

sub System($;$$)
{
    my $command = shift || confess "No Command to launch passed to System!\n";
    my $print = shift;
    my $timeout = shift;

    if (defined($print) && $print=~/^\d+$/) {
        logger("RUN: $command", $print);
    }

    if ($timeout) {
        my $pid = fork();
        if (not defined $pid) {
            logger("FORK failed for: $command", 1);
            return 1;
        }
        if (not $pid) {
            if (not exec("nohup $command"))
            {
                exit(1);
            }
            exit($?);
        } else {
            my $rc;
            eval {
                local $SIG{ALRM} = sub { logger("ALARM: process timeout", 1); };
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
                    logger("\n*** PROCESS TIMED OUT ***\n", 1);
                    kill 15, $pid;
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

    logger("Adding file $file to $tar", 3);
    my $error;
    if (-e $tar) {
        $error = System("tar -rf $tar $file > /dev/null 2>&1");
    } else {
        $error = System("tar -cf $tar $file > /dev/null 2>&1");
    }
    if ($error) {
        $retval |= ERR_SYSTEM_CALL;
        logger("Error $error adding $file to $tar - $!", 1);
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
    logger("Determining restart ability", 5);
    if ($opt->{restart} && $info->{uid} == 0) {
        logger("requested to restart daemons, beginning.", 5);
        my $options = { daemon => "",
                        loglevel => "$state",
                    };

        if ($opt->{dcerpcd} && defined($info->{lw}->{dcedaemon})) {
            logger("Attempting restart of dce endpoint mapper daemon", 3);
            $options->{daemon} = $info->{lw}->{dcedaemon};
            $info = daemonRestart($info, $options);
            sleep 5;
        }
        if ($opt->{netlogond} && defined($info->{lw}->{netdaemon})) {
            logger("Attempting restart of netlogon daemon", 3);
            $options->{daemon} = $info->{lw}->{netdaemon};
            $info = daemonRestart($info, $options);
            sleep 5;
        }
        if ($opt->{lwiod} && defined($info->{lw}->{smbdaemon})) {
            logger("Attempting restart of SMB daemon", 3);
            $options->{daemon} = $info->{lw}->{smbdaemon};
            $info = daemonRestart($info, $options);
            sleep 5;
        }
        if ($opt->{lsassd} && defined($info->{lw}->{authdaemon})) {
            logger("attempting restart of auth daemon", 3);
            $options->{daemon} = $info->{lw}->{authdaemon};
            if ($info->{lw}->{version} eq "4.1") {
                #TODO add code to edit lwiauthd.conf
            } 
            if ($info->{OStype} eq "darwin") {
                killProc("DirectoryService", "USR1");
            }
            $info = daemonRestart($info, $options);
            sleep 5;
            
        }
        if ($opt->{gpagentd} && defined($info->{lw}->{gpdaemon})) {
            logger("Attempting restart of Group Policy daemon", 3);
            $options->{loglevel} = 4 if ($state eq "debug");
            $options->{loglevel} = 4 if ($state eq "verbose");
            $options->{loglevel} = 3 if ($state eq "info"); 
            $options->{loglevel} = 2 if ($state eq "warning");
            $options->{loglevel} = 1 if ($state eq "error");
            $options->{daemon} = $info->{lw}->{gpdaemon};
            $info = daemonRestart($info, $options);
            sleep 5;
        }
    } else {
        if ($info->{uid} != 0) {
            logger("can't restart daemons or make syslog.conf changes", 1);
            logger("This tool needs to be run as root for these options", 1);
            $retval |= ERR_SYSTEM_CALL;
            return $info;
        }
        my ($error);
        if (not(defined($info->{logedit}->{file}))) {
            $info->{logedit} = {};
            $info->{logedit}->{line} = "*.*\t\t\t\t$info->{logpath}/$info->{logfile}";
            $info->{logedit}->{line} = "*.debug\t\t\t\t$info->{logpath}/$info->{logfile}" if ($info->{OStype} eq "solaris");
            $info->{logedit}->{file} = findInPath("syslog.conf", ["/etc", "/etc/syslog", "/opt/etc/"]);
        }
        if ($info->{logedit}->{file}->{type} eq "f") {
            if ($state eq "normal") {
                if (not defined($info->{logedit}->{line})) {
                    logger("Don't know what to remove from syslog.conf!!", 1);
                    return $info;
                }
                logger("Removing debug logging from syslog.conf", 2);
                lineDelete($info->{logedit}->{file}, $info->{logedit}->{line});
                if ($info->{lw}->{version}=~/^5\./) {
                    logger("Changing log levels for Likewise daemons to $state", 2);
                    runTool($info, $opt, "lw-set-log-level warning") if ($opt->{lsassd});
                    runTool($info, $opt, "$info->{lw}->{smblog} warning") if (($opt->{lwiod}) && ($info->{lw}->{smblog}));
                } elsif ($info->{lw}->{version} eq "4.1") {
                    #TODO Put in changes for lw 4.1
                }
            } else {
                # Force the "messages" option on, since that's where we'll gather data from
                $opt->{messages} = 1;
                logger("system has syslog.conf, editing to capture debug logs", 2);
                lineInsert($info->{logedit}->{file}, $info->{logedit}->{line});
                if ($info->{lw}->{version}=~/^5\./) {
                    logger("Changing log levels for Likewise daemons to $state", 2);
                    runTool($info, $opt, "lw-set-log-level $state") if ($opt->{lsassd});
                    runTool($info, $opt, "$info->{lw}->{smblog} $state") if ($opt->{lwiod} && $info->{lw}->{smblog});
                } elsif ($info->{lw}->{version} eq "4.1") {
                    #TODO Put in changes for lw 4.1
                }
            }
            killProc("syslog", 1);
        } else {
            $retval |= ERR_FILE_ACCESS;
            logger("syslog.conf is not a file, could not edit!", 1)
        }
    }

    return $info;
}

sub determineOS($) {
    my $info = shift || confess "no info hash passed";
    logger("Determining OS Type...", 5);
    my $file={};
    my $uname;

    if ($^O eq "linux") {
        foreach my $i (("rpm", "dpkg")) {
            $file=findInPath($i, ["/sbin", "/usr/sbin", "/usr/bin", "/bin", "/usr/local/bin", "/usr/local/sbin"]);
            if ((defined($file->{path})) && $file->{type} eq "x") {
                $info->{OStype} = "linux-$i";
                logger("System is $info->{OStype}", 5);
            } 
        }
        $info->{timezonefile} = "/etc/sysconfig/clock";
        if (not defined($info->{OStype})) {
            $retval |= ERR_OS_INFO;
            logger("Could not determine Linux subtype", 2);
            $info->{OStype} = "linux-unknown";
        } elsif ($info->{OStype} eq "linux-dpkg") {
            $info->{OStype} = "linux-deb";
            $info->{timezonefile} = "/etc/timezone";
        }
        logger("Setting Linux paths", 5);
        $info->{initpath} = "/etc/init.d";
        $info->{initorder} = "normal";
        $info->{rcpath} = "/etc/rc.d";
        $info->{pampath} = "/etc/pam.d";
        $info->{logpath} = "/var/log";
        $info->{logfile} = "messages";
        $info->{nsfile} = "/etc/nsswitch.conf";
    } elsif ($^O eq "hpux") {
        $info->{OStype} = "hpux";
        logger("Setting HP-UX paths", 5);
        $info->{initpath} = "/sbin/init.d";
        $info->{initorder} = "normal";
        $info->{rcpath} = "/etc/rc.d";
        $info->{pampath} = "/etc/pam.conf";
        $info->{logpath} = "/var/adm";
        $info->{logfile} = "messages";
        $info->{nsfile} = "/etc/nsswitch.conf";
        $info->{timezonefile} = "/etc/TIMEZONE";
    } elsif ($^O eq "solaris") {
        $info->{OStype} = "solaris";
        logger("Setting Solaris paths", 5);
        $info->{initpath} = "/etc/init.d";
        $info->{initorder} = "normal";
        $info->{rcpath} = "/etc/init.d";
        $info->{pampath} = "/etc/pam.conf";
        $info->{logpath} = "/var/adm";
        $info->{logfile} = "messages";
        $info->{nsfile} = "/etc/nsswitch.conf";
        $info->{timezonefile} = "/etc/TIMEZONE";
    } elsif ($^O eq "aix") {
        $info->{OStype} = "aix";
        logger("Setting AIX paths", 5);
        $info->{initpath} = "/etc/rc.d/init.d";
        $info->{initorder} = "normal";
        $info->{rcpath} = "/etc/rc.d";
        $info->{pampath} = "/etc/pam.conf";
        $info->{logpath} = "/var/adm";
        $info->{logfile} = "syslog";
        $info->{nsfile} = "/etc/netsvc.conf";
        $info->{timezonefile} = "/etc/environment";
    } elsif ($^O eq "MacOS" or $^O eq "darwin") {
        $info->{OStype} = "darwin";
        logger("Setting darwin paths", 5);
        $info->{initpath} = "launchctl";
        $info->{initorder} = "reverse";
        $info->{rcpath} = "/etc/rc.d";
        $info->{pampath} = "/etc/pam.d";
        $info->{logpath} = "/var/log";
        $info->{logfile} = "system.log";
        $info->{nsfile} = "/etc/nsswitch.conf";
        $info->{timezonefile} = "/etc/localtime";
    } else {
        $retval |= ERR_OS_INFO;
        $info->{OStype} = "unknown";
        logger("ERROR: Could not determine OS information!", 1);
        $info->{timezonefile} = "/etc/localtime";
        $info->{initpath} = "/etc/init.d";
        $info->{initorder} = "normal";
        $info->{rcpath} = "/etc/rc.d";
        $info->{pampath} = "/etc/pam.d";
        $info->{logpath} = "/var/log";
        $info->{logfile} = "messages";
        $info->{nsfile} = "/etc/nsswitch.conf";
    }
    logger("OS: $info->{OStype}", 2);
    use Config;
    $info->{platform} = $Config{myarchname};
    $info->{platform}=~s/-.*$//;
    $info->{osversion} = `uname -r`;
    $info->{uname} = `uname`;
    logger("Version: $info->{osversion}", 2);
    logger("Platform: $info->{platform}", 2);
    logger("Full Uname: $info->{uname}", 2);
    $info->{logon} = getlogin();
    $info->{name} = getpwuid($<);
    $info->{uid} = getpwnam($info->{name});
    logger("Currently running as: $info->{name} with effective UID: $info->{uid}", 4);
    logger("Run under sudo from $info->{logon}", 4) if ($info->{logon} ne $info->{name});
    logger("Gathered at: ".scalar(localtime), 2);
    $info->{sshd_config}=findInPath("sshd_config", ["/etc/ssh", "/opt/ssh/etc", "/usr/local/etc", "/etc", "/etc/openssh", "/usr/openssh/etc", "/opt/csw/etc", "/services/ssh/etc/"]);
    $info->{krb5conf}=findInPath("krb5.conf", ["/etc/krb5", "/opt/krb5/etc", "/usr/local/etc", "/usr/local/etc/krb5", "/etc", "/opt/csw/etc"]);
    $info->{sudoers}=findInPath("sudoers", ["/etc", "/usr/local/etc", "/opt/etc", "/opt/local/etc", "/opt/usr/local/etc"]);
    $info->{sambaconf}=findInPath("smb.conf", ["/etc/samba", "/etc/smb", "/opt/etc/samba", "/usr/local/etc/samba", "/etc/opt/samba"]);
    logger("Found sshd_config at $info->{sshd_config}->{path}", 2) if ($info->{sshd_config}->{path});
    logger("Found krb5.conf at $info->{krb5conf}->{path}", 2) if ($info->{krb5conf}->{path});
    logger("Found sudoers at $info->{sudoers}->{path}", 2) if ($info->{sudoers}->{path});
    logger("Found smb.conf at $info->{sambaconf}->{path}", 2) if ($info->{sambaconf}->{path});
    return $info;
}

sub waitForDomain($$) {
    my $info = shift || confess "no info hash passed to waitForDomain!\n";
    my $opt = shift || confess "no options hash passed to waitForDomain!\n";
    logger("Waiting for auth daemon to find domains and finish startup.", 3);
    if ($info->{lw}->{version}=~/^5\./) {
        my ($error, $i)=(0,0);
        for ($i=0; $i<30; $i++) {
            sleep 5;
            $error=System("$info->{lw}->{path}/lw-get-status >/dev/null 2>&1");
            last unless $error;
            # lw-get-status returns 0 for success, 2 if lsassd hasn't started yet
        }
    }
}

sub getLikewiseVersion($) {

    # determine likewise version installed
    # look in reverse order, in case a bad upgrade was done
    # we can get the current running version

    my $info=shift;
    my $error=0;
    my $lwregd=findInPath("lwregd", ["/opt/likewise/sbin/"]);
    my $lwiod=findInPath("lwiod", ["/opt/likewise/sbin/"]);
    my $lwrdrd=findInPath("lwrdrd", ["/opt/likewise/sbin/"]);
    my $npcmuxd=findInPath("npcmuxd", ["/opt/likewise/sbin/"]);
    my $winbindd=findInPath("winbindd", ["/opt/centeris/sbin/", "/usr/centeris/sbin"]);
    my $gporefresh=findInPath("gporefresh", ["/opt/centeris/bin/", "/usr/centeris/bin", "/opt/likewise/bin"]);
    if (defined($lwregd->{path})) {
        $info->{lw}->{version} = "5.3";
        $info->{lw}->{base} = "/opt/likewise";
        $info->{lw}->{path} = "/opt/likewise/bin";
        $info->{lw}->{smblog} = "lwio-set-log-level";
        $info->{lw}->{smbdaemon} = "lwiod";
        $info->{lw}->{authdaemon} = "lsassd";
        $info->{lw}->{gpdaemon} = "gpagentd";
        $info->{lw}->{dcedaemon} = "dcerpcd";
        $info->{lw}->{netdaemon} = "netlogond";
        $info->{lw}->{eventlogd} = "eventlogd";
        $info->{lw}->{eventfwdd} = "eventfwdd";
        $info->{lw}->{syslogreaper} = "reapsysldd";
        $info->{lw}->{registry} = "lwregd";
        logger("Likewise Version 5.3 installed", 2);
    } elsif (defined($lwiod->{path})) {
        $info->{lw}->{version} = "5.2";
        $info->{lw}->{base} = "/opt/likewise";
        $info->{lw}->{path} = "/opt/likewise/bin";
        $info->{lw}->{smblog} = "lwio-set-log-level";
        $info->{lw}->{smbdaemon} = "lwiod";
        $info->{lw}->{authdaemon} = "lsassd";
        $info->{lw}->{gpdaemon} = "gpagentd";
        $info->{lw}->{dcedaemon} = "dcerpcd";
        $info->{lw}->{netdaemon} = "netlogond";
        $info->{lw}->{eventlogd} = "eventlogd";
        $info->{lw}->{eventfwdd} = "eventfwdd";
        $info->{lw}->{syslogreaper} = "reapsysldd";
        logger("Likewise Version 5.2 installed", 2);
    } elsif (defined($lwrdrd->{path})) {
        $info->{lw}->{version}= "5.1";
        $info->{lw}->{base} = "/opt/likewise";
        $info->{lw}->{path} = "/opt/likewise/bin";
        $info->{lw}->{smblog} = "lw-smb-set-log-level";
        $info->{lw}->{smbdaemon} = "lwrdrd";
        $info->{lw}->{authdaemon} = "lsassd";
        $info->{lw}->{gpdaemon} = "gpagentd";
        $info->{lw}->{dcedaemon} = "dcerpcd";
        $info->{lw}->{netdaemon} = "netlogond";
        $info->{lw}->{eventlogd} = "eventlogd";
        logger("Likewise Version 5.1 installed", 2);
    } elsif (defined($npcmuxd->{path})) {
        $info->{lw}->{version}= "5.0";
        $info->{lw}->{base} = "/opt/likewise";
        $info->{lw}->{path} = "/opt/likewise/bin";
        $info->{lw}->{smbdaemon} = "npcmuxd";
        $info->{lw}->{authdaemon} = "lsassd";
        $info->{lw}->{gpdaemon} = "gpagentd";
        $info->{lw}->{dcedaemon} = "dcerpcd";
        $info->{lw}->{netdaemon} = "netlogond";
        $info->{lw}->{eventlogd} = "eventlogd";
        logger("Likewise Version 5.0 installed", 2);
    } elsif (defined($winbindd->{path})) {
        $info->{lw}->{version}= "4.1";
        if ($info->{OStype}=~/linux/) {
            $info->{lw}->{base} = "/usr/centeris";
            $info->{lw}->{path} = "/usr/centeris/bin";
        } else {
            $info->{lw}->{base} = "/opt/centeris";
            $info->{lw}->{path} = "/opt/centeris/bin";
        }
        $info->{lw}->{smbdaemon} = "centeris.com-npcmuxd";
        $info->{lw}->{authdaemon} = "winbindd";
        $info->{lw}->{gpdaemon} = "centeris.com-gpagentd";
        $info->{lw}->{dcedaemon} = "centeris.com-dcerpcd";
        logger("Likewise Version 4.1 installed", 2);
    }
    if ($info->{lw}->{version}=~/^5\./) {
        $error = open(SV, "<$info->{lw}->{base}/data/VERSION");
        unless ($error) {
            $retval |= ERR_FILE_ACCESS;
            logger("Can't open $info->{lw}->{base}/data/VERSION", 1);
        } else {
            while (<SV>) {
                logger($_, 2);
            }
            close SV;
        }
    }
    if (not defined($gporefresh->{path})) {
        # Likewise Open doesn't include gporefresh or the following daemons, so mark them undef,
        # This way, we won't attempt to restart them later, or do anything with them.
        # Reduces errors printed to screen.
        undef $info->{lw}->{gpdaemon};
        undef $info->{lw}->{eventlogd};
        undef $info->{lw}->{eventfwd};
        undef $info->{lw}->{syslogreaper};
    }
    return $info;
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
        $retval |= ERR_FILE_ACCESS;
        logger("Can't write log tarball $opt->{tarballfile}$opt->{tarballext}!", 1);
        logger("both $opt->{tarballdir} and ./ are non-writable!", 1);
        return $retval;
    }
    if (-e $tarballfile.$opt->{tarballext}) {
        logger("WARNING: file $tarballfile".$opt->{tarballext}." exists, adding ext...", 2);
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
        $tarballfile=~s/\.gz$//;
    }
    logger("Creating tarball $tarballfile and adding logs", 4);
    if ($opt->{restart} && $info->{uid} == 0) {
        if ($opt->{lsassd}) {
            logger("Adding auth daemon log", 5);
            $appendfile = $info->{logpath}."/lsassd.log";
            tarFiles($info, $opt, $tarballfile, $appendfile);
            tarFiles($info, $opt, $tarballfile, "/Library/Logs/DirectoryService/DirectoryService.debug.log") if ($info->{OStype} eq "darwin");
        }
        if ($opt->{lwiod}) {
            logger("Adding SMB Daemon log", 5);
            my $daemon;
            if ($info->{lw}->{version}=~/5\.[2-5]/) {
                logger("SMB Daemon is lwiod", 5);
                $daemon = "/lwiod.log";
                $appendfile = $info->{logpath}.$daemon;
                tarFiles($info, $opt, $tarballfile, $appendfile);
            } elsif ($info->{lw}->{version} eq "5.1") {
                logger("SMB Daemon is lwrdrd", 5);
                $daemon = "/lwrdrd.log";
                $appendfile = $info->{logpath}.$daemon;
                tarFiles($info, $opt, $tarballfile, $appendfile);
            } elsif ($info->{lw}->{version} eq "5.0") {
                logger("SMB Daemon is npcmuxd", 5);
                $daemon = "/npcmuxd.log";
                $appendfile = $info->{logpath}.$daemon;
                tarFiles($info, $opt, $tarballfile, $appendfile);
            } else {
                $retval |= ERR_OPTIONS;
                logger("Unknown likewise version to gather SMB daemon info from", 1);
            }
        }
        if ($opt->{netlogond}) {
            logger("adding netlogond log", 5);
            $appendfile = $info->{logpath}."/netlogond.log";
            tarFiles($info, $opt, $tarballfile, $appendfile);
        }
        if ($opt->{gpagentd}) {
            logger("Adding gpagentd log", 5);
            $appendfile = $info->{logpath}."/gpagentd.log";
            tarFiles($info, $opt, $tarballfile, $appendfile);
        }
    } elsif ($info->{uid} ne 0) {
        logger("Not adding any logs from $info->{logpath}, we have no rights", 2);
        $retval |= ERR_OPTIONS;
    }
    if ($opt->{messages}) {
        logger("Adding $info->{logpath}/messages", 5);
        $appendfile = $info->{logpath}."/".$info->{logfile};
        tarFiles($info, $opt, $tarballfile, $appendfile);
    }
    if ($opt->{automounts}) {
        logger("Adding autofs files from Likewise GPO", 5);
        tarFiles($info, $opt, $tarballfile, "/etc/lwi_automount/*");
        logger("Now adding /etc/auto* as well", 5);
        tarFiles($info, $opt, $tarballfile, "/etc/auto*");
    }
    if ($opt->{sambalogs}) {
        logger("Adding files from $info->{logpath}/samba", 5);
        tarFiles($info, $opt, $tarballfile, "$info->{logpath}/samba/*");
        if ($info->{logpath} ne "/var/log") {
            logger("Adding files from /var/log/samba also", 5);
            tarFiles($info, $opt, $tarballfile, "/var/log/samba/*");
        }
        logger("Adding samba configuration files", 5);
        tarFiles($info, $opt, $tarballfile, "$info->{sambaconf}->{dir}/*"); 
    }
    if ($opt->{ssh}) {
        logger("Adding sshd_config", 5);
        tarFiles($info, $opt, $tarballfile, $info->{sshd_config}->{path}) if ($info->{sshd_config}->{path});
        logger("Can't find sshd_config to add to tarball!", 1) unless ($info->{sshd_config}->{path});
    }
    if ($opt->{authtest}) {
        logger("Adding pam files", 5);
        tarFiles($info, $opt, $tarballfile, $info->{pampath});
        logger("Adding krb5.conf", 5);
        tarFiles($info, $opt, $tarballfile, $info->{krb5conf}->{path}) if ($info->{krb5conf}->{path});
        logger("Can't find krb5.conf to add to tarball!", 1) unless ($info->{krb5conf}->{path});
    }
    if ($opt->{sudo}) {
        logger("Adding sudoers", 5);
        tarFiles($info, $opt, $tarballfile, $info->{sudoers}->{path}) if ($info->{sudoers}->{path});
        logger("Can't find krb5.conf to add to tarball!", 1) unless ($info->{sudoers}->{path});
    }
    if ($opt->{tcpdump}) {
        logger("Adding tcpdump capture...", 5);
        tarFiles($info, $opt, $tarballfile, $info->{tcpdumpfile});
    }
    # Files to add under all circumstances
    logger("Adding Likewise Configuration...", 5);
    tarFiles($info, $opt, $tarballfile, "/etc/likewise");
    tarFiles($info, $opt, $tarballfile, $info->{nsfile});
    tarFiles($info, $opt, $tarballfile, $info->{timezonefile});
    logger("Finished adding files, now for our output", 5);
    if ($opt->{logfile} eq "-") {
        logger("Can't add STDOUT to tarball - you need to run with an actual logfile!", 1);
    } else {
        tarFiles($info, $opt, $tarballfile, $opt->{logfile});
    }
    if ($opt->{tarballext}=~/\.gz$/) {
        logger("Output gathered, now gzipping for email", 5);
        $error = System("gzip $tarballfile");
        if ($error) {
            $retval |= ERR_SYSTEM_CALL;
            logger("Can't gzip $tarballfile - $!", 1);
        } else {
            $tarballfile = "$tarballfile.gz";
        }
    }
    logger("All data gathered successfully in $tarballfile.", 2);
    logger("Please email $tarballfile to support\@likewise.com to help diagnose your problem", 2);
}

sub runTests($$) {
    my $info=shift || confess "no info hash passed to test runner!\n";
    my $opt=shift || confess "no options hash passed to test runner!\n";
    my $data;
    # Run tests that run every time no matter what

    sectionBreak("lw-get-status");
    $data = runTool($info, $opt, "lw-get-status");
    logger($data, 2);

    sectionBreak("Running User");
    getUserInfo($info, $opt, $info->{logon});

    sectionBreak("DC Times");
    my $domains = runTool($info, $opt, "lw-get-status | sed -n '/DNS Domain:/p'");
    foreach my $domain (split(/\n/, $domains)) {
        sectionBreak("Current DC Time for $domain.");
        $domain=~/[^\s]+$/;
        $data = runTool($info, $opt, "lw-get-dc-time $domain");
        logger($data, 2);
    }

    # run optional tests
    if ($opt->{users}) {
        sectionBreak("Enum Users");
        if ($info->{lw}->{version}=~/^5\./) {
            $data = runTool($info, $opt, "lw-enum-users --level 2");
        } elsif ($info->{lw}->{version} eq "4.1") {
            $data = runTool($info, $opt, "lwiinfo -U");
        }
        logger($data, 2) if ($data);
    }
    if ($opt->{groups}) {
        sectionBreak("Enum Groups");
        if ($info->{lw}->{version} eq "5.2") {
            $data=runTool($info, $opt, "lw-enum-groups --level 1 -c");
        } elsif ($info->{lw}->{version}=~/^5\./) {
            $data=runTool($info, $opt, "lw-enum-groups --level 1");
        } elsif ($info->{lw}->{version} eq "4.1") {
            $data=runTool($info, $opt, "lwiinfo -G");
        }
        logger($data, 2) if ($data);
    }

    if ($opt->{ssh}) {
        sectionBreak("SSH Test");
        my $user;
        if ($opt->{sshuser}) {
            $user = $opt->{sshuser};
        } else {
            logger("Testing SSH as an AD user - please enter a username here:", 1);
            $user = <STDIN>;
            chomp $user;
            $opt->{sshuser} = $user;
        }
        $user=safeRegex($user);
        logger("Looking up $user prior to test", 4);
        getUserInfo($info, $opt, $user);
        my $sshcommand = "ssh -vvv -l $user localhost '$opt->{sshcommand}' 2>&1";
        
        logger("Running ssh as: $sshcommand", 3);
        $data=`$sshcommand`;
        if ($?) {
            $retval |= ERR_SYSTEM_CALL;
            logger("Error running ssh as $user!", 1);
        }
        logger($data, 2);
    } elsif ($opt->{sshuser} || $info->{uid} ne "0") {
        my $user = $opt->{sshuser};
        $user = $info->{logon} if (not defined($opt->{sshuser}));
        sectionBreak("User Lookup");
        getUserInfo($info, $opt, $user);
    }

    if ($opt->{gpo}) {
        sectionBreak("GPO Tests");
        $opt->{dns} = 1; #do dns tests as well if we're testing GPO, they often are related
        #TODO Write
    }

    if ($opt->{dns}) {
        sectionBreak("DNS Tests");
        #TODO Write
    }
    
    if ($opt->{smb}) {
        sectionBreak("SMB Tests");
        #TODO Write
    }

    if ($opt->{sudo}) {
        sectionBreak("Sudo Test");
        logger("Opening a bash shell in this window.", 2);
        logger("Perform the sudoers tests required, then type 'exit'", 2);
        #TODO Fix this
        logger("Some output may not print to screen - this is OK", 1);
        my $file=findInPath("bash", ["/bin", "/usr/bin", "/usr/local/bin"]);
        $data=`$file->{path}`;
        logger($data, 2);
    }
    
    if ($opt->{othertests}) {
        sectionBreak("Other Tests");
        logger("Please run any manual tests required now (interactive logon, sudo, su, etc.)", 2);
        logger("Type 'done' and hit Enter when complete...", 2);
        my $complete = "";
        while (not ($complete=~/done/i)) {
            $complete=readline(*STDIN);
            chomp $complete;
        }
    }

    if ($opt->{authtest}) {
        sectionBreak("PAC info");
        my ($logfile, $error);
        $data="";
        $logfile = "$info->{logpath}/$info->{logfile}" if ($opt->{syslog});
        $logfile = "$info->{logpath}/lsassd.log" if ($opt->{restart});
        my %sids;
        open(LF, "<$logfile");
        while(<LF>) {
            if (/PAC (\w+ )?membership is (S-1-5[^\s]+)/) {
                logger("Found SID $2", 5);
                $sids{$2}=1;
            }
        }
        foreach my $sid (sort(keys(%sids))) {
            $data=runTool($info, $opt, "lw-find-by-sid $sid");
            if ($data) {
                logger($data, 2);
                $error=1;
                $data="";
            } else {
                logger("Error getting user by SID $sid!", 1);
            }
        }
        close LF;
        logger("Couldn't find any PAC information to review", 2) unless ($error);
    }

    return $info;    
}

# Main Functions End
#####################################

###########################################
# controller subroutine starts here
sub main() {

#    Getopt::Long::Configure('no_ignore_case', 'no_auto_abbrev') || confess;

    my @time = localtime();
    $time[5]+=1900;
    my $datestring = $time[5].sprintf("%02d", $time[4]+1).sprintf("%02d", $time[3]).sprintf("%02d", $time[2]).sprintf("%02d", $time[1]);

    my $opt = { netlogond => 1,
            users => 1,
            groups => 1,
            lsassd => 1,
            lwiod => 1,
            netlogond => 1,
            gpagentd => 1,
            messages => 1,
            restart => 1,
            authtest => 1,
            capturefile => "/tmp/likewise-support.cap",
            loglevel => "info",
            logfile => "/tmp/likewise-support.log",
            tarballdir => "/tmp",
            tarballfile => "lw-support-$datestring",
            tarballext => ".tar.gz",
            sshcommand => "exit",
        };

    my $ok = GetOptions($opt,
        'help|h|?',
        'logfile|log|l=s',
        'loglevel|v=s',
        'tarballdir|t=s',
        'ssh!',
        'gpo|grouppolicy!',
        'users!',
        'groups!',
        'sudo!',
        'automounts|autofs!',
        'tcpdump|capture|c!',
        'othertests|other!',
        'lsassd|winbindd!',
        'lwiod|lwrdrd|npcmuxd!',
        'netlogond!',
        'gpagentd!',
        'messages!',
        'sambalogs!',
        'restart|r!',
        'syslog!',
        'sshcommand=s',
        'sshuser=s',
    );
    my $more = shift @ARGV;
    my $errors;

    if ($opt->{syslog}) {
        $opt->{restart} = 0;
    }
    if (not $opt->{restart}) {
        $opt->{syslog} = 1;
    }

    if ($opt->{help} or not $ok) {
        $retval |= ERR_OPTIONS;
        print usage($opt);
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
        $retval |= ERR_OPTIONS;
        print $errors.usage($opt);
    }

    exit $retval if $retval;

    if (defined($opt->{logfile}) && $opt->{logfile} ne "-") {
        open(OUTPUT, ">$opt->{logfile}") || die "can't open logfile $opt->{logfile}\n";
		$output = \*OUTPUT;
		logger("Initializing logfile $opt->{logfile}.", 3);
	} else { 
		$output = \*STDOUT;
		logger("Logging to STDOUT.", 3);
	}

    if (defined($opt->{loglevel})) {
        $debug=5 if ($opt->{loglevel}=~/^debug$/i);
        $debug=4 if ($opt->{loglevel}=~/^verbose$/i);
    	$debug=3 if ($opt->{loglevel}=~/^info$/i);
    	$debug=2 if ($opt->{loglevel}=~/^warning$/i);
    	$debug=1 if ($opt->{loglevel}=~/^error$/i);
        $debug=$opt->{loglevel} if ($opt->{loglevel}=~/^\d+$/);
	    logger("Logging at $opt->{loglevel} level.", 2);
    }

	if ($debug<1 or $debug > 5) {
		$debug=2;
		$opt->{loglevel}="warning";
		logger("Log Level not specified, logging at warning level.", 2);
	}

    $opt->{tarballdir}=~s/\/$// if ($opt->{tarballdir}=~/\/$/);

    unless (-d $opt->{tarballdir}) {
        $retval |= ERR_OPTIONS;
        $retval |= ERR_FILE_ACCESS;
        logger("$opt->{tarballdir} is not a directory!", 1);
    }

    exit $retval if $retval;
    
    my $info = {};
    sectionBreak("OS Information");
    logger("Determining OS info", 5);
    $info = determineOS($info);
    sectionBreak("Likewise Version");
    logger("Determining Likewise version", 5);
    $info = getLikewiseVersion($info);
    sectionBreak("Options Passed");
    logger(fileparse($0)." version $ver", 2);
    foreach my $el (keys(%{$opt})) {
        logger("$el = ".&getOnOff($opt->{$el}), 2);
    }
    sectionBreak("Daemon restarts");
    logger("Turning up logging levels", 5);
    $info= changeLogging($info, $opt, "debug");
    logger("Sleeping for 60 seconds to let Domains be found", 2);
    waitForDomain($info, $opt);

    runTests($info, $opt);

    sectionBreak("Daemon restarts");
    logger("Turning logging levels back to normal", 5);
    $info = changeLogging($info, $opt, "normal");
    
    outputReport($info, $opt);
}

=head1 (C)2009, Likewise Software

=head1 Description

usage: likewise-support.pl [tests] [log choices] [options]

  This is the Likewise Software (Open/Enterprise) support tool.
  It creates a log as specified by the options, and creates 
  a gzipped tarball in for emailing to support@likewise.com

  The options are broken into three (3) groups: Tests, Logs,
   and Options.  Any "on/off" flag has a "--no" option available,
   for example: "--nossh" and "--ssh". The "no" obviously
   negates the flag's operation

  Examples:

    likewise-support.pl --ssh --lsassd --nomessages --restart -l likewise.log


=head2 Usage

  Tests to be performed:

    --ssh (flag)
        Test ssh logon interactively and gather logs
    --sshcommand <commandname>
        The command to be run inside of the ssh section.
        By default, it is just "exit"
    --sshuser <name>
        Enter the username to perform ssh tests as,
        instead of asking the person performing the test
    --(no)gpo (flag)
        Perform Group Policy tests
    --(no)users (flag)
        Enumerate all users in the domain
    --(no)groups (flag)
        Enumerate all groups in the domain
    --(no)automounts (flag)
        Capture /etc/lwi_automount in tarball
    --(no)dns (flag)
        DNS lookup tests
    -c --(no)tcpdump (--capture) (flag)
        Capture network traffic using OS default tool
        (tcpdump, nettl, snoop, etc.)
    --capturefile <file>
        The snoop or tcpdump file to write to during
        capture.
    --(no)smb (flag)
        run smbclient against local samba server
        for testing Samba integration.
    --(no)other (flag)
        Pause to allow other tests (interactive logon,
        multiple ssh tests, etc.) to be run and logged.
        Run this flag, then perform other tests in another
        window or console.  This flag allows the tool to
        be used for capturing logging for long-running tests.

  Log choices (some have multiple names, and all are flags):

    --(no)lsassd (--winbindd)
        Gather lsassd debug logs
    --(no)lwiod (--lwrdrd | --npcmuxd)
        Gather lwrdrd debug logs
    --(no)netlogond
        Gather netlogond debug logs
    --(no)gpagentd
        Gather gpagentd debug logs
    --(no)messages
        Gather syslog logs
    --(no)sambalogs
        Gather logs and config for Samba server

  Options:

    -r --(no)restart (flag)
        Cause restart of the Likewise daemons to separate logs
        Will not log Likewise messages to syslog
        Daemons will restart at the start of the support script
        And again at the end.
        --syslog and --restart are mutually exclusive options
    --(no)syslog (flag)
        Allow editing syslog.conf during the debug run if not
        restarting daemons (exclusive of -r)
        Syslog daemon will be reloaded at the start of the
        support script, and again at the end.
        --syslog and --restart are mutually exclusive options
    -v --loglevel {error,warning,info,verbose,debug}
        Changes the logging level of the support tool.
        Has no effect on Likewise daemon logging levels.
         Error skips even section headers, so use with caution.
    -l --logfile <path>
        Choose the logfile to write data to.  This file is
        included in the tarball sent to support.
    -t --tarballdir <path>
        Choose where to create the gzipped tarball of log data
    --tarballext <extension>
        normally ".tar.gz" - if it ends with "gz", then gzip
        will be run against the tarball before finalizing
        the script. Controls tarball file naming.


=head1 Programmer's data

=head2 Data Structures:

$retval is a bitstring used to hold error values.  The small subs at the top ERR_XXX_XXX = { 2;} 
are combined with the existing value via bitwise OR operations. This gives us a cumulative count
of the errors that have happened during the run of the program, and a good exit status if non-0

$debug is used to determine which level to log at.  higher is more verbose, with 5 being the
current max (debug).

$opt contains each of the command-line options passed.
$info contains information about the system the tool is running on.

$opt is a hash reference, with keys as below, grouped for ease of reading (no groups in the hash structure, each key exists at top level):
    (Group: Daemons and logs)
        netlogond
        lsassd
        lwiod
        gpagentd
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
        authtest
    (Group: Extra Info to gather)
        automounts
        tcpdump
    (Group: file locations and options)
        capturefile (string)
        loglevel (string or number)
        logfile (string - loglevel and logfile affect screen output, not commands to daemons)
        tarballdir (string)
        tarballfile (string)
        tarballext (string)
        sshcommand (string in case anything other than ssh auth is requested to be tested.  default is "exit")
        sshuser (string, if not entered, program will prompt for one)
        help

$info is a hash reference, with keys as below. This is a multi-level hash as described below
    OStype (string describing OS: solaris, darwin, linux-rpm, etc.)
    initpath (string, path to init scripts as "/etc/init.d", or launchctl)
    initorder (string, "normal" or "reverse". Normal is "$initpath $daemon start/stop", Reverse is "$initpath start/stop $daemon")
    rcpath (string, path to rc scripts: "/etc/rc.d")
    pampath (string, path to pam files or file, "/etc/pam.d")
    logpath (string, path to log files, "/var/log")
    logfile (string, system's default log, such as "messages" or "system.log" )
    nsfile (string, full path to nsswitchc.conf or other)
    timezonefile (string, path to system's timezone information)
    platform (string, like "i386" or similar, removes anything after a "-" as returned from Config{platform})
    osversion (string, "uname -r" output)
    uname (string, "uname" output)
    logon (string, login name of user who called the tool (sudo does not mask this))
    name (string, real name of user program is running under (root under sudo))
    uid (number, the effective uidNumber of the user running the program)
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
    lw (hash reference to Likewise specific information)
        base (string, normally "/opt/likewise")
        path (string, path to bin dir: "/opt/likewise/bin")
        version (string, version of Likewise (5.2, 5.1, 4.1))
        smblog (string, name of tool to change smb logging level, which changes from 5.1 to 5.2, doesn't exist previously)
        authdaemon (string, name of auth daemon - "lsassd" or "winbindd");
        gpdaemon (string, name of group policy daemon "gpagentd" or "centeris.com-gpagentd")
        dcedaemon (string, name of dcerpc endpoint mapper "dcerpcd" or "centeris.com-dcerpcd")
        netdaemon (string, name of netlogon daemon "netlogond" or undef)
        eventlogd (string, name of eventlog daemon "eventlogd or undef)
        eventfwdd (string, name of event forwarder daemon "eventfwdd" or undef)
        syslogreaper (string, name of syslog reaper daemon "reapsysld" or undef)
        registry (string, name of registry daemon "lwregd" or undef)
    {daemonname} (hash ref, key name may be "netlogond", "lwiod", etc.)
        pid (number, the PID of the daemon launched)
        handle (anonymous ref to the handle used to launch the daemon)
    logedit (hash ref, the file changed if --restart was negated)
        file (findInPath() hash ref)
            info
            path
            dir
            name
            type
            perm
        line (string, the actual line being inserted or commented out)
    tcpdumpfile (string, the location we're storing the tcpdump file we are creationg from the options passed. in $info, rather than $opt, due to centos' funny handling of "pcap" user requiring us to figure out where to store the file)
        
            
findInPath() returns a hash reference with following keys:
    info (array ref, structure from the "stat()" on the file)
    path (string, full path to file)
    dir (string, full path to the dir the file lives in)
    name (string, name of the file itself: "$file->{dir}/$file->{name}" eq "$file->{path}")
    type (character, c,d,f for character, directory, or file)
    perm (character, r,x,w for readable, executable, writable (checked in that order, as writable matters most to our tests. only keep one perm for this test, since stat holds all))
            
=cut
