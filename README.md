# iChat - 分布式即时通讯系统

一个基于微服务架构的即时通讯系统，支持文字、语音、图片等多种消息类型，提供用户管理、好友关系、消息转发等完整功能。

## 项目架构

### 技术栈

- **编程语言**: C++11+
- **RPC框架**: Apache brpc
- **数据库**: MySQL (ODB ORM), Redis, Elasticsearch
- **消息队列**: RabbitMQ
- **服务注册与发现**: etcd
- **第三方服务**: 百度AI SDK (语音识别、OCR等)
- **容器化**: Docker, Docker Compose

### 微服务模块

```
server/
├── service/
│   ├── speech/          # 语音识别服务
│   ├── file/            # 文件存储服务
│   ├── user/            # 用户管理服务
│   ├── forward/         # 消息转发服务
│   ├── message/         # 消息存储服务
│   ├── friend/          # 好友关系服务
│   └── gateway/         # 网关服务 (HTTP/WebSocket)
├── common/              # 公共组件
├── proto/               # Protobuf协议定义
├── odb/                 # 数据库实体定义
└── third/               # 第三方依赖
```

## 核心功能

### 用户服务 (user_server.hpp)
- 用户注册/登录
- 手机验证码验证
- 用户信息管理 (昵称、头像、签名等)
- 用户搜索

### 好友服务 (friend_server.hpp)
- 好友申请/处理
- 好友列表管理
- 好友关系存储

### 消息服务 (message_server.hpp)
- 消息持久化
- 历史消息查询
- 范围消息搜索

### 转发服务 (forward_server.hpp)
- 实时消息转发
- 基于 RabbitMQ 的消息队列
- 用户在线状态管理

### 文件服务 (file_server.hpp)
- 文件上传/下载
- 头像、图片、语音等多媒体存储

### 语音服务 (speech_server.hpp)
- 语音识别 (集成百度 AI)
- 语音转文字

### 网关服务 (gateway_server.hpp)
- HTTP/WebSocket 接口
- 用户认证与会话管理
- 请求路由与负载均衡

## 快速开始

### 环境要求

- Ubuntu 22.04 (推荐)
- CMake 3.12+
- GCC/Clang (支持 C++11)
- Docker & Docker Compose

### 编译项目

```bash
cd server
mkdir build && cd build
cmake ..
make -j$(nproc)

cd ../script/
# 拷贝构建产物
bash build.sh
# 收集依赖库
bash depend.sh
```

### 配置文件

每个服务的配置文件位于 server/conf 目录：

- speech_server.conf - 语音服务配置
- file_server.conf - 文件服务配置
- user_server.conf - 用户服务配置
- forward_server.conf - 转发服务配置
- message_server.conf - 消息服务配置
- friend_server.conf - 好友服务配置
- gateway_server.conf - 网关服务配置

### Docker 部署

```bash
# 启动所有服务
docker-compose up -d

# 查看服务状态
docker-compose ps

# 查看日志
docker-compose logs -f [service_name]

# 停止服务
docker-compose down
```

服务启动顺序由 script/entrypoint.sh 控制，确保依赖服务先启动。

## 数据库设计

### MySQL 表结构 (位于 server/sql)

- user.sql - 用户信息表
- relation.sql - 好友关系表
- friend_request.sql - 好友申请表
- session.sql - 会话表
- session_member.sql - 会话成员表
- message.sql - 消息记录表

### ODB 实体定义 (位于 server/odb)

使用 ODB ORM 框架进行对象关系映射，实体类定义：

- `User` - 用户实体
- `Relation` - 好友关系实体
- `FriendRequest` - 好友申请实体
- `Session` - 会话实体
- `SessionMember` - 会话成员实体
- `Message` - 消息实体

## 核心组件

### 服务注册与发现 (registry.hpp)
基于 etcd 实现的服务注册与发现机制

### 通道管理 (channel.hpp)
`ChannelManager` 负责管理 RPC 服务通道

### 消息队列 (mq.hpp)
基于 RabbitMQ 的消息队列封装

### 数据访问层
- MySQL: `UserTable`, `MessageTable`, `RelationTable` 等
- Redis: `Session`, `Status`, `Code`
- Elasticsearch: `ESUser`, `ESMessage`

### 工具函数 (utils.hpp)
- `uuid()` - 生成唯一ID
- `verify_code()` - 生成验证码
- `read_file()` / `write_file()` - 文件操作
- `serialize()` / `unserialize()` - JSON序列化

## API 示例

### 用户注册
```protobuf
// 请求
message UserRegisterReq {
  string request_id = 1;
  string nickname = 2;
  string password = 3;
  string phone = 4;
  string verify_code = 5;
}

// 响应
message UserRegisterRsp {
  string request_id = 1;
  bool success = 2;
  string errmsg = 3;
}
```

### 发送消息
通过网关 WebSocket 连接发送消息，经由转发服务分发到接收方。

## 开发指南

### 添加新服务

1. 在 server/service 创建服务目录
2. 编写服务实现代码
3. 创建 CMakeLists.txt 和 Dockerfile
4. 在 `server/CMakeLists.txt` 添加子模块
5. 更新 compose.yaml

### 协议定义

Protocol Buffers 定义位于 server/proto，修改后需重新生成代码。

## 性能优化

- 使用连接池管理数据库连接
- Redis 缓存热点数据
- Elasticsearch 实现高效搜索
- RabbitMQ 削峰填谷
- Docker 容器化部署

## 贡献
欢迎提交 Issue 和 Pull Request！

## 许可证
MIT License

## 联系方式
如有问题或建议，请通过以下方式联系：
- Email: huzch123@gmail.com
- GitHub: [huzch](https://github.com/huzch)
