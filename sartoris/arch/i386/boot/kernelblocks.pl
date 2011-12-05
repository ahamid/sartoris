#!/usr/bin/perl

@Info = stat($ARGV[$1] || "../kernel/kimg");

$size = $Info[7];
$blocks = $size / 512;

print "$blocks";

