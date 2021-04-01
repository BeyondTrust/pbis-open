#!/usr/bin/perl
#
# History
# Version 1.0 - initial version, required setting sid and domain in code
# Version 1.5 - allowed setting site name
# Version 2.0 - add flags for dynamic domain/sid/site name. Fix ability to run multiple times on single host
# Version 2.1 - allow setting the computer password during offline join, for better security and to ease automation.

#use warnings;
use strict;
use Carp;
use Getopt::Long;

my $domainname="";
my %domain;
$domain{sid} = "";
$domain{site} = "";
my $password = "";


Getopt::Long::Configure('ignore_case', 'no_auto_abbrev') || die;

my $opt = {};
my $maplist="hosts|services|auto";

my $ok = GetOptions($opt,
        'help|h|?',
        'domain=s',
        'sid=s',
        'site=s',
        'password=s',
        'loglevel|v=s',
   );

$domainname=$opt->{domain} if (defined($opt->{domain}) and $opt->{domain});
$domain{sid}=$opt->{sid} if (defined($opt->{sid}) and $opt->{sid});
$domain{site}=$opt->{site} if (defined($opt->{site}) and $opt->{site});
$password=$opt->{password} if (defined($opt->{password}) and $opt->{password});
if (defined($opt->{help}) and $opt->{help}) {
    print <<EOF

PBIS offline-join.pl - an alternate to "domainjoin-cli" for pre-staged computers.

Use as: offline-join.pl --domain <domain> --sid <sid>
Or set the domainname and SID values on lines 7 and 9.

 Instructions
  1) Join any PBIS system to the target AD domain by hand normally. Gather the domain SID from a "pbis status" output.
  2) On a joined system, prestage the target computer anywhere in AD with "adtool -a new-computer" or with ADUC, choosing "pre-windows 2000 compatible computer".
  3) Enter the DNS domain name on line 7, and the domain SID on line 9. (or use the "--domain <dnsname> --sid <sid>" flags)
  4) On any sytsem with a hostname of 15 or fewer characters, install PBIS normally.
  5) Run the offline-join.pl script - as root - on the target pc INSTEAD of "domainjoin-cli"

Options:
    --domain <dnsname>  = the DNS name of the domain to join
    --sid <SID> = the SID of that domain (get this from your AD team or from the get-status output of a running computer)
    --site <sitename> (optional) = the Site the computer is in, if site lookups are failing.
    --password <computer password> (optional) = the computer password to use during join — note that this MUST already be set correctly on the prestaged computer account! You can use "adtool -a new-computer --password=***" to accomplish this.

EOF
;
}

die "Need a domain name!" unless ($domainname);
my $cmdfh;
my %forest;
my %pbcmd;
my $gRetval=0;
my $gDebug=2;
my $gOutput=\*STDOUT;
my $hostname="";
my $fqdn="";

if (defined($opt->{loglevel})) {
    if ($opt->{loglevel}=~/(\d)/) {
        $gDebug=$opt->{loglevel};
    }
    $gDebug=5 if ($gDebug > 5);
    $gDebug=1 if ($gDebug < 1);
}
sub main() {};
main();
exit $gRetval;

sub ERR_UNKNOWN ()      { 1; } #The goal is to never return this, since that's what AutoSys, HP Opsware, and other tools return for general failure - avoids overload
sub ERR_OPTIONS ()      { 2; }
sub ERR_OS_INFO ()      { 2; } #Options and OS info are "passed in externally", so send back as same error level
sub ERR_ACCESS  ()      { 4; }
sub ERR_FILE_ACCESS ()  { 4; } #"FILE_ACCESS" was used traditionally, been replaced with simple "ACCESS" Keep for backwards compatibility
sub ERR_SYSTEM_CALL ()  { 8; }
sub ERR_DATA_INPUT  ()  { 16; }
sub ERR_LDAP        ()  { 32; }
sub ERR_NETWORK ()      { 64; }
sub ERR_CHOWN   ()      { 256; } #some ancient shells return 128 instead of higher-value returns, so we don't define 128 right now.
sub ERR_STAT    ()      { 512; }
sub ERR_MAP     ()      { 1024; }

sub exit_if_error {
    if ($gRetval != 0) {
        LogError("There was an unrecoverable error, exiting!");
        exit $gRetval;
    }
}

sub main() {
    getSysInfo();
    getAdInfo();
    exit_if_error();
    my $djbasekey='[HKEY_THIS_MACHINE\Services\lsass\Parameters\Providers\ActiveDirectory\DomainJoin';
    if ($domain{site}) {
        #only add these if the site has been set in the CLI.
        if (defined($pbcmd{siteNamePlugin})) {
            addRegValue('[HKEY_THIS_MACHINE\Services\netlogon\Parameters]', "PluginPath", "REG_SZ", $pbcmd{siteNamePlugin});
        }
        addRegValue('[HKEY_THIS_MACHINE\Services\netlogon\Parameters]', "SiteName", "REG_SZ", $domain{site});
        addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\DomainTrust\\'.$domain{nt4Name}.']', "ClientSiteName", "REG_SZ", $domain{site});
        exit_if_error();
        `$pbcmd{lwsm} refresh`;
        `$pbcmd{lwsm} restart netlogon`;
    }
    $password=lc($hostname) unless $password;
    addRegValue($djbasekey."]", "Default", "REG_SZ", $domain{ucDnsName});
    # Do we need DomainTrust info, or can lsass find that on first start?
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\DomainTrust]', "DomainTrustOrder", "REG_MULTI_SZ", $domain{nt4Name});
    exit_if_error();
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\DomainTrust\\'.$domain{nt4Name}.']', "DNSDomainName", "REG_SZ", $domain{dnsName});
    exit_if_error();
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\DomainTrust\\'.$domain{nt4Name}.']', "Flags", "REG_DWORD", 1);
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\DomainTrust\\'.$domain{nt4Name}.']', "ForestName", "REG_SZ", $forest{dnsName});
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\DomainTrust\\'.$domain{nt4Name}.']', "GUID", "REG_SZ", $domain{guid});
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\DomainTrust\\'.$domain{nt4Name}.']', "NetBiosDomainName", "REG_SZ", $domain{nt4Name});
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\DomainTrust\\'.$domain{nt4Name}.']', "SID", "REG_SZ", $domain{sid});
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\DomainTrust\\'.$domain{nt4Name}.']', "TrustAttributes", "REG_DWORD", 0);
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\DomainTrust\\'.$domain{nt4Name}.']', "TrustDirection", "REG_DWORD", 4);
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\DomainTrust\\'.$domain{nt4Name}.']', "TrustFlags", "REG_DWORD", 0x19);
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\DomainTrust\\'.$domain{nt4Name}.']', "TrustMode", "REG_DWORD", 2);
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\DomainTrust\\'.$domain{nt4Name}.']', "TrustType", "REG_DWORD", 2);
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\DomainTrust\\'.$domain{nt4Name}.']', "TrusteeDomainName", "REG_SZ", "");
    exit_if_error();
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\ProviderData]', "ADConfigurationMode", "REG_DWORD", 0);
    exit_if_error();
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\ProviderData]', "Domain", "REG_SZ", $domain{ucDnsName});
    exit_if_error();
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\ProviderData]', "ShortDomain", "REG_SZ", $domain{nt4Name});
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\ProviderData]', "CellDN", "REG_SZ", "");
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\ProviderData]', "ComputerDN", "REG_SZ", "");
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\ProviderData]', "DirectoryMode", "REG_DWORD", 1);
    exit_if_error();
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\Pstore]', "AccountFlags", "REG_DWORD", 1);
    exit_if_error();
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\Pstore]', "DnsDomainName", "REG_SZ", $domain{ucDnsName});
    exit_if_error();
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\Pstore]', "DomainSid", "REG_SZ", $domain{sid});
    exit_if_error();
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\Pstore]', "Fqdn", "REG_SZ", $fqdn);
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\Pstore]', "KeyVersionNumber", "REG_DWORD", 2);
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\Pstore]', "NetbiosDomainName", "REG_SZ", $domain{nt4Name});
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\Pstore]', "SamAccountName", "REG_SZ", uc($hostname)."\$");
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\Pstore]', "UnixLastChangeTime", "REG_DWORD", 0);
    addRegValue($djbasekey.'\\'.$domain{ucDnsName}.'\Pstore\PasswordInfo]', "Password", "REG_SZ", $password);
    exit_if_error();
    `domainjoin-cli configure --enable pam`;
    `domainjoin-cli configure --enable nsswitch`;
    `domainjoin-cli configure --enable --long $domain{dnsName} --short $domain{nt4Name} krb5`;
    `domainjoin-cli configure --enable ssh`;
    addRegValue('[HKEY_THIS_MACHINE\Services\gpagent]', "Autostart", "REG_DWORD", "1");
    `$pbcmd{lwsm} refresh`;
    `$pbcmd{lwsm} restart lsass`;
    `$pbcmd{lwsm} restart gpagent`;
}

sub getSysInfo {
    #later we'll auto-detect PBIS from pbis-support code
    $pbcmd{regshell} = "/opt/pbis/bin/regshell";
    $pbcmd{status} = "/opt/pbis/bin/get-status";
    $pbcmd{dcname} = "/opt/pbis/bin/get-dc-name";
    $pbcmd{lwsm} = "/opt/pbis/bin/lwsm";
    PLUGIN: for my $testfile ("/opt/pbis/lib64/netlogon/SiteName.so", "/opt/pbis/lib/netlogon/SiteName.so", "/opt/pbis/lib32/netlogon/SiteName.so" ) {
        LogDebug("Testing file path: $testfile.");
        if ( -f $testfile) {
            LogInfo("Found Netlogon SiteName plugin at: $testfile.");
            $pbcmd{siteNamePlugin}=$testfile;
            last;
        }
    }
    LogVerbose("PBIS paths set up properly.");
    $hostname=`hostname`;
    chomp $hostname;
    foreach my $name (gethostbyname($hostname)) {
        if ($name=~/\./) {
            $fqdn=$name;
            last;
        }
    }
    if ($fqdn eq "") {
        LogError("Somehow got a non-FQDN hostname of: $fqdn from $hostname, this won't work for PBIS!");
        LogError("To fix, run 'domainjoin-cli setname $hostname.$domainname' and retry this script.");
        $gRetval|=ERR_OS_INFO;
    }
    return $gRetval;
}

sub getAdInfo {
    my $command="$pbcmd{dcname} $domainname";
    if ($domain{site}) {
        $command.=" --site $domain{site}";
    }
    open ($cmdfh, "$command |");
    while (<$cmdfh>) {
        chomp;
        LogDebug("get-dc-name output line: $_");
        /DnsForestName =\s+(.*)$/;
        $forest{dnsName}=$1 if ($1);
        next if ($1);
        /pszDomainControllerName =\s+(.*)$/;
        $domain{dcName} = $1 if ($1);
        next if ($1);
        /pszNetBIOSDomainName =\s+(.*)$/;
        $domain{nt4Name} = $1 if ($1);
        next if ($1);
        /pszFullyQualifiedDomainName =\s+(.*)$/;
        $domain{dnsName} = $1 if ($1);
        next if ($1);
        /pucDomainGUID.*=\s+(.*)$/;
        $domain{guid} = GuidToString($1) if ($1);
        next if ($1);
        /pszClientSiteName.*=\s_(.*)$/;
        $domain{site} = $1 if (defined($1) and not $1=~/EMPTY/);
    }
    close $cmdfh;
    if ($? != 0 ) {
        LogError("Could not determine all AD information properly, exiting!");
        $gRetval |= ERR_LDAP;
        return;
    }

    LogDebug("Forest = $forest{dnsName}");
    LogDebug("nt4=$domain{nt4Name}");
    LogDebug("Domain = $domain{dnsName}");
    LogDebug("Domain DC = $domain{dcName}");
    LogDebug("Domain GUID = $domain{guid}");
    if (not (defined($domain{dnsName}) and defined($domain{nt4Name}) and defined($domain{dcName}) and defined($forest{dnsName}) and defined($domain{guid}))) {
        LogError("Could not determine all AD information properly, exiting!");
        $gRetval |= ERR_LDAP;
    }
    $domain{ucDnsName} = uc($domain{dnsName});
    $forest{ucDnsName} = uc($forest{dnsName});
    LogVerbose("AD Info gathered.");
}

sub addRegValue {
    my $key = shift;
    my $valName = shift;
    my $valType = shift;
    my $value = shift;
    my $keyfh;
    my $cmd;
    unless ($key and $valName and $valType and defined($value)) {
        LogError("No key, value name, type, or data passed!");
        $gRetval |= ERR_OPTIONS;
        confess $gRetval;
    }
    LogInfo("Checking if '$key' exists, if so, letting it get created...");
    my $error = createRegKeys($key);
    if ($error ne 1 and $error ne 0) {
        LogError("Something went disastrous checking/creating the parent keys!");
        $gRetval |= ERR_SYSTEM_CALL;
        return 0;
    }
    LogInfo("Checking if '$key' \"$valName\" already has data.");
    my @data;
    @data = getRegValue($key, $valName);  # this will return "something" if it's defined, even an empty string, will return undef if not.
    if ($#data>0) {
        $cmd = "$pbcmd{regshell} set_value '$key' \"$valName\" \"$value\" 2>&1|";
        LogInfo("Running: $cmd");
      open($keyfh, $cmd);
        while (<$keyfh>) {
            LogDebug($_);
        }
        close $keyfh;
        if ($? ne 0 ) {
            LogError("Failed to set value for: $key! exiting!");
            $gRetval |= ERR_SYSTEM_CALL;
            return 0;
        }
        return 1;
    } else {
        $cmd = "$pbcmd{regshell} add_value '$key' \"$valName\" $valType \"$value\" 2>&1|";
        open($keyfh, $cmd);
        LogInfo("Running: $cmd");
        while (<$keyfh>) {
            LogDebug($_);
        }
        close $keyfh;
        if ($? ne 0 ) {
            LogError("Failed to add value for: $key! exiting!");
            $gRetval |= ERR_SYSTEM_CALL;
            return 0;
        }
        return 1;

    }
}

sub createRegKeys {
    my $key = shift;
    my $keyfh;
    unless ($key) {
        LogError("Need a registry key!");
        $gRetval |= ERR_OPTIONS;
        exit $gRetval;
    }
    LogDebug("Checking if '$key' exists?");
    open($keyfh, "$pbcmd{regshell} list_keys '$key' 2>&1|");
    while (<$keyfh>){
        LogDebug("$_");
    };
    close $keyfh;
    if ( $? ne 0 ) {
        LogVerbose("'$key' does not exist, recursing upwards to create the tree.");
        my @parts=split(/\\/, $key);
        pop(@parts);
        my $pkey = join('\\', @parts)."]";
        LogDebug("Recursion: Trying to add new key: $pkey");
        my $error = createRegKeys($pkey);
        if ($error == 1 ) {
            LogVerbose("Parent key '$pkey' exists, creating '$key'...");
            open($keyfh, "$pbcmd{regshell} add_key '$key' 2>&1|");
            while (<$keyfh>){
                LogDebug("$_");
            };
            close $keyfh;
            if ($? ne 0 ) {
                LogError("Could not add key '$key', exiting!");
                $gRetval |= ERR_SYSTEM_CALL;
                exit $gRetval;
            }
            return 1;  #have to return 1 here, so recursion returns properly up the calling stack
        }
    } else {
        LogDebug("Found that key: '$key' exists, returning 1;");
        return 1;
    }
    return 0;
}

sub getRegValue {
    my $key = shift;
    my $valName = shift;
    my $keyfh;
    my ($retval, $retType);
    LogDebug("Reading values for '$key'...");
    open($keyfh, "$pbcmd{regshell} list_values '$key' 2>&1 |");
    while (<$keyfh>) {
        chomp;
        LogDebug("$_");
        /\s+"$valName"\s+REG_([A-Z_0-9\[\]]+)\s+(.*)$/;
        next unless $1;
        if ($1 eq "SZ") {
            #it's a string, easy!
            $retval=$2;
            $retType = $1;
            $retval=~s/^"//;
            $retval=~s/"$//;
            LogVerbose("Returning registry type: $retType with value: $retval");
            return ($retType, $retval);
        } if ($1 eq "DWORD") {
            # it's a number, let's take just the HEX
            $retType = $1;
            $retval = $2;
            $retval =~ s/\(\d+\)$//;
            LogVerbose("Returning registry type: $retType with value: $retval");
            return ($retType, $retval);
        } if ($1=~/MULTI_SZ/) {
            # it's a multi-string, so we need to return an array ref.
            $retType=$1;
            if (not defined($retval)) {
                $retval=[];
            }
            push(@{$retval}, $2);
        } else {
            LogError("Data type $1 not handled yet!");
            $gRetval |= ERR_DATA_INPUT;
            exit $gRetval;
        }
    }
    close $keyfh;
    if (defined($retval)) {
        return($retType, $retval);
    }
    return undef;
}
sub GuidToString
{
    my $stringGUID=shift;
    $stringGUID=~s/\s+//g;
    $stringGUID=~s/^(\w{8})(\w{4})(\w{4})(\w{4})(.*)$/$1-$2-$3-$4-$5/;
    return lc $stringGUID;
#    my $stringGUID = unpack("H*", shift);
#    $stringGUID =~ s/^(\w\w)(\w\w)(\w\w)(\w\w)(\w\w)(\w\w)(\w\w)(\w\w)(\w\w\w\w)}/$4$3$2$1-$6$5-$8$7-$9-/;
#    return $stringGUID;
}

sub sid2string {
    my ($sid) = @_;
    my ($revision_level, $authority, $sub_authority_count, @sub_authorities) = unpack 'C Vxx C V*', $sid;
    die if $sub_authority_count != scalar @sub_authorities;
    my $string = join '-', 'S', $revision_level, $authority, @sub_authorities;
    LogVerbose("sid    = " . join( '\\', unpack '(H2)*', $sid ));
    LogVerbose("string = $string");
    return $string;
}

sub LogData($;$$) {
    my $line = shift;
    my $debug = shift;
    my $output = shift;
    Logger($line, 1, $debug, $output);
}

sub LogDebug($;$$) {
    my $line = shift;
    my $debug = shift;
    my $output = shift;
    $line = "DEBUG   : $line";
    Logger($line, 5, $debug, $output);
}

sub LogVerbose($;$$) {
    my $line = shift;
    my $debug = shift;
    my $output = shift;
    $line = "VERBOSE : $line";
    Logger($line, 4, $debug, $output);
}

sub LogInfo($;$$) {
    my $line = shift;
    my $debug = shift;
    my $output = shift;
    $line = "INFO    : $line";
    Logger($line, 3, $debug, $output);
}

sub LogWarning($;$$) {
    my $line = shift;
    my $debug = shift;
    my $output = shift;
    $line = "WARNING : $line";
    Logger($line, 2, $debug, $output);
}

sub LogError($;$$) {
    my $line = shift;
    my $debug = shift;
    my $output = shift;
    $line = "ERROR   : $line";
    my $error = print STDERR "$line\n" unless ($output=\*STDOUT or $gOutput=\*STDOUT);
#    confess "ERROR: Can't write to STDERR! $!\n" unless $error;
    Logger($line, 1, $debug, $output);
}

sub Logger($$;$$) {
    # adds "ERROR:, VERBOSE: or DEBUG:" to appropriate line levels for
    # easier parsing of logfile later.
    my $line=shift; # now ok to pass empty line to Logger ||confess "ERROR: No line to log passed to Logger!\n";
    my $level=shift ||confess "ERROR: No verbosity level passed to Logger!\n";
    my $debug = shift;
    my $output = shift;

    $output = $gOutput unless ($output);
    $debug = $gDebug unless ($debug);


    unless (ref($output) eq "GLOB") {
        #print "Redefinding output from $output\n";
        $output = \*STDOUT;
        #print "Redefinding output to $output\n";
    } elsif (not defined($output) or (not $output)) {
        #print "Redefinding output from $output in 'else'\n";
        $output = \*STDOUT;
        #print "Redefinding output to $output in 'else'\n";
    }
    unless (defined($debug) && $debug) {
        #print "Redefinding debug from $debug\n";
        $debug = 3;
        #print "Redefinding debug to $debug\n";
    } elsif (not defined($debug) or (not $debug)) {
        #print "Redefinding debug from $debug in 'else'\n";
        $debug = 3;
        #print "Redefinding debug to $debug in 'else'\n";
    }


    return if ($level>$debug);

    $line = " " if (not defined($line));

    my $error=0;
    chomp $line;
#    if ($level<=1) {
#        $error = print STDERR "$line\n";
#        die "ERROR: Can't write to STDERR! $!\n" unless $error;
#    } else {
    $error = print $output "$line\n";
    die "ERROR: Can't write to output file!! $!\n" unless $error;
#    }
#    if ($output != \*STDOUT && $level != 1) {
#        print "$line\n";
#    }
    return ;
}
