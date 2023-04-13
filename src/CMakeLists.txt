project(adverbs)

set(HEADER_FILES
        adverbs.h
        )

set(SOURCE_FILES
        adverbs.cpp
        )

add_library(adverbs_lib SHARED ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(
        adverbs_lib
        PUBLIC
        ibverbs)

