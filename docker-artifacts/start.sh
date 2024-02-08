#!/bin/sh

if ! [ -d /chessh-data/environment ] ; then
	mkdir /chessh-data/environment
fi

chown -R chessh:chessh /chessh-data

exec /usr/bin/supervisord
