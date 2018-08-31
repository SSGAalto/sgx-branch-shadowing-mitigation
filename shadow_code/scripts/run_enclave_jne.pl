#! /usr/bin/env perl
# Author: Hans Liljestrand, Shohreh Hosseinzadeh
# Copyright: Secure Systems Group, Aalto University https://ssg.aalto.fi/
# This code is released under Apache 2.0 license

package main;

use strict;
use warnings;

use Fcntl qw(SEEK_END);
use Term::ANSIColor;

our $APP= "app_hw";
our $MAX_TEST_NUM = 13;
our $DEFAULT_ITERATIONS = 100;

sub main {
    my $count = $_[0] || $DEFAULT_ITERATIONS;
    my $test_num = $_[1] || -1;
    my @args = @_;
    shift @args;

    if (not -e $APP) {
        print("Cannot find executable $APP\n");
        exit(1);
    }

    if ($test_num > 0) {
        while (my $test_num = shift @args) {
            if ($test_num > $MAX_TEST_NUM) {
                print("Bad test number $test_num");
                exit(1);
            }
            run_test($test_num, $count);
        }
        return;
    }

    for (my $i = 0; $i < $MAX_TEST_NUM; $i++) {
        run_test($i+1, $count);
    }
}

sub run_once {
    my $test_num = shift;
    my $victim_input = shift;
    my $shadow_input = shift;
    my $hit = 0;
    my $is_following = 0;
    my $time = -1;
    my $following_cycles = -1;
    my $not_found = 1;

    my $addr = `./$APP -t $test_num -m -v $victim_input -s $shadow_input`;
    $! and die "Unable to execute $APP $!";

    if ($addr !~ m/^[0123456789abcdef]{8,16}$/) {
        die qq|"Bad address "$addr" from $APP|;
    }

    if (not $addr) {
        print("Something wen't wrong, no address!?!\n");
        exit(-1);
    }

    open(my $fh, "dmesg | tail -n 40 |")
        or die "Failed to open dmesg pipe: $!";

    while(<$fh>) {
        if (m/\[\s*\d+\.\d+\]\s+0x0*([0123456789abcdef]+)\s+.*cycles:\s+(\d+)/) {
            # if (m/\[\s*\d+\.\d+\]\s+(0x0*$addr).*cycles:\s+(\d+)/) {
            # print "$addr == $1\n";
            if ($addr eq $1) {
                $time = $2;
                $hit = ($_ !~ m/MISS/);
                $not_found = 0;
                $is_following = 1;
            }
            elsif ($is_following) {
                $is_following = 0;
                $following_cycles = $2;
            }
        }
    }

    #$not_found and warn "missed an entry (for 0x$addr )";

    close($fh)
        or die "Failed to close dmesg pipe: $!";

    return {
        "input"     => $victim_input,
        "hit"       => $hit,
        "not_found" => $not_found,
        "cycles"    => $time,
        "fcycles"   => $following_cycles
    };
}

sub collect_results {
    my $test_num = shift;
    my $si = shift;
    my $total_count = shift;
    my $res = shift;

    my @output = ();

    for (my $i = 0; $i < 2; $i++) {
        my $hits = 0;
        my $not_found = 0;
        my $cycles = 0;
        my $fcycles = 0;
        my $count = 0;
        my $fcount = 0;

        for my $r (@$res) {
            if ($r->{input} == $i) {
                if ($r->{not_found}) {
                    $not_found++;
                }
                else {
                    $hits += $r->{hit};
                    $cycles += $r->{cycles};
                    if ($r->{fcycles} >= 0) {
                        $fcycles += $r->{fcycles};
                        $fcount++;
                    }
                    $count++;
                }
            }
        }

        my $avg_hits = ($count != 0 ? $hits / $count : 0);
        my $avg_cycles = ($count != 0 ? $cycles / $count : 0);
        my $avg_fcycles = ($fcount != 0 ? $fcycles / $fcount : 0);

        my $c_stdd = 0;
        my $f_stdd = 0;
        my $h_stdd = 0;

        for my $r (@$res) {
            if ($r->{input} == $i) {
                $c_stdd += (($r->{cycles} - $avg_cycles) ** 2);
                $f_stdd += (($r->{fcycles} - $avg_fcycles) ** 2);
                $h_stdd += ($r->{hit} - $avg_hits) ** 2;
            }
        }

        $c_stdd = sqrt($count > 2 ? $c_stdd / $count : 0);
        $f_stdd = sqrt($fcount > 2 ? $f_stdd / $fcount : 0);
        $h_stdd = sqrt($count > 2 ? $h_stdd / $count : 0);

        push @output, {
            test_num => $test_num,
            v_input => $i,
            not_found => $not_found,
            hits => $hits,
            count => $count,
            h_avg => $avg_hits,
            h_std => $h_stdd,
            c_avg => $avg_cycles,
            c_std => $c_stdd,
            f_avg => $avg_fcycles,
            f_std => $f_stdd
        }
    }

    return \@output;
}

sub get_diff_mean {
    my $r = shift;
    my $type = shift;

    return $r->[0]->{$type} - $r->[1]->{$type};
}

sub get_diff_stddev {
    my $r = shift;
    my $type = shift;

    if ($r->[0]->{count} == 0 || $r->[1]->{count} == 0) {
        return 999999;
    }

    return sqrt(
        ($r->[0]->{$type} / $r->[0]->{count})
        +
        ($r->[1]->{$type} / $r->[1]->{count})
    );
}

sub get_val_color {
    my $m = shift;
    my $s = shift;
    my $d = shift;

    my $retval =  (abs($m) > $s*2 ? 'red' :
        (abs($m) > $s ? 'blue' : $d));

    # print "m=$m, s=$s, returning $retval\n";
    return $retval;
}

sub print_res {
    my $r = collect_results(@_);

    my $h_diff_mean = get_diff_mean($r, "h_avg");
    my $c_diff_mean = get_diff_mean($r, "c_avg");
    my $f_diff_mean = get_diff_mean($r, "f_avg");
    my $h_diff_stddev = get_diff_stddev($r, "h_std");
    my $c_diff_stddev = get_diff_stddev($r, "c_std");
    my $f_diff_stddev = get_diff_stddev($r, "f_std");

    for my $t (@$r) {
        my $line_color = ($t->{v_input} % 2 == 0 ? 'grey23' : 'grey15');
        my $hit_color = get_val_color($h_diff_mean, $h_diff_stddev, $line_color);
        my $cycle_color = get_val_color($c_diff_mean, $c_diff_stddev, $line_color);
        my $fcycle_color = get_val_color($f_diff_mean, $f_diff_stddev, $line_color);

        print colored([$line_color], sprintf(
            "test %-2d, v_input %d (not found: %-4d): %4d/%-4d ---> ",
            $t->{test_num}, $t->{v_input}, $t->{not_found}, $t->{hits}, $t->{count}));
        print colored([$hit_color], sprintf("%0.2f (%0.2f)", $t->{h_avg}, $t->{h_std}));
        print colored([$line_color], " avg_cycles: ");
        print colored([$cycle_color], sprintf("%03.0f (%3.0f)", $t->{c_avg}, $t->{c_std}));
        print colored([$line_color], " avg_fcycles: ");
        print colored([$fcycle_color], sprintf("%03.0f (%3.0f)", $t->{f_avg}, $t->{f_std}));
        print "\n";
    }
}

sub run_test_with_shadow_input {
    my $test_num = shift;
    my $count = shift;
    my $shadow_input = shift;

    my @res = ();
    for (my $i = 0, my $ones = $count, my $zeros = $count; $i < 2*$count; $i++) {
        # Ugly(?) hack to randomize test run order...
        # Our attack should ideally prevent this by manipulating BTB/predictor
        # before attack, but until we get that working this might serve by adding
        # some confusion.
        my $input = (int(rand(2)) ?
            ($ones-- > 0 ? 1 : 0) :
            ($zeros-- > 0 ? 0 : 1));
        push @res,run_once($test_num, $input, $shadow_input);
    }
    print_res($test_num, $shadow_input, $count, \@res);
}

sub run_test {
    my $test_num = shift;
    my $count = shift;

    # Pointless with input 0, sice the shadow doesn't take any jumps!
    # run_test_with_shadow_input($test_num, $count, 0);
    run_test_with_shadow_input($test_num, $count, 1);
}

main(@ARGV);



