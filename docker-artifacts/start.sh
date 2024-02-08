#!/bin/sh

if ! [ -d /chessh-data/environment ] ; then
	mkdir /chessh-data/environment
	chown chessh:chessh /chessh-data
fi

exec /usr/bin/supervisord -c /etc/supervisor/conf.d/supervisord.conf
