project(adverbs)

set(HEADER_FILES
        adverbs.h
        )

set(SOURCE_FILES
        adverbs.cpp
        )

add_library(adverbs SHARED ${SOURCE_FILES} ${HEADER_FILES})

target_link_libraries(
        adverbs
        PUBLIC
        ibverbs)

