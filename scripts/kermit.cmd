#!/usr/bin/kermit +

set line \%2

set speed 115200
set reliable
fast
set carrier-watch off
set flow-control none
set prefixing all

set file type bin

set rec pack 4096
set send pack 4096
set window 5

send \%1

exit
