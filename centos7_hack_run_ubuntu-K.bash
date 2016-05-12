#!/bin/bash

# this is REALLY hacky
# CentOS 7.2.1511 offers Boost 1.53 (nothing newer), whose boost::string_ref
# implementation is BROKEN (compile error compiling boost .h file).
# Building K requires Boost 1.54+ (present on Ubuntu 14.04).
#
# Since I can't compile K on CentOS 7 per above, I attempted to run K as built
# on Ubuntu 14.04, on CentOS 7 (via NFS).  This failed, but _only_ with
#
#   k.goodwin@ssjc-eng-c1-stor3:~$ k
#   k: error while loading shared libraries: libpcre.so.3: cannot open shared object file: No such file or directory
#   k.goodwin@ssjc-eng-c1-stor3:~$ ldd $(which k)
#   	linux-vdso.so.1 =>  (0x00007ffdcd4dd000)
#   	libncurses.so.5 => /lib64/libncurses.so.5 (0x00007f1d366d8000)
#   	libtinfo.so.5 => /lib64/libtinfo.so.5 (0x00007f1d364ae000)
#   	libpthread.so.0 => /lib64/libpthread.so.0 (0x00007f1d36291000)
#   	libpcre.so.3 => not found
#   	libm.so.6 => /lib64/libm.so.6 (0x00007f1d35f8f000)
#   	libc.so.6 => /lib64/libc.so.6 (0x00007f1d35bcd000)
#   	/lib64/ld-linux-x86-64.so.2 (0x00007f1d36908000)
#   	libdl.so.2 => /lib64/libdl.so.2 (0x00007f1d359c9000)
#   k.goodwin@ssjc-eng-c1-stor3:~$ sudo yum install pcre
#     . . .
#   Package pcre-8.32-15.el7.x86_64 already installed and latest version
#
# Reading http://www.openfl.org/archive/community/installing-openfl/solved-haxelib-missing-libpcreso3-fedora/
# and having nothing to lose, I tried the following gross hackery, which worked:
# 1. allows K to run on CentOS 7, _and_
# 2. allows K's PCRE-based functions to perform correctly!

pcre_src_so="/usr/lib64/libpcre.so.1"       # this may change as CentOS PCRE lib changes
ubu1404_pcre_so="/usr/lib64/libpcre.so.3"   # this may change as Ubuntu PCRE lib changes

function die { printf %s "${@+$@$'\n'}" >&2 ; exit 1; }

[ -f "$pcre_src_so" ] || die "$pcre_src_so does not exist; have you run sudo yum install pcre ?"
[ $EUID -eq 0 ] || die "$0 must be run as root"
echo "installing xclip" ; yum install -y xclip
echo "creating symlink $ubu1404_pcre_so" ; ln -s "$pcre_src_so" "$ubu1404_pcre_so" || die "symlink creation failed"
echo "ok"
