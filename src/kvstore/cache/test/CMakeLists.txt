set(cache_test_deps
    $<TARGET_OBJECTS:storage_cache_obj>
)

nebula_add_executable(
    NAME
        storage_cache_test
    SOURCES
        StorageCacheTest.cpp
    OBJECTS
        ${cache_test_deps}
    LIBRARIES
        ${CACHELIB_LIBRARIES}
        ${THRIFT_LIBRARIES}
        ${PROXYGEN_LIBRARIES}
        wangle
        gtest
)
