/* Copyright (c) 2021 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License.
 */

#ifndef GRAPH_CACHE_GRAPHCACHE_H
#define GRAPH_CACHE_GRAPHCACHE_H

#include "common/base/Base.h"
#include "common/base/CacheLibLRU.h"

namespace nebula {
namespace graph {

static const char kEdgePoolName[] = "EdgePool";
static const char kGraphCacheName[] = "__GraphCache__";

struct EdgePoolInfo {
  std::string poolName_;
  uint32_t capacity_;
  EdgePoolInfo(std::string poolName, uint32_t capacity)
      : poolName_(poolName), capacity_(capacity) {}
};

class GraphCache {
 public:
  GraphCache();

  ~GraphCache() { LOG(INFO) << "Destroy graph cache"; }

  bool init();

  // create a edge cache as pool
  bool createEdgePool(std::string poolName = kEdgePoolName);

  // create a edge property cache as pool
  bool createEdgePropertyPool(std::string& key, std::string* value);

  // get all edges of a vertex via key
  StatusOr<std::vector<std::string>> getEdges(std::string& key);

  // insert the edges of a vertex as a whole
  bool addAllEdges(std::string& key, std::vector<std::string>& edges, uint32_t ttl = 300);

  // evict a vertex in cache
  void invalidateEdges(std::string& key);

  // get the size of the vertex pool
  uint32_t getEdgePoolSize();

 private:
  uint32_t capacity_ = 0;  // in MB
  std::unique_ptr<CacheLibLRU> cacheInternal_{nullptr};
  std::shared_ptr<EdgePoolInfo> edgePool_{nullptr};
};

}  // namespace graph
}  // namespace nebula

#endif  // GRAPH_CACHE_GRAPHCACHE_H
