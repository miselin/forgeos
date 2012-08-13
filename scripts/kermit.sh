#!/bin/bash

SCRIPT_PATH=`dirname $0`

KERMIT_TTY=$2

if [ -e $KERMIT_TTY ]; then

    # First argument - uImage location.
    $SCRIPT_PATH/kermit.cmd $1 $KERMIT_TTY

    # Connect to the serial console.
    screen $KERMIT_TTY 115200

else
    echo "tty $KERMIT_TTY isn't usable"
fi
