#pragma once
#include <cpr/cpr.h>
#include <elasticlient/client.h>
#include <json/json.h>

#include <sstream>

#include "logger.hpp"
#include "utils.hpp"

namespace huzch {

class ESIndex {
 public:
  ESIndex(const std::shared_ptr<elasticlient::Client>& client,
          const std::string& name, const std::string& type = "_doc")
      : _client(client), _name(name), _type(type) {
    Json::Value tokenizer;
    tokenizer["tokenizer"] = "ik_max_word";
    Json::Value ik;
    ik["ik"] = tokenizer;
    Json::Value analyzer;
    analyzer["analyzer"] = ik;
    Json::Value analysis;
    analysis["analysis"] = analyzer;
    _index["settings"] = analysis;
  }

  ESIndex& append(const std::string& key, const std::string& type = "text",
                  bool enabled = true,
                  const std::string& analyzer = "ik_max_word") {
    Json::Value fields;
    fields["type"] = type;
    fields["analyzer"] = analyzer;
    if (!enabled) {
      fields["enabled"] = enabled;
    }

    _properties[key] = fields;
    return *this;
  }

  bool init(const std::string& id = "default_id") {
    Json::Value mappings;
    mappings["dynamic"] = true;
    if (!_properties.empty()) {
      mappings["properties"] = _properties;
    }
    _index["mappings"] = mappings;

    std::string body;
    bool ret = serialize(_index, body);
    if (!ret) {
      LOG_ERROR("索引序列化失败");
      return false;
    }
    // LOG_DEBUG("{}", body);

    try {
      auto resp = _client->index(_name, _type, id, body);
      if (resp.status_code < 200 || resp.status_code >= 300) {
        LOG_ERROR("索引 {} 初始化失败，状态码: {}", _name, resp.status_code);
        return false;
      }
    } catch (const std::exception& e) {
      LOG_ERROR("索引 {} 初始化失败: {}", _name, e.what());
      return false;
    }
    return true;
  }

 private:
  std::shared_ptr<elasticlient::Client> _client;
  std::string _name;
  std::string _type;
  Json::Value _properties;
  Json::Value _index;
};

class ESInsert {
 public:
  ESInsert(const std::shared_ptr<elasticlient::Client>& client,
           const std::string& name, const std::string& type = "_doc")
      : _client(client), _name(name), _type(type) {}

  template <class T>
  ESInsert& append(const std::string& key, const T& val) {
    _item[key] = val;
    return *this;
  }

  bool insert(const std::string& id = "") {
    std::string body;
    bool ret = serialize(_item, body);
    if (!ret) {
      LOG_ERROR("索引序列化失败");
      return false;
    }
    LOG_DEBUG("{}", body);

    try {
      auto resp = _client->index(_name, _type, id, body);
      if (resp.status_code < 200 || resp.status_code >= 300) {
        LOG_ERROR("索引 {} 插入失败，状态码: {}", _name, resp.status_code);
        return false;
      }
    } catch (const std::exception& e) {
      LOG_ERROR("索引 {} 插入失败: {}", _name, e.what());
      return false;
    }
    return true;
  }

 private:
  std::shared_ptr<elasticlient::Client> _client;
  std::string _name;
  std::string _type;
  Json::Value _item;
};

class ESRemove {
 public:
  ESRemove(const std::shared_ptr<elasticlient::Client>& client,
           const std::string& name, const std::string& type = "_doc")
      : _client(client), _name(name), _type(type) {}

  bool remove(const std::string& id) {
    try {
      auto resp = _client->remove(_name, _type, id);
      if (resp.status_code < 200 || resp.status_code >= 300) {
        LOG_ERROR("索引 {} 删除失败，状态码: {}", _name, resp.status_code);
        return false;
      }
    } catch (const std::exception& e) {
      LOG_ERROR("索引 {} 删除失败: {}", _name, e.what());
      return false;
    }
    return true;
  }

 private:
  std::shared_ptr<elasticlient::Client> _client;
  std::string _name;
  std::string _type;
};

class ESSearch {
 public:
  ESSearch(const std::shared_ptr<elasticlient::Client>& client,
           const std::string& name, const std::string& type = "_doc")
      : _client(client), _name(name), _type(type) {}

  ESSearch& append_must_term(const std::string& key, const std::string& val) {
    Json::Value fields;
    fields[key] = val;

    Json::Value term;
    term["term"] = fields;
    _must.append(term);
    return *this;
  }

  ESSearch& append_must_match(const std::string& key, const std::string& val) {
    Json::Value fields;
    fields[key] = val;

    Json::Value match;
    match["match"] = fields;
    _must.append(match);
    return *this;
  }

  ESSearch& append_must_not_terms(const std::string& key,
                                  const std::vector<std::string>& vals) {
    Json::Value fields;
    for (auto& val : vals) {
      fields[key].append(val);
    }

    Json::Value terms;
    terms["terms"] = fields;
    _must_not.append(terms);
    return *this;
  }

  ESSearch& append_should_match(const std::string& key,
                                const std::string& val) {
    Json::Value fields;
    fields[key] = val;

    Json::Value match;
    match["match"] = fields;
    _should.append(match);
    return *this;
  }

  Json::Value search() {
    Json::Value cond;
    if (!_must.empty()) {
      cond["must"] = _must;
    }
    if (!_must_not.empty()) {
      cond["must_not"] = _must_not;
    }
    if (!_should.empty()) {
      cond["should"] = _should;
    }

    Json::Value query;
    query["bool"] = cond;
    Json::Value index;
    index["query"] = query;

    std::string body;
    bool ret = serialize(index, body);
    if (!ret) {
      LOG_ERROR("索引序列化失败");
      return false;
    }
    LOG_DEBUG("{}", body);

    cpr::Response resp;
    try {
      resp = _client->search(_name, _type, body);
      if (resp.status_code < 200 || resp.status_code >= 300) {
        LOG_ERROR("索引 {} 搜索失败，状态码: {}", _name, resp.status_code);
        return Json::Value();
      }
    } catch (const std::exception& e) {
      LOG_ERROR("索引 {} 搜索失败: {}", _name, e.what());
      return Json::Value();
    }

    LOG_DEBUG("{}", resp.text);
    Json::Value resp_json;
    ret = unserialize(resp.text, resp_json);
    if (!ret) {
      LOG_ERROR("索引反序列化失败");
      return Json::Value();
    }
    return resp_json["hits"]["hits"];
  }

 private:
  std::shared_ptr<elasticlient::Client> _client;
  std::string _name;
  std::string _type;
  Json::Value _must;
  Json::Value _must_not;
  Json::Value _should;
};

}  // namespace huzch