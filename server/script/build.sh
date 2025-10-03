#!/bin/bash

mkdir -p ../service/speech/build
mkdir -p ../service/file/build
mkdir -p ../service/user/build
mkdir -p ../service/forward/build
mkdir -p ../service/message/build
mkdir -p ../service/friend/build
mkdir -p ../service/gateway/build

cp -r ../build/service/speech/* ../service/speech/build
cp -r ../build/service/file/* ../service/file/build
cp -r ../build/service/user/* ../service/user/build
cp -r ../build/service/forward/* ../service/forward/build
cp -r ../build/service/message/* ../service/message/build
cp -r ../build/service/friend/* ../service/friend/build
cp -r ../build/service/gateway/* ../service/gateway/build