#include<sys/select.h>
#include<unistd.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>




void quirePositionv1(int sockfd, struct sockaddr_in addr, unsigned char commandId){
    socklen_t addr_len=sizeof(addr);

    unsigned char send_buf[2048] = {0};
    send_buf[0] = 15 * 10 + 3; // 0xF4
    send_buf[1] = commandId; // 0x07
    sendto(sockfd, send_buf, 2, 0, (sockaddr*)&addr, addr_len);
    printf("send query position\n");
}

void quirePositionv2(int sockfd, struct sockaddr_in addr, , unsigned char commandId){
    socklen_t addr_len=sizeof(addr);

    unsigned char send_buf[2048] = {0};
    send_buf[0] = 15 * 10 + 4; // 0xF4
    send_buf[1] = commandId; // 0x07
    sendto(sockfd, send_buf, 2, 0, (sockaddr*)&addr, addr_len);
    printf("send query position\n");
}

int main(){
    //同一台电脑测试，需要两个端口
    // int port_in  = 12321;
    int port_out = 12000;
    int sockfd;

    // 创建socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(-1==sockfd){
        return false;
        puts("Failed to create socket");
    }

    // 设置地址与端口
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;       // Use IPV4
    addr.sin_port = htons(port_out);    //
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    
   // Time out
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 200000;  // 200 ms
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));

    // Bind 端口，用来接受之前设定的地址与端口发来的信息,作为接受一方必须bind端口，并且端口号与发送方一致
    if (bind(sockfd, (struct sockaddr*)&addr, addr_len) == -1){
        printf("Failed to bind socket on port %d\n", port_out);
        close(sockfd);
        return false;
    }

    unsigned char buffer[128];
    memset(buffer, 0, 128);

    int counter = 0;
    while(1){
        struct sockaddr_in src;
        socklen_t src_len = sizeof(src);
        memset(&src, 0, sizeof(src));
	
     // 阻塞住接受消息
        printf("start receive\n");
        int sz = recvfrom(sockfd, buffer, 128, 0, (sockaddr*)&src, &src_len);
        if (sz > 0){
            buffer[sz] = 0;
            printf("length : %d\n", sz);
            // printf("Get Message %d: %s\n", counter++, buffer);
            u_short receive_port = src.sin_port;   //
            // std::cout <<  addr.sin_addr << std::endl;
            std::cout << receive_port << std::endl;
                        

            for (int i = 0; i < sz; i ++){
                printf("%x ", buffer[i]);
            }


            // =======================================================================
            int commandId;
            std::cin >> commandId;
            if (commandId == 1){
                
            }else if (commandId == 2){
                
            }else if (commandId == 3){

            }else if (commandId == 4){

            }else if (commandId == 5){

            }else if (commandId == 6){

            }else if (commandId == 7){
                quirePosition(sockfd, addr);
            }else{
                printf("invalid command id\n");
            }

            // =======================================================================
        }
        else{
            // puts("timeout");
        }
    }

    close(sockfd);
    return 0;
}
