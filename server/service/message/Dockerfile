# 设置基础镜像
FROM ubuntu:22.04
# 设置工作路径
WORKDIR /iChat
RUN mkdir -p log bin conf &&\
    apt update &&\
    apt install -y netcat
# 拷贝执行程序
COPY ./build/message_server ./bin/
# 拷贝执行依赖
COPY ./dep/ /lib/aarch64-linux-gnu/
# 设置启动操作
CMD ./bin/message_server -flagfile=./conf/message_server.conf