/* Copyright (c) 2021 vesoft inc. All rights reserved.
 *
 * This source code is licensed under Apache 2.0 License.
 */

#ifndef COMMON_BASE_CACHELIBLRU_H_
#define COMMON_BASE_CACHELIBLRU_H_

#include <cachelib/allocator/CacheAllocator.h>

#include "common/base/Base.h"
#include "common/base/Status.h"
#include "common/base/StatusOr.h"

namespace nebula {

using Cache = facebook::cachelib::LruAllocator;

class CacheLibLRU {
 public:
  explicit CacheLibLRU(std::string name,
                       uint32_t capacity,
                       uint32_t bucketsPower,
                       uint32_t locksPower)
      : name_(name), capacity_(capacity), bucketsPower_(bucketsPower), locksPower_(locksPower) {}

  /**
   * @brief Create cache instance. If there is any exception, we will allow the process continue.
   *
   * @param poolName: the pool to allocate cache space
   * @return Status
   */
  Status initializeCache() {
    Cache::Config config;
    try {
      config
          // size cannot exceed the maximum cache size (274'877'906'944 bytes)
          .setCacheSize(capacity_ * 1024 * 1024)
          .setCacheName(name_)
          .setAccessConfig({bucketsPower_, locksPower_})
          .validate();  // will throw if bad config
    } catch (const std::exception& e) {
      // We do not stop the service. Users should refer to the log to determine whether to restart
      // the service.
      return Status::Error("Cache configuration error: %s", e.what());
    }
    nebulaCache_ = std::make_unique<Cache>(config);

    return Status::OK();
  }

  /**
   * @brief add cache pool into cache instance
   *
   * @param poolName
   * @param poolSize
   * @return Status
   */
  Status addPool(std::string poolName, uint32_t poolSize) {
    if (poolIdMap_.find(poolName) != poolIdMap_.end()) {
      return Status::Error("Cache pool creation error. Cache pool exists: %s", poolName.data());
    }
    try {
      auto poolId = nebulaCache_->addPool(poolName, poolSize * 1024 * 1024);
      poolIdMap_[poolName] = poolId;
    } catch (const std::exception& e) {
      return Status::Error("Adding cache pool error: %s", e.what());
    }
    return Status::OK();
  }

  /**
   * @brief Get key from cache. Return true if found.
   *
   * @param key
   * @return Status error (cache miss) or value
   */
  StatusOr<std::string> get(const std::string& key) {
    std::shared_lock<std::shared_mutex> guard(lock_);
    auto itemHandle = nebulaCache_->find(key);
    if (itemHandle) {
      return std::string(reinterpret_cast<const char*>(itemHandle->getMemory()),
                         itemHandle->getSize());
    }
    return Status::Error("Cache miss!");
  }

  /**
   * @brief Insert or update value in cache pool
   *
   * @param key
   * @param value
   * @param poolName: The pool name to insert/update cache item
   * @param ttl
   * @return Status
   */
  Status put(const std::string& key,
             const std::string& value,
             std::string poolName,
             uint32_t ttl = 300) {
    if (poolIdMap_.find(poolName) == poolIdMap_.end()) {
      return Status::Error("Cache write error. Pool does not exists: %s", poolName.data());
    }
    auto itemHandle = nebulaCache_->allocate(poolIdMap_[poolName], key, value.size(), ttl);
    if (!itemHandle) {
      return Status::Error("Cache write error. Too many pending writes.");
    }

    {
      std::unique_lock<std::shared_mutex> guard(lock_);
      std::memcpy(itemHandle->getMemory(), value.data(), value.size());
      nebulaCache_->insertOrReplace(itemHandle);
    }
    return Status::OK();
  }

  /**
   * @brief CacheLib will first search for the key. If found, remove it.
   * Note here we do not log anything if not found, as it can have a good chance that an item is not
   * in the cache due to ttl setting.
   *
   * @param key
   */
  void invalidateItem(const std::string& key) {
    nebulaCache_->remove(key);
  }  // do we need to lock here?

  /**
   * @brief Get the configured size of the pool
   *
   * @param poolName
   * @return Status error (pool not existing) or unit64_t
   */
  StatusOr<uint64_t> getConfiguredPoolSize(const std::string& poolName) {
    if (poolIdMap_.find(poolName) == poolIdMap_.end()) {
      return Status::Error("Cache write error. Pool does not exists: %s", poolName.data());
    }
    return nebulaCache_->getPoolStats(poolIdMap_[poolName]).poolSize;
  }

 private:
  std::unique_ptr<Cache> nebulaCache_{nullptr};
  std::unordered_map<std::string, facebook::cachelib::PoolId> poolIdMap_;
  std::string name_;
  uint32_t capacity_ = 0;       // in MB
  uint32_t bucketsPower_ = 25;  // bucketsPower number of buckets in base 2 logarithm
  uint32_t locksPower_ = 2;     // locksPower number of locks in base 2 logarithm

  // CacheLib does not protect data at item level. We need to synchronize the access.
  mutable std::shared_mutex lock_;
};

}  // namespace nebula

#endif  // COMMON_BASE_CACHELIBLRU_H_
