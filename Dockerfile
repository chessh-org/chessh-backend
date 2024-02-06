FROM debian:stable-slim

RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y build-essential libncursesw5-dev libreadline-dev pkg-config openssh-server supervisor telnetd supervisor telnetd

COPY . /chessh

ADD docker-artifacts/supervisord.conf /etc/supervisor/conf.d/supervisord.conf

RUN mkdir -p /run/sshd

RUN chessh/docker-artifacts/build.sh

ENTRYPOINT /usr/bin/supervisord
