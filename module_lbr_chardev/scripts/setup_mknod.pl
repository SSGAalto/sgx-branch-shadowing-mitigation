#Authors: Hans Liljestrand and Shohreh Hosseinzadeh
#Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
#This code is released under Apache 2.0 and GPL 2.0 licenses.

#!/usr/bin/env perl

use strict;
use warnings FATAL => 'all';

our $device_name="lbr_dumper";
our $device_dir="/dev";
our $device_filename="${device_dir}/${device_name}";

sub find_mknod_args {
    my $retval = undef;

    open(my $fh, "dmesg | tail -n 40 |")
        or die "Failed to open dmesg pipe: $!";

    while(<$fh>) {
        if (m/^\s*\[\s*\d+\.\d+\s*]\s+mknod\s+${device_name}\s+(\w+)\s+(\d+)\s+(\d+)\s*$/) {
            $retval = [ $1, $2, $3 ];
        }
    }

    close($fh)
        or die "Failed to close dmesg pipe: $!";

    return $retval;
}

sub main {
    my $args = find_mknod_args;
    if ($args) {
        my $cmd_mknod = sprintf("/bin/mknod %s %s %d %d", ${device_filename}, $args->[0], $args->[1], $args->[2]);
        my $cmd_rm = sprintf("/bin/rm ${device_filename}");

        if (-e ${device_filename}) {
            print "$cmd_rm\n";
            `$cmd_rm`;
        }
        print "$cmd_mknod\n";
        `$cmd_mknod`;
    } else {
        print("Failed to find mknod in dmesg\n");
    }
}

main
