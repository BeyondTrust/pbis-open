#!/usr/bin/perl -w

use strict;
use warnings;
use IO::File;
use Getopt::Long;
use File::Basename;

sub main();
main();
exit 0;

# perl -e 'use IO::File; my $fh = new IO::File("Info.plist", "r") or die; my @lines = $fh->getlines(); map { s/PRODUCT/HELO/; } @lines; print join("", @lines);'
# perl -e 'use IO::File; my $fh = new IO::File("Info.plist", "r") or die; my $lines = join("", $fh->getlines()); $lines =~ s/NAME/ENEMY/gm; print $lines;'
# perl -e 'use IO::File; my $fh = new IO::File("Info.plist", "r") or die; my $lines = join("", $fh->getlines()); $lines =~ s/<key>CFBundleName<\/key>\s*<string>[^<]*<\/string>/JOE/gm; print $lines;'
# perl -e 'use IO::File; my $fh = new IO::File("Info.plist", "r") or die; my $lines = join("", $fh->getlines()); $lines =~ s/(<key>CFBundleName<\/key>\s*<string>)[^<]*/\1JOE/gm; print $lines;'
# perl -e 'use IO::File; my $fh = new IO::File("'"$(INFO)"'", "r") or die; my $lines = join("", $fh->getlines()); $lines =~ s/(<key>CFBundleName<\/key>\s*<string>)[^<]*/\1'"$(CF_BUNDLE_NAME)"'/gm; $lines =~ s/(<key>CFBundleExecutable<\/key>\s*<string>)[^<]*/\1'"$(CF_BUNDLE_EXECUTABLE)"'/gm; print $lines;' > "$@"

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
    return "usage: $0 [-n BUNDLE_NAME] [-e BUNDLE_EXECUTABLE] [-v VERSION] [-o OUTPUT_PLIST] INPUT_PLIST\n";
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
                        'shortversion|s=s',
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
