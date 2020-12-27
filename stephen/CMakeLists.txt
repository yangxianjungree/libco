set(STEPHEN_SERVER_SRC
        server.cpp
        common.cpp)

set(STEPHEN_CLIENT_SRC
        client.cpp)

add_executable(client ${STEPHEN_CLIENT_SRC})

add_executable(server ${STEPHEN_SERVER_SRC})
target_link_libraries(server colib_static pthread dl)
