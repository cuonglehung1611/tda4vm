#!/bin/bash


docker build --build-arg USER_ID=$(id -u) \
             --build-arg GROUP_ID=$(id -g) \
             --build-arg USER_NAME=$(whoami) \
             -t cuong_work .

