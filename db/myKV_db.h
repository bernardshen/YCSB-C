#ifndef YCSB_C_MYKV_DB_H_
#define YCSB_C_MYKV_DB_H_

#include "core/db.h"

#include <iostream>
#include <string.h>
#include <random>
#include "RPCClient.h"

namespace ycsbc {

class MyKVDB: public DB {
public:
    MyKVDB(std::string);
    void Init() {
        return;
    }
    int Read(const std::string &table, const std::string &key,
             const std::vector<std::string> *fields,
             std::vector<KVPair> &result);
    int Scan(const std::string &table, const std::string &key,
             int len, const std::vector<std::string> *fields,
             std::vector<std::vector<KVPair>> &result) {
        throw "Scan: function not implemented";
    }
    int Update(const std::string &table, const std::string &key,
               std::vector<KVPair> &values);
    int Insert(const std::string &table, const std::string &key,
               std::vector<KVPair> &values);
    int Delete(const std::string &table, const std::string &key);

private:
    RPCClient client;
};
}

#endif