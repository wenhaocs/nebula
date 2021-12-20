SET(CONTEXT_TEST_LIBS
)

nebula_add_executable(
    NAME
        graph_cache_test
    SOURCES
        GraphCacheTest.cpp
    OBJECTS
        $<TARGET_OBJECTS:graph_cache_obj>
        ${CONTEXT_TEST_LIBS}
    LIBRARIES
        ${THRIFT_LIBRARIES}
        ${PROXYGEN_LIBRARIES}
        ${CACHELIB_LIBRARIES}
        wangle
        gtest
)
