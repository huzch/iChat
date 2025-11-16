#!/bin/bash

# $1:program path
# $2:linking lib storage path
get_dep() {
  dep=$(ldd $1 | awk '{if(match($3,"/")){print $3}}')
  mkdir -p $2
  cp -Lr $dep $2
}

# server dependency
get_dep ../service/speech/build/speech_server ../service/speech/dep/
get_dep ../service/file/build/file_server ../service/file/dep/
get_dep ../service/user/build/user_server ../service/user/dep/
get_dep ../service/forward/build/forward_server ../service/forward/dep/
get_dep ../service/message/build/message_server ../service/message/dep/
get_dep ../service/friend/build/friend_server ../service/friend/dep/
get_dep ../service/gateway/build/gateway_server ../service/gateway/dep/

# nc dependency (entry_point.sh use nc)
get_dep /bin/nc ../service/speech/dep/
get_dep /bin/nc ../service/file/dep/
get_dep /bin/nc ../service/user/dep/
get_dep /bin/nc ../service/forward/dep/
get_dep /bin/nc ../service/message/dep/
get_dep /bin/nc ../service/friend/dep/
get_dep /bin/nc ../service/gateway/dep/

cp /bin/nc ../service/speech/build/
cp /bin/nc ../service/file/build/
cp /bin/nc ../service/user/build/
cp /bin/nc ../service/forward/build/
cp /bin/nc ../service/message/build/
cp /bin/nc ../service/friend/build/
cp /bin/nc ../service/gateway/build/
