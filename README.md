CashCrow
=================================================
COSE451 SW Security - Team Pumping Lemma

<p align="center">
<img width="415" alt="logo_remove" src="https://github.com/FirstCinnamon/CashCrow/assets/25877816/54f086f1-99a0-44be-b57e-a28bd12468d4"> 
</p>

<div align="center">
  
[![MIT license](https://img.shields.io/badge/License-MIT-blue.svg)](https://lbesson.mit-license.org/)
[![Docker](https://badgen.net/badge/icon/docker?icon=docker&label)](https://docker.com/)

</div>

Table of contents
-----------------

* [Introduction](#introduction)
* [Installation](#installation)
* [Team Pumping Lemma](#team-pumping-lemma)


Introduction
------------
CashCrow is a mock stock trading platform. CashCrow is for those who are new to stock trading and hesistant to dive straight into real-world stock market.
Users can buy and sell mock stocks without spending real-world money, which is fun and instructive experience.
Our platform targets being the benign gateway to real stock markets.

CashCrow uses [HTMX](https://htmx.org) for front-end, [CrowCpp](https://crowcpp.org/master/) for back-end and [PostgreSQL](https://www.postgresql.org) for database.


Installation (for x86-64)
------------
1. Download the Dockerfile from the repository or Discord channel.
2. Navigate to the directory where the Dockerfile is located.
3. Build an image with the Dockerfile. (This takes about more than 15 minutes😅)
```bash
docker build -t cashcrowimg .
```
4. Run from the image built with port number 18080.
```bash
docker run -dit -p 18080:18080 --name cashcrow cashcrowimg
```
5. Done!

<br>
※ Note that the web-server and the database have been initialized in a single Dockerfile✨

※ If you are running it on **arm64** architecture, please consult the comment in the Dockerfile.

Team Pumping Lemma
---------------------------

[Seon Woong Yoon](https://github.com/remy2019)(Leader)

[Junho Lee](https://github.com/FirstCinnamon)

[Hyo Jong Bae](https://github.com/bacon8282)

[Edward Minwoo Kim](https://github.com/Eddy-M-K)

[Byeong Jin Oh](https://github.com/obj0311)

