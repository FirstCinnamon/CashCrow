## CashCrow(2023 Team Pumping Lemma)
# Last edited on 11/2/2023 00:00
# Note: Update before submission. remove temp segments. resolve cache problem.

FROM ubuntu:16.04
LABEL maintainer="Seonwoong Yoon <remy2019@korea.ac.kr>"

RUN apt-get update \
    && apt-get install git -y \
    && apt-get install g++ -y \
    && apt-get install cmake -y \
    && apt-get install libpq-dev -y \
    && mkdir ./docker
WORKDIR ./docker

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

# TEMPORARY: git clone private repo
RUN git config --global user.name "Seon Woong Yoon" \
    && git config --global user.email "remy2019@gmx.us" \
    && git clone https://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@github.com/"FirstCinnamon/CashCrow.git"
WORKDIR ./CashCrow

# TEMPORARY: for debugging purpose. refer /var/log/syslog
RUN apt-get install -y rsyslog \
    && rsyslogd

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

# Install libpqxx 7.7.4
RUN git clone https://github.com/jtv/libpqxx.git \
    && cd libpqxx \
    && git checkout 7.7.4 \
    && ./configure --disable-shared --disable-documentation CXXFLAGS=-std=c++17 \
    && make \
    && make install \
    && cd ..

# Compile and run
RUN git clone https://github.com/CrowCpp/Crow.git \
    && g++ -std=c++17 main.cpp -lpthread -I./include -I./Crow/include -I./libpqxx/include -L/usr/local/lib -lpqxx -o cashcrow
EXPOSE 18080/tcp
ENTRYPOINT touch /dev/xconsole; chgrp syslog /dev/xconsole; chmod 664 /dev/xconsole; service rsyslog start; cron; ./cashcrow

## Usage:
# 	docker build -t cashcrowimg .
# 	docker run -dit -p 18080:18080 --name cashcrow cashcrowimg
## Useful commands:
# 	docker exec -it cashcrow /bin/bash
# 	docker rmi $(docker images --filter "dangling=true" -q)
# 	docker logs --tail 20 -f cashcrow
