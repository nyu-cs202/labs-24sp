#!/bin/bash
set -e
TEST_DIR=$1
if [ -e ${TEST_DIR} ]; then
    echo "Cowardly refusing to change an existing directory ${TEST_DIR}"
    exit 1
fi
# Create base directory.
mkdir ${TEST_DIR}
# Create some files.
touch ${TEST_DIR}/a
touch ${TEST_DIR}/b
touch -t 1912301230.03 ${TEST_DIR}/c
touch -t 1810251600.00 ${TEST_DIR}/d
dd if=/dev/urandom of=${TEST_DIR}/s1 bs=1024 count=1 status=none > /dev/null
dd if=/dev/urandom of=${TEST_DIR}/s2 bs=1024 count=2 status=none > /dev/null
dd if=/dev/urandom of=${TEST_DIR}/s3 bs=1024 count=3 status=none > /dev/null
dd if=/dev/urandom of=${TEST_DIR}/s4 bs=1024 count=4 status=none > /dev/null
mkdir ${TEST_DIR}/in0
mkdir ${TEST_DIR}/in0/in1
mkdir ${TEST_DIR}/in0/in1/in2 
touch -t 1912301230.03 ${TEST_DIR}/in0/a
touch  ${TEST_DIR}/in0/b
touch ${TEST_DIR}/in0/in1/in2/x
touch ${TEST_DIR}/.boo
mkdir ${TEST_DIR}/bad
touch ${TEST_DIR}/bad/bad_user
sudo chown 2002 ${TEST_DIR}/bad/bad_user
touch ${TEST_DIR}/bad/bad_group
sudo chown :2220 ${TEST_DIR}/bad/bad_group
touch ${TEST_DIR}/bad/bad_ugroup
sudo chown 2002:2220 ${TEST_DIR}/bad/bad_ugroup
mkdir ${TEST_DIR}/.hidden
touch ${TEST_DIR}/.hidden/a
touch ${TEST_DIR}/.hidden/b
touch ${TEST_DIR}/.hidden/c

touch ${TEST_DIR}/ungrwx
chmod 070 ${TEST_DIR}/ungrwx
touch ${TEST_DIR}/urwxgn
chmod 700 ${TEST_DIR}/urwxgn
touch ${TEST_DIR}/urwgrwarw
chmod 666 ${TEST_DIR}/urwgrwarw
touch ${TEST_DIR}/urwgrar
chmod 644 ${TEST_DIR}/urwgrar

