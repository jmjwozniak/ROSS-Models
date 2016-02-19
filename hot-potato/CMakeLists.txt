INCLUDE_DIRECTORIES(${ROSS_SOURCE_DIR} ${ROSS_BINARY_DIR})

SET(hot_potato_srcs
hp.c)

ADD_EXECUTABLE(hot-potato ${hot_potato_srcs})

TARGET_LINK_LIBRARIES(hot-potato ROSS m)
