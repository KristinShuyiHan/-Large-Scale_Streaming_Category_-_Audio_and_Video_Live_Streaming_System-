# 添加一个可执行文件，名字是 EventLoopThreadTest，源文件是 EventLoopThreadTest.cpp
# Add an executable named "EventLoopThreadTest" using the source file "EventLoopThreadTest.cpp"
add_executable(EventLoopThreadTest EventLoopThreadTest.cpp)

# 将目标文件 EventLoopThreadTest 链接到 base 和 network 库
# Link the target "EventLoopThreadTest" to the libraries "base" and "network"
target_link_libraries(EventLoopThreadTest base network)

add_executable(InetAddressTest InetAddressTest.cpp)
target_link_libraries(InetAddressTest base network)

add_executable(SocketOptTest SocketOptTest.cpp)
target_link_libraries(SocketOptTest base network)

add_executable(AcceptorTest AcceptorTest.cpp)
target_link_libraries(AcceptorTest base network)

add_executable(TcpConnectionTest TcpConnectionTest.cpp)
target_link_libraries(TcpConnectionTest base network)

add_executable(TcpServerTest TcpServerTest.cpp)
target_link_libraries(TcpServerTest base network)

add_executable(DnsServiceTest DnsServiceTest.cpp)
target_link_libraries(DnsServiceTest base network)

add_executable(TcpClientTest TcpClientTest.cpp)
target_link_libraries(TcpClientTest base network)

add_executable(UdpClientTest UdpClientTest.cpp)
target_link_libraries(UdpClientTest base network)

add_executable(UdpServerTest UdpServerTest.cpp)
target_link_libraries(UdpServerTest base network)