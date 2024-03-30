fail() {
	printf '\e[31mfail\e[m: %s\n' "$@"
	fusermount -u /lab5mnt 1>/dev/null 2>/dev/null
	exit 1
}

ok() {
	printf 'ok: %s\n' "$@"
}

generate_test_msg() {
	# Keep synced with static char *msg in fsdriver.c.
	printf 'This is a rather uninteresting message.\n\n' > build/msg
}

fuse_unmount() {
	warn=`fusermount -u /lab5mnt 2>&1`
	notfnd=`echo "$warn" | grep 'not found in /etc/mtab'`
        notfndcims=`echo "$warn" | grep 'Invalid argument'`
        nosuchfile=`echo "$warn" | grep 'No such file or directory'`
	if [ -n "$warn" -a -z "$notfnd" -a -z "$notfndcims" -a -z "$nosuchfile" ]; then
		fail "$warn"
	fi
}

recreate_mnt() {
	rm -f mnt || fail "couldn't remove mnt symlink"
	rm -rf /lab5mnt || fail "couldn't remove /lab5mnt"
	mkdir /lab5mnt || fail "couldn't mkdir /lab5mnt"
	ln -s /lab5mnt mnt || fail "couldn't symlink mnt"
	chmod a+rwx /lab5mnt || fail "couldn't change /lab5mnt permissions"
}

make_fsimg() {
	build/fsformat testfs.img 2048 $@ || fail "couldn't make test image"
}

fuse_mount() {
	build/fsdriver testfs.img mnt $@ || fail "couldn't mount test image"
}
