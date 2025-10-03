#!/bin/bash

# $1:program path
# $2:linking lib storage path
get_dep() {
  dep=$(ldd $1 | awk '{if(match($3,"/")){print $3}}')
  mkdir -p $2
  cp -Lr $dep $2
}

get_dep ../service/speech/build/speech_server ../service/speech/dep/
get_dep ../service/file/build/file_server ../service/file/dep/
get_dep ../service/user/build/user_server ../service/user/dep/
get_dep ../service/forward/build/forward_server ../service/forward/dep/
get_dep ../service/message/build/message_server ../service/message/dep/
get_dep ../service/friend/build/friend_server ../service/friend/dep/
get_dep ../service/gateway/build/gateway_server ../service/gateway/dep/