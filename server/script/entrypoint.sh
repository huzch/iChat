#!/bin/bash

# 解析脚本执行参数
while getopts "h:p:c:" arg
do
  case $arg in
    h)
      ip=$OPTARG;;
    p)
      ports=$OPTARG;;
    c)
      cmd=$OPTARG;;
  esac
done

# $1:ip
# $2:port
wait_for() {
  while ! nc -z $1 $2
  do
    echo "$2 端口连接失败";
    sleep 1;
  done
  echo "$2 端口连接成功";
}

# 由于shell中数组是以空格分割的字符串，所以将逗号","替换为空格" "
for port in ${ports//,/ }
do
  wait_for $ip $port
done

# 将字符串当作命令执行
eval $cmd