//
//  ycsbc.cc
//  YCSB-C
//
//  Created by Jinglei Ren on 12/19/14.
//  Copyright (c) 2014 Jinglei Ren <jinglei@ren.systems>.
//

#include <cstring>
#include <string>
#include <iostream>
#include <vector>
#include <future>
#include <time.h>
#include "core/utils.h"
#include "core/timer.h"
#include "core/client.h"
#include "core/core_workload.h"
#include "db/db_factory.h"
#include "db/myKV_db.h"

using namespace std;

void UsageMessage(const char *command);
bool StrStartWith(const char *str, const char *pre);
string ParseCommandLine(int argc, const char *argv[], utils::Properties &props);

int DelegateClient(ycsbc::DB *db, ycsbc::CoreWorkload *wl, const int num_ops,
    bool is_loading) {
  db->Init();
  ycsbc::Client client(*db, *wl);
  int oks = 0;
  for (int i = 0; i < num_ops; ++i) {
    if (is_loading) {
      oks += client.DoInsert();
    } else {
      oks += client.DoTransaction();
    }
  }
  db->Close();
  return oks;
}

int DelegateClient_lat(ycsbc::DB *db, ycsbc::CoreWorkload *wl, const int num_ops,
    bool is_loading, double * avg_lat) {
  db->Init();
  ycsbc::Client client(*db, *wl);
  int oks = 0;
  timespec t0, t1;
  uint64_t lat;
  for (int i = 0; i < num_ops; ++i) {
    if (is_loading) {
      oks += client.DoInsert();
    } else {
      clock_gettime(CLOCK_REALTIME, &t0);
      oks += client.DoTransaction();
      clock_gettime(CLOCK_REALTIME, &t1);
      uint64_t tmp = (t1.tv_sec - t0.tv_sec) * 1000000000 + t1.tv_nsec - t0.tv_nsec;
      lat += tmp;
    }
  }
  *avg_lat = (double)lat / num_ops;
  db->Close();
  return oks;
}

int main(const int argc, const char *argv[]) {
  utils::Properties props;
  string file_name = ParseCommandLine(argc, argv, props);

  ycsbc::DB *db = ycsbc::DBFactory::CreateDB(props);
  ycsbc::DB **dbList = NULL;  // sjc added
  if (!db) {
    cout << "Unknown database name " << props["dbname"] << endl;
    exit(0);
  }

  ycsbc::CoreWorkload wl;
  wl.Init(props);

  const int num_threads = stoi(props.GetProperty("threadcount", "1"));
  if (props["dbname"] == "myKV" || props["dbname"] == "redis") {
    dbList = (ycsbc::DB **)malloc(sizeof(ycsbc::DB *) * num_threads);
    dbList[0] = db;
    for (int i = 1; i < num_threads; i++) {
      dbList[i] = ycsbc::DBFactory::CreateDB(props);
    }
  }
  double * avg_lat = NULL;
  if (props["measure"] == "latency") {
    avg_lat = (double*)malloc(sizeof(double) * num_threads);
  }

  // Loads data
  vector<future<int>> actual_ops;
  int total_ops = stoi(props[ycsbc::CoreWorkload::RECORD_COUNT_PROPERTY]);
  for (int i = 0; i < num_threads; ++i) {
    if (props["dbname"] == "myKV" || props["dbname"] == "redis") {
      if (props["measure"] == "latency") {
        actual_ops.emplace_back(async(launch::async, 
          DelegateClient_lat, dbList[i], &wl, total_ops / num_threads, true, &avg_lat[i]));
      } else {
        actual_ops.emplace_back(async(launch::async,
          DelegateClient, dbList[i], &wl, total_ops / num_threads, true));
      }
    } else {
      if (props["measure"] == "latency") {
        actual_ops.emplace_back(async(launch::async, 
          DelegateClient_lat, db, &wl, total_ops / num_threads, true, &avg_lat[i]));
      } else {
        actual_ops.emplace_back(async(launch::async,
          DelegateClient, db, &wl, total_ops / num_threads, true));
      }
    }
  }
  assert((int)actual_ops.size() == num_threads);

  int sum = 0;
  for (auto &n : actual_ops) {
    assert(n.valid());
    sum += n.get();
  }
  cerr << "# Loading records:\t" << sum << endl;

  // Peforms transactions
  actual_ops.clear();
  total_ops = stoi(props[ycsbc::CoreWorkload::OPERATION_COUNT_PROPERTY]);
  utils::Timer<double> timer;
  timer.Start();
  for (int i = 0; i < num_threads; ++i) {
    if (props["dbname"] == "myKV" || props["dbname"] == "redis") {
      if (props["measure"] == "latency") {
        actual_ops.emplace_back(async(launch::async, 
          DelegateClient_lat, dbList[i], &wl, total_ops / num_threads, false, &avg_lat[i]));
      } else {
        actual_ops.emplace_back(async(launch::async,
          DelegateClient, dbList[i], &wl, total_ops / num_threads, false));
      }
    } else {
      if (props["measure"] == "latency") {
        actual_ops.emplace_back(async(launch::async, 
          DelegateClient_lat, db, &wl, total_ops / num_threads, false, &avg_lat[i]));
      } else {
        actual_ops.emplace_back(async(launch::async,
          DelegateClient, db, &wl, total_ops / num_threads, false));
      }
    }
  }
  assert((int)actual_ops.size() == num_threads);

  sum = 0;
  for (auto &n : actual_ops) {
    assert(n.valid());
    sum += n.get();
  }
  double lat = 0;
  if (props["measure"] == "latency") {
    for (int i = 0; i < num_threads; i++) {
      lat += avg_lat[i];
    }
    lat = lat / num_threads;
  }

  double duration = timer.End();
  cerr << "# Transaction throughput (KTPS)" << endl;
  cerr << props["dbname"] << '\t' << file_name << '\t' << num_threads << '\t';
  cerr << total_ops / duration / 1000 << endl;
  cerr << "# Transaction latency (ns)" << endl;
  cerr << lat << endl;
}

string ParseCommandLine(int argc, const char *argv[], utils::Properties &props) {
  int argindex = 1;
  string filename;
  while (argindex < argc && StrStartWith(argv[argindex], "-")) {
    if (strcmp(argv[argindex], "-threads") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("threadcount", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-db") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("dbname", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-host") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("host", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-port") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("port", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-slaves") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("slaves", argv[argindex]);
      argindex++;
    } else if (strcmp(argv[argindex], "-table") == 0) {
      argindex ++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      props.SetProperty("table", argv[argindex]);
      argindex ++;
    } else if (strcmp(argv[argindex], "-lat") == 0) {
      props.SetProperty("measure", "latency");
      argindex++;
    } else if (strcmp(argv[argindex], "-P") == 0) {
      argindex++;
      if (argindex >= argc) {
        UsageMessage(argv[0]);
        exit(0);
      }
      filename.assign(argv[argindex]);
      ifstream input(argv[argindex]);
      try {
        props.Load(input);
      } catch (const string &message) {
        cout << message << endl;
        exit(0);
      }
      input.close();
      argindex++;
    } else {
      cout << "Unknown option '" << argv[argindex] << "'" << endl;
      exit(0);
    }
  }

  if (argindex == 1 || argindex != argc) {
    UsageMessage(argv[0]);
    exit(0);
  }
  if (props.GetProperty("measure", "aaa") == "aaa") {
    props.SetProperty("measure", "throughput");
  }
  return filename;
}

void UsageMessage(const char *command) {
  cout << "Usage: " << command << " [options]" << endl;
  cout << "Options:" << endl;
  cout << "  -threads n: execute using n threads (default: 1)" << endl;
  cout << "  -db dbname: specify the name of the DB to use (default: basic)" << endl;
  cout << "  -P propertyfile: load properties from the given file. Multiple files can" << endl;
  cout << "                   be specified, and will be processed in the order specified" << endl;
  cout << "  -table TableType: specify the table type used by MyKV, can be SIMPLE, CUCKOO, and HOPSCOTCH" << endl;
  cout << "  -lat: measure the latency instead of throughput" << endl;
}

inline bool StrStartWith(const char *str, const char *pre) {
  return strncmp(str, pre, strlen(pre)) == 0;
}

