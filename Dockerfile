FROM debian:stable-slim

RUN apt-get update && apt-get upgrade -y
RUN apt-get install -y build-essential pkg-config supervisor libcrypt1 libdb-dev

COPY . /chessh

ADD docker-artifacts/supervisord.conf /etc/supervisor/conf.d/supervisord.conf

RUN mkdir -p /run/sshd

RUN chessh/docker-artifacts/build.sh

VOLUME /chessh-data

ENTRYPOINT /chessh/docker-artifacts/start.sh
