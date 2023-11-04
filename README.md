# socket_chat
# 基于socket进行网络编程，使用线程池、IO复用技术实现高并发群聊
thread_poll.cpp,thread_poll.h              提供线程池的实现  
client_group_chat.cpp                      客户端实现（因代码量较少，并未拆分)  
server_group_chat.cpp,server_group_chat.h  服务端实现  
main_group_chat.cpp                        服务端主函数  

# 服务端编译
g++ main_group_chat.cpp server_group_chat.cpp thread_poll.cpp -o server_chat -pthread
# 客户端编译
g++ client_group_chat.cpp -o client_group -pthread
