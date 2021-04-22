#include "myKV_db.h"

#include <cstring>
#include <cstdlib>
#include <string>
#include <sstream>
#include <random>

using namespace std;

namespace ycsbc {

MyKVDB::MyKVDB(string tableType) {
    TableType t;
    if (tableType == "simple") {
        t = SIMPLE;
    } else if (tableType == "cuckoo") {
        t = CUCKOO;
    } else if (tableType == "hopscotch") {
        t = HOPSCOTCH;
    }
    int ret = -1;
    ret = initRPCClient(&(this->client), "10.176.37.63", t);
    if (ret < 0) {
        printf("initRPCClient failed\n");
    }
}

inline int MyKVDB::Read(const string &table, const string &key, const vector<string> *fields, vector<KVPair> &result) {
    string key_index(table + key);
    key_index = key_index.substr(0, 15);
    char * ckey = (char *)key_index.c_str();
    int ret = -1;
    int64_t value;
    size_t vlen;
    ret = RPCClientKVGet1S(&client, ckey, key_index.length(), &value, &vlen);
    // ret = RPCClientKVGet2S(&client, ckey, key_index.length(), &value, &vlen);
    if (ret < 0) {
        printf("RPCClientKVGet1S failed\n");
        return DB::kErrorNoData;
    }
    stringstream ss;
    ss << value;
    result.push_back(std::make_pair(key, ss.str()));
    return DB::kOK;
}

inline int MyKVDB::Update(const string &table, const string &key, vector<KVPair> &values) {
    string key_index(table + key);
    key_index = key_index.substr(0, 15);
    int ret = -1;
    char * ckey = (char *)key_index.c_str();
    int64_t value = rand() % 100;
    ret = RPCClientKVPut(&client, ckey, key_index.length(), &value, sizeof(int64_t));
    if (ret < 0) {
        printf("RPCClientKVPut failed\n");
        return DB::kErrorNoData;
    }
    return DB::kOK;
}

inline int MyKVDB::Insert(const string &table, const string &key, vector<KVPair> &values) {
    return this->Update(table, key, values);
}

inline int MyKVDB::Delete(const string &table, const string &key) {
    string key_index(table + key);
    key_index = key_index.substr(0, 15);
    char * ckey = (char *)key_index.c_str();
    int ret = -1;
    ret = RPCClientKVDel(&client, ckey, key_index.length());
    if (ret < 0) {
        printf("RPCClientKVDel failed\n");
        return DB::kErrorNoData;
    }
    return DB::kOK;
}
}