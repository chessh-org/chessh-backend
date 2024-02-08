#!/bin/sh
set -e

make -C /chessh -B
make -C /chessh/src/frontends -B

passwd -d root
useradd chessh

mkdir /chessh-server
chown chessh:chessh /chessh-server
