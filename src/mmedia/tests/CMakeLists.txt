add_executable(HandShakeClientTest HandShakeClientTest.cpp)
target_link_libraries(HandShakeClientTest base network mmedia crypto)

add_executable(HandShakeServerTest HandShakeServerTest.cpp)
target_link_libraries(HandShakeServerTest base network mmedia crypto)

add_executable(RtmpServerTest RtmpServerTest.cpp)
target_link_libraries(RtmpServerTest base network mmedia crypto)

add_executable(RtmpClientTest RtmpClientTest.cpp)
target_link_libraries(RtmpClientTest base network mmedia crypto)

add_executable(HttpClientTest HttpClientTest.cpp)
target_link_libraries(HttpClientTest base network mmedia crypto)

add_executable(DtlsCertsTest DtlsCertsTest.cpp)
target_link_libraries(DtlsCertsTest base network mmedia crypto)