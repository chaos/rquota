#!/bin/bash -e

# build-install-lustre.sh - side-install lustre without kernel modules

declare -r prog=${0##*/}
declare -r prefix=$HOME/local
declare -r pkg=https://github.com/LLNL/lustre/archive/2.10.4_1.chaos.tar.gz
declare -r name=lustre

die() { echo -e "$prog: $@"; exit 1; }

mkdir -p ${name}  || die "Failed to mkdir ${name}"
pushd ${name}
  curl -L -O --insecure ${pkg} || die "Failed to download ${pkg}"
  tar --strip-components=1 -xf *.tar.gz || die "Failed to un-tar ${name}"
  sed --in-place -e 's/^sysconfdir=/# sysconfdir=/' config/lustre-build.m4
  sed --in-place -e 's/^rootsbin_/sbin_/' lustre/utils/Makefile.am
  . autogen.sh
  CC=gcc ./configure --disable-modules --prefix=${prefix} \
                     --with-systemdsystemunitdir=${prefix}/etc/systemd \
                     --sysconfdir=${prefix}/etc
  touch undef.h
  make
  make install
popd
