#!/usr/bin/bash

# If we're on M1/M2 hardware, then invoke the cross-arch gdb; otherwise,
# the ordinary gdb.

PREFIX=""

if echo `arch` | grep -n aarch64 > /dev/null; then
    PREFIX=/usr/x86_64-linux-gnu/bin/
fi

${PREFIX}gdb
