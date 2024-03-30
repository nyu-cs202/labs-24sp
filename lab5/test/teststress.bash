#!/bin/bash

. test/libtest.bash

gcc test/stressfs.c -o build/stressfs || fail "can't build stressfs binary"

fuse_unmount
recreate_mnt
generate_test_msg
make_fsimg build/msg
fuse_mount

build/stressfs || fail "stressfs panicked"

fuse_unmount
echo "teststress pass"
