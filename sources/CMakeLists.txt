
file(GLOB HEADER_LIST CONFIGURE_DEPENDS "../include/y/*.h")

add_library(flow SHARED)
target_sources(flow
        PRIVATE flow.cpp
        PUBLIC ${HEADER_LIST})
target_include_directories(flow PUBLIC ../include)
target_compile_features(flow PUBLIC cxx_std_17)