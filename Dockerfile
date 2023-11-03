## CashCrow(2023 Team Pumping Lemma)
# Last edited on 11/2/2023 00:00
# Note: Update before submission. remove temp segments. resolve cache problem. remove redundant g++

FROM ubuntu:16.04
LABEL maintainer="Seonwoong Yoon <remy2019@korea.ac.kr>"
ARG DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Seoul

ARG STEP1=true 
RUN apt-get update \
    && apt-get install tzdata -y \
    && apt-get install git -y \
    && apt-get install g++ -y \
    && apt-get install cmake -y \
    && apt-get install libpq-dev -y \
    && mkdir ./docker
WORKDIR ./docker

ARG STEP2=true 
# Install Python3.9.7
RUN apt-get update \
    && apt-get install -y build-essential zlib1g-dev libncurses5-dev libgdbm-dev libnss3-dev libssl-dev libreadline-dev libffi-dev wget \
    && wget https://www.python.org/ftp/python/3.9.7/Python-3.9.7.tgz \
    && tar -zxvf Python-3.9.7.tgz \
    && cd Python-3.9.7 \
    && ./configure --prefix=/usr/local/python3 \
    && make && make install \
    && ln -sf /usr/local/python3/bin/python3.9 /usr/bin/python3 \
    && ln -sf /usr/local/python3/bin/pip3.9 /usr/bin/pip3 \
    && pip3 install pricegenerator \
    && cd -

ARG STEP3=true 
# Install g++9
RUN apt-get update -y && \
apt-get upgrade -y && \
apt-get dist-upgrade -y && \
apt-get install build-essential software-properties-common -y && \
add-apt-repository ppa:ubuntu-toolchain-r/test -y && \
apt-get update -y && \
apt-get install gcc-9 g++-9 -y && \
update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-9 60 --slave /usr/bin/g++ g++ /usr/bin/g++-9 && \
update-alternatives --config gcc

ARG STEP4=true 
# TEMPORARY: git clone private repo
RUN git config --global user.name "Seon Woong Yoon" \
    && git config --global user.email "remy2019@gmx.us" \
    && git clone https://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@github.com/"FirstCinnamon/CashCrow.git"
WORKDIR ./CashCrow

# TEMPORARY: for debugging purpose. refer /var/log/syslog
RUN apt-get install -y rsyslog \
    && rsyslogd

ARG STEP5=true 
# Price generation initialization
# Generate initial prices
RUN cd price \
    && sh ./compile.sh \
    && sh ./price_gen_daily.sh \
    && sh ./price_gen_minutely.sh \
    && cd ..
# Config crontab for periodic price generation
RUN apt-get install cron -y \
    && cd price \
    && cp cron-jobs /etc/cron.d/cron-jobs \
    && chmod 0644 /etc/cron.d/cron-jobs \
    && crontab /etc/cron.d/cron-jobs \
    && cron \
    && cd ..

ARG STEP6=true 
# Install libpqxx 7.7.4
RUN git clone https://github.com/jtv/libpqxx.git \
    && cd libpqxx \
    && git checkout 7.7.4 \
# if you are using arm64: ./configure --disable-shared --disable-documentation CXXFLAGS=-std=c++17 --build=aarch64-unknown-linux-gnu
    && ./configure --disable-shared --disable-documentation CXXFLAGS=-std=c++17 \
    && make \
    && make install \
    && cd ..

ARG STEP7=true 
# Install PostgreSQL 13
RUN apt-get install apt-transport-https ca-certificates -y
RUN apt-get install sudo -y \
    && touch /etc/apt/sources.list.d/pgdg.list \
    && echo "deb http://apt-archive.postgresql.org/pub/repos/apt/ xenial-pgdg main" | tee /etc/apt/sources.list.d/pgdg.list \
    && wget --quiet -O - https://www.postgresql.org/media/keys/ACCC4CF8.asc | apt-key add - \
    && apt-get update \
    && apt-get install postgresql -y

ARG STEP8=true 
# Compile and run
# Install g++-7(7.5.0) for final compilation
RUN apt-get install g++-7 -y
RUN git clone https://github.com/CrowCpp/Crow.git \
    && g++-7 -std=c++17 main.cpp -z execstack -fno-stack-protector -z norelro -g -O0 -lpthread -I./include -I./Crow/include -I./libpqxx/include -L/usr/local/lib -lpqxx -Llibs -lpq -o cashcrow
EXPOSE 18080/tcp
ENTRYPOINT touch /dev/xconsole; chgrp syslog /dev/xconsole; chmod 664 /dev/xconsole; service rsyslog start; service postgresql start; cp db/init.sql /var/lib/postgresql/init.sql; runuser -l postgres -c 'createdb crow; psql -U postgres -d crow -a -f init.sql'; cron; ./cashcrow

## Usage:
# 	docker build -t cashcrowimg .
# 	docker run -dit -p 18080:18080 --name cashcrow cashcrowimg
## Useful commands:
# 	docker exec -it cashcrow /bin/bash
# 	docker rmi $(docker images --filter "dangling=true" -q)
# 	docker logs --tail 20 -f cashcrow
#	docker build --build-arg STEP4=false -t cashcrowimg .
#	inside container: sudo -i -u postgres
#	inside container: psql
