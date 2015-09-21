FROM debian:jessie
MAINTAINER Christopher Brichford <chrisb@farmersbusinessnetwork.com>

ENV POSTGRES_VERSION=9.4
ENV WEIGHTED_STATS_VERSION=0.1

RUN apt-get update && apt-get install -y apt-transport-https && \
    apt-get clean

COPY docker/jessie/postgres.pub.gpg /etc/apt/
COPY docker/jessie/sources.list /etc/apt/

RUN apt-key add /etc/apt/postgres.pub.gpg && \
    apt-get update && \
    apt-get dist-upgrade -y && \
    apt-get install -y build-essential \
                       devscripts \
                       debhelper \
                       postgresql-server-dev-all \
                       git && \
    apt-get clean

COPY . /build/weighted-stats-0.1/

RUN cd /build/weighted-stats-0.1 && make deb
