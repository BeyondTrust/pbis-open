#!/usr/bin/perl -w

# NOTE: Canonical copy in build/mac-makefile-template/.

use strict;
use warnings;
use IO::File;
use Getopt::Long;
use File::Basename;

sub main();
main();
exit 0;

sub ReadFile($)
{
    my $file = shift || die;
    my $fh = new IO::File("<$file") or die "Failed to open $file: $!\n";
    return $fh->getlines();
}

sub WriteFile($$)
{
    my $file = shift || die;
    my $data = shift || die;

    my $fh = new IO::File(">$file") or die "Failed to create $file: $!\n";
    print $fh $data;
    $fh->close();
}

sub GetKeyValueString($$)
{
    my $lines = shift || die;
    my $keyName = shift || die;

    my $keyValue = undef;
    if ($lines =~ /(<key>$keyName<\/key>\s*<string>)([^<]*)/m)
    {
        $keyValue = $2;
    }

    return $keyValue;
}

sub ReplaceKeyValueString($$$)
{
    my $lines = shift || die;
    my $keyName = shift || die;
    my $keyValue = shift || die;

    $lines =~ s/(<key>$keyName<\/key>\s*<string>)[^<]*/$1$keyValue/gm;

    return $lines;
}


sub Usage()
{
    $0 = basename($0);
    return <<DATA;
usage: $0 [options] INPUT_PLIST_FILE

    Process an input plist file for use as Contents/Info.plist file of a Mac
    application, bundle, or plugin.  The processing replaces existing values
    with values that are passed in as options and can also generate a
    Contents/PkgInfo file.

  options:

    --name, -n BUNDLE_NAME -- Replace the CFBundleName property.

    --executable, -e BUNDLE_EXECUTABLE -- Replace the CFBundleExecutable
            property.

    --version, -v VERSION -- Replace the CFBundleVersion and
            CFShortVersionString properties.

    --genpkginfo PKGINFO_FILE -- Create a PkgInfo file.

    --output, -o PLIST_FILE -- Output to the desired plist file instead
            of stdout.

  examples:

    $0 -v 5.3 -o MyApp.app/Contents/Info.plist Info.plist
    $0 -v 5.3 -o MyApp.app/Contents/Info.plist --genpkginfo MyApp.app/Contents/PkgInfo Info.plist

DATA
}

sub main()
{
    my $opt = {};
    my $ok = GetOptions($opt,
                        'help',
                        'genpkginfo=s',
                        'name|n=s',
                        'executable|e=s',
                        'version|v=s',
                        'output|o=s');
    if (!$ok || $opt->{help})
    {
        die Usage();
    }

    my $file = shift @ARGV;

    my $error = '';
    if ($#ARGV != -1)
    {
        $error .= "Too many arguments.\n";
    }
    if (not defined $file)
    {
        $error .= "Missing input plist file.\n";
    }

    if ($error)
    {
        die $error.Usage();
    }

    my $lines = join("", ReadFile($file));

    if ($opt->{genpkginfo})
    {
        my $pkgType = GetKeyValueString($lines, 'CFBundlePackageType');
        my $signature = GetKeyValueString($lines, 'CFBundleSignature');

        WriteFile($opt->{genpkginfo}, "$pkgType$signature");
    }

    # Map plist keys to command-line options.
    # See http://developer.apple.com/documentation/MacOSX/Conceptual/BPRuntimeConfig/Articles/PListKeys.html
    my $keys =
    {
     CFBundleName => 'name',
     CFBundleExecutable => 'executable',
     CFBundleVersion => 'version', # one or more intergers separated by periods
     CFBundleShortVersionString => 'version',  # three integers separated by periods
    };

    foreach my $key (sort keys %$keys)
    {
        if (defined $opt->{$keys->{$key}})
        {
            $lines = ReplaceKeyValueString($lines, $key, $opt->{$keys->{$key}});
        }
    }

    if ($opt->{output})
    {
        WriteFile($opt->{output}, $lines);
    }
    else
    {
        print $lines;
    }
}
