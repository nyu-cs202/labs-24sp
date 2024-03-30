#!/bin/bash

. test/libtest.bash

fuse_unmount
recreate_mnt
make_fsimg
fuse_mount

rm -f mnt/hello
echo "hello, world" > mnt/hello
oldmod=`stat --format='%a' mnt/hello`
echo "mode of hello is $oldmod"
chmod 777 mnt/hello
newmod=`stat --format='%a' mnt/hello`
if [ "$newmod" != "777" ]; then
	fail "mode $newmod is not 0777 for hello"
else
	ok "hello mode set to 777"
fi
chmod $oldmod mnt/hello
newmod=`stat --format='%a' mnt/hello`
if [ "$newmod" != "$oldmod" ]; then
	fail "mode $newmod is not $oldmod for hello"
else
	ok "hello mode reset to $oldmod"
fi

fuse_unmount
echo "testmode pass"
