#pragma once
#include <odb/database.hxx>
#include <odb/mysql/database.hxx>

namespace huzch {

class MysqlClientFactory {
 public:
  static std::shared_ptr<odb::core::database> create(
      const std::string& user, const std::string& passwd, const std::string& db,
      const std::string& host, size_t port, const std::string& charset,
      size_t max_connections) {
    auto cpf =
        std::make_unique<odb::mysql::connection_pool_factory>(max_connections);
    return std::make_shared<odb::mysql::database>(
        user, passwd, db, host, port, "", charset, 0, std::move(cpf));
  }
};

}  // namespace huzch