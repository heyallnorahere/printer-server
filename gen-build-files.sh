#!/bin/bash
# gonna use this script with msys too
GENERATOR=$1
# this will screw build times
BUILD_TESTS=OFF
# we are not building ssl, as this should run on a private network
BUILD_SSL=OFF

shift
cmake . -B build -G "$GENERATOR" -DBUILD_TESTS=$BUILD_TESTS -DBUILD_SSL=$BUILD_SSL $@