#!/bin/sh

cd /chessh
make

passwd -d root
useradd -m -d /guest guest
echo "guest:guest" | chpasswd
echo "Match User guest" >> /etc/ssh/sshd_config
echo "ForceCommand /chessh/build/chessh-client -u guest -p guest -d /chessh-server" >> /etc/ssh/sshd_config

mkdir /chessh-server
chown -R guest:guest /chessh-server
