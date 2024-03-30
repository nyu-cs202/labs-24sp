#!/bin/bash

. test/libtest.bash

fuse_unmount
recreate_mnt
make_fsimg
fuse_mount

touch mnt/file
oldatime=`stat --format='%X' mnt/file`
oldmtime=`stat --format='%Y' mnt/file`
oldctime=`stat --format='%Z' mnt/file`

sleep 1
echo "time is the fire in which we burn" > mnt/file
newmtime=`stat --format='%Y' mnt/file`
if [ "$oldmtime" == "$newmtime" ]; then
	fail "mtime $newmtime was not updated from $oldmtime"
else
	ok "mtime updated to $newmtime from $oldmtime"
fi

sleep 1
cat mnt/file
newatime=`stat --format='%Y' mnt/file`
if [ "$oldatime" == "$newatime" ]; then
	fail "atime $newatime was not updated from $oldatime"
else
	ok "atime updated to $newatime from $oldatime"
fi

sleep 1
chmod a+rx mnt/file
newctime=`stat --format='%Z' mnt/file`
if [ "$oldctime" == "$newctime" ]; then
	fail "ctime $newctime was not updated from $oldctime"
else
	ok "ctime updated to $newctime from $oldctime"
fi

fuse_unmount
echo "testtime pass"
