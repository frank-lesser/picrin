#!/usr/bin/perl

use strict;

print <<EOL;
/**
 *                                !!NOTICE!!
 * This file was automatically generated by mkinit.pl, and includes all of
 * the prelude files required by Picrin. PLEASE DO NOT EDIT THIS FILE, changes
 * will be overwritten the next time the script runs.
 */

#include "picrin.h"

void
pic_init_contrib(pic_state *pic)
{
EOL

foreach my $lib (@ARGV) {
    print "  void pic_nitro_init_$lib(pic_state *);\n";
}

print;

foreach my $lib (@ARGV) {
    print "  pic_nitro_init_$lib(pic);\n";
}

print <<EOL;
}
EOL
