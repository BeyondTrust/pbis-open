#!/usr/bin/perl

$datecmd = "date +%s.%N";

$i = 10000;


while($i > 4) {

    $starttime = `$datecmd` + 0;

    #print "starttime = " . $starttime . "\n";

    $cmd = "./listevents " . $i . " >/dev/null 2>/dev/null";

    system($cmd);

    $endtime = `$datecmd` + 0;

    $elapsedtime = $endtime - $starttime;

    $i = ($i * 3) / 4;

    $i = sprintf("%d", $i);

    print "i," . $i . ",time_elapsed," . $elapsedtime . "\n";

}
