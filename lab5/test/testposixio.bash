#!/bin/bash

. test/libtest.bash

gcc test/posixio.c -o build/posixio || fail "can't build posixio binary"

fuse_unmount
recreate_mnt
generate_test_msg
make_fsimg build/msg
fuse_mount

build/posixio || fail "posixio panicked"

fuse_unmount
echo "testposixio pass"
