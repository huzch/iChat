# 设置基础镜像
FROM ubuntu:22.04
# 设置工作路径
WORKDIR /iChat
RUN mkdir -p log bin conf
# 拷贝执行程序
COPY ./build/nc /bin/
COPY ./build/friend_server ./bin/
# 拷贝执行依赖
COPY ./dep/ /lib/x86_64-linux-gnu/
# 设置启动操作
CMD ./bin/friend_server -flagfile=./conf/friend_server.conf
