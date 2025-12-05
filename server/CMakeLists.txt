cmake_minimum_required(VERSION 3.5)

set (CMAKE_CXX_STANDARD 20)
project (server)

add_executable(server Connection.cpp ErrorPage.cpp FileResponder.cpp Http.cpp HttpServer.cpp IndexPage.cpp InetSocketWrapper.cpp LoginApi.cpp LoginPage.cpp Page.cpp TimedEvent.cpp UploadApi.cpp)

target_link_libraries(server ssl crypto)