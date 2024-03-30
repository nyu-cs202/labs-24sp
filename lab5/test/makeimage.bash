#!/bin/bash

. test/libtest.bash
if [ -f "testfs.img" ]; then
    rm testfs.img
fi
make_fsimg build/msg
