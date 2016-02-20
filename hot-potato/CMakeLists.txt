INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR} ${ROSS_BINARY_DIR})

SET(hot_potato1_srcs hp1.c )

ADD_EXECUTABLE(hot-potato1 ${hot_potato1_srcs})

SET(hot_potato2_srcs hp2.c)

ADD_EXECUTABLE(hot-potato2 ${hot_potato2_srcs})

TARGET_LINK_LIBRARIES(hot-potato1 ROSS m)
TARGET_LINK_LIBRARIES(hot-potato2 ROSS m)
