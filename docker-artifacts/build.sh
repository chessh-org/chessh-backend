#!/bin/sh
set -e

make -C /chessh -B
make -C /chessh/src/frontends -B

passwd -d root
useradd -m -d /chessh chessh
echo "chessh:chessh" | chpasswd
echo "Match User chessh" >> /etc/ssh/sshd_config
echo "ForceCommand /chessh/build/chessh-client -u chessh -p chessh -d /chessh-server" >> /etc/ssh/sshd_config

mkdir /chessh-server
chown -R chessh:chessh /chessh-server
