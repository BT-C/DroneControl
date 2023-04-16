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

/* CRC余式表 */
const unsigned short crc_table[] = {
    0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
    0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
    0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
    0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
    0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
    0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
    0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
    0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
    0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
    0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
    0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
    0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
    0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
    0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
    0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
    0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
    0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
    0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
    0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
    0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
    0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
    0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
    0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
    0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
    0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
    0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
    0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
    0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
    0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
    0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
    0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
    0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040 
};

//查表法计算crc
unsigned short do_crc_table(unsigned char *ptr, int len)
{
    // unsigned short crc = 0x0000;
    unsigned short crc = 0xFFFF;
    
    while(len--) 
    {
        crc = (crc << 8) ^ crc_table[(crc >> 8 ^ *ptr++) & 0xff];
    }
    
    return(crc);
}

unsigned short Crc16Byte(unsigned short crc, unsigned char data){
    int i = (crc ^ data) & 0xFF;
    return (ushort) ((crc >> 8) ^ crc_table[i]);
}

unsigned short Crc16Generic(unsigned short crc, unsigned char *buffer, int offset, int len){
    for (int i = 0; i < len; i ++){
        crc = Crc16Byte(crc, buffer[i + offset]);   
    }

    return crc;
}

unsigned short Calc(unsigned char * buffer, int offset, int len){
    return Crc16Generic(0xFFFF, buffer, offset, len);
}


//直接计算法计算crc
unsigned short do_crc(unsigned char *ptr, int len)
{
    unsigned int i;
    unsigned short crc = 0x0000;
    
    while(len--)
    {
        crc ^= (unsigned short)(*ptr++) << 8;
        for (i = 0; i < 8; ++i)
        {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    
    return crc;
}

void setCRC(unsigned char *last_send_buf, unsigned char *start_buf, int length){
    // unsigned short crc = do_crc_table(start_buf, length);
    unsigned short crc = Calc(start_buf, 2, length);
    last_send_buf[1] = (unsigned char)(crc / 256);
    last_send_buf[0] = (unsigned char)(crc % 256);
}

unsigned short getCRC(unsigned char *start_buf, int length){
    unsigned short crc = Calc(start_buf, 2, length);
    return crc;
}

void showMessage(unsigned char *send_buf, int length){
    printf("Message content : ");
    for (int i = 0; i < length; i ++){
        printf("%x ", send_buf[i]);
    }
    printf("\n");
}


void wrapMessage(unsigned char *send_buf, unsigned char length, unsigned char sequenceNumber){
    // 0xA55A
    send_buf[0] = 0xA5;      
    send_buf[1] = 0x5A;
    // length of message
    send_buf[2] = length;
    // SRCID (0x00)
    send_buf[3] = 0;
    // DSTID (0x00)
    send_buf[4] = 0;
    // sequence number
    send_buf[5] = sequenceNumber;
}

void float2hex(unsigned char *out, float value){
    unsigned char *temp = (unsigned char *)&value;
    for (int i = 0; i < 4; i ++)
        out[4 - i] = temp[i];
}

void setPosition(int sockfd, struct sockaddr_in addr, unsigned char commandId, unsigned char sequenceNumber,
        unsigned short time, float x, float y, float z, float offset){
    socklen_t addr_len=sizeof(addr);

    unsigned char send_buf[2048] = {0};
    unsigned char length = 8 + 21;
    wrapMessage(send_buf, length, sequenceNumber);
    send_buf[6] = 0xF4; // 0xF4
    send_buf[7] = commandId; // 0x03
    send_buf[8] = 0;

    send_buf[9] = time % 256;
    send_buf[10] = (unsigned int)(time / 256);
    float2hex(&send_buf[11], x);
    float2hex(&send_buf[15], y);
    float2hex(&send_buf[19], z);
    float2hex(&send_buf[23], offset);

    setCRC(&send_buf[length - 2], send_buf, length - 4);
    
    showMessage(send_buf, length);
    sendto(sockfd, send_buf, 2, 0, (sockaddr*)&addr, addr_len);
    printf("set position\n");
}

void setSeed(int sockfd, struct sockaddr_in addr, unsigned char commandId, unsigned char sequenceNumber,
        unsigned short time, float x, float y, float z, float offset){
    socklen_t addr_len=sizeof(addr);

    unsigned char send_buf[2048] = {0};
    unsigned char length = 8 + 21;
    wrapMessage(send_buf, length, sequenceNumber);
    send_buf[6] = 0xF4; // 0xF4
    send_buf[7] = commandId; // 0x03
    send_buf[8] = 0;
    
    send_buf[9] = time % 256;
    send_buf[10] = (unsigned int)(time / 256);
    float2hex(&send_buf[11], x);
    float2hex(&send_buf[15], y);
    float2hex(&send_buf[19], z);
    float2hex(&send_buf[23], offset);

    setCRC(&send_buf[length - 2], send_buf, length - 4);
    
    showMessage(send_buf, length);
    sendto(sockfd, send_buf, 2, 0, (sockaddr*)&addr, addr_len);
    printf("set seed\n");
}

void setPose(int sockfd, struct sockaddr_in addr, unsigned char commandId, unsigned char sequenceNumber,
        unsigned short time, float x, float y, float z, float offset){
    socklen_t addr_len=sizeof(addr);

    unsigned char send_buf[2048] = {0};
    unsigned char length = 8 + 20;
    wrapMessage(send_buf, length, sequenceNumber);
    send_buf[6] = 0xF4; // 0xF4
    send_buf[7] = commandId; // 0x03

    send_buf[8] = time % 256;
    send_buf[9] = (unsigned int)(time / 256);
    float2hex(&send_buf[10], x);
    float2hex(&send_buf[14], y);
    float2hex(&send_buf[18], z);
    float2hex(&send_buf[22], offset);

    setCRC(&send_buf[length - 2], send_buf, length - 4);
    
    showMessage(send_buf, length);
    sendto(sockfd, send_buf, 2, 0, (sockaddr*)&addr, addr_len);
    printf("set pose\n");
}

void setSeedtest(int sockfd, struct sockaddr_in addr, unsigned char commandId, unsigned char sequenceNumber){
    socklen_t addr_len=sizeof(addr);

    unsigned char send_buf[2048] = {0};
    unsigned char length = 8 + 21;
    wrapMessage(send_buf, length, sequenceNumber);
    send_buf[6] = 0xF4; // 0xF4
    send_buf[7] = commandId; // 0x01
    send_buf[8] = 1;

    send_buf[9] = 7 * 16 + 8;
    send_buf[10] = 0;
    
    send_buf[11] = 0;
    send_buf[12] = 0;
    send_buf[13] = 6 * 16;
    send_buf[14] = 4 * 16;

    send_buf[15] = 9 * 16 + 10;
    send_buf[16] = 9 * 16 + 9;
    send_buf[17] = 1 * 16 + 9;
    send_buf[18] = 3 * 16 + 15;

    send_buf[19] = 0;
    send_buf[20] = 0;
    send_buf[21] = 0;
    send_buf[22] = 0;

    send_buf[23] = 0;
    send_buf[24] = 0;
    send_buf[25] = 0;
    send_buf[26] = 0;


    setCRC(&send_buf[27], send_buf, length - 4);

    showMessage(send_buf, length);
    sendto(sockfd, send_buf, 2, 0, (sockaddr*)&addr, addr_len);
    printf("send seed command\n");
}


void quirePositionv1(int sockfd, struct sockaddr_in addr, unsigned char commandId, unsigned char sequenceNumber){
    socklen_t addr_len=sizeof(addr);

    unsigned char send_buf[2048] = {0};
    unsigned char length = 8 + 2;
    wrapMessage(send_buf, length, sequenceNumber);
    send_buf[6] = 0xF4; // 0xF4
    send_buf[7] = commandId; // 0x03
    setCRC(&send_buf[8], send_buf, length - 4);
    
    showMessage(send_buf, length);
    sendto(sockfd, send_buf, 2, 0, (sockaddr*)&addr, addr_len);
    printf("send query position\n");
}

void land(int sockfd, struct sockaddr_in addr, unsigned char commandId, unsigned char sequenceNumber){
    socklen_t addr_len=sizeof(addr);

    unsigned char send_buf[2048] = {0};
    unsigned char length = 8 + 2;
    wrapMessage(send_buf, length, sequenceNumber);
    send_buf[6] = 0xF4; // 0xF4
    send_buf[7] = commandId; // 0x06
    setCRC(&send_buf[8], send_buf, length - 4);
    
    showMessage(send_buf, length);
    sendto(sockfd, send_buf, 2, 0, (sockaddr*)&addr, addr_len);
    printf("send query position\n");
}

void quirePositionv2(int sockfd, struct sockaddr_in addr, unsigned char commandId, unsigned char sequenceNumber){
    socklen_t addr_len=sizeof(addr);

    unsigned char send_buf[2048] = {0};
    unsigned char length = 8 + 2;
    wrapMessage(send_buf, length, sequenceNumber);
    send_buf[6] = 0xF4; // 0xF4
    send_buf[7] = commandId; // 0x07
    setCRC(&send_buf[8], send_buf, length - 4);
    
    showMessage(send_buf, length);
    sendto(sockfd, send_buf, 2, 0, (sockaddr*)&addr, addr_len);
    printf("send query position\n");
}


unsigned char getNext(unsigned char x){
    x = (x + 1) % 255;
    x = (x == 0) ? (x + 1) % 255 : x;
    return x;
}

float hex2float(unsigned char *buf){
    float *out = (float *)buf;
    for (int i = 0; i < 4; i ++){
        unsigned char temp = out[4 - i];
        out[4 - i] = out[i];
        out[i] = temp;
    }

    return (*out);
}

void parseMessage(unsigned char *rec_buf, int length){
    if ((rec_buf[0] != 0xA5) or (rec_buf[1] != 0x5A)){
        printf("invalid start infomation message\n");
        return ;
    }

    int msg_len = rec_buf[2];
    unsigned short crc = getCRC(rec_buf, msg_len - 4);
    if (rec_buf[length - 2] != (crc % 256) | rec_buf[length - 1] != (crc / 256)){
        printf("invalid crc\n");
        return ;
    }

    unsigned char * rec_infor = &rec_buf[6];
    if (rec_infor[0] != 0x00)
        return ;

    float roll = hex2float(&rec_buf[2]);
    float pitch = hex2float(&rec_buf[6]);
    float yaw = hex2float(&rec_buf[10]);
    float velx = hex2float(&rec_buf[14]);
    float vely = hex2float(&rec_buf[18]);
    float velz = hex2float(&rec_buf[22]);
    float posx = hex2float(&rec_buf[26]);
    float posy = hex2float(&rec_buf[30]);
    float posz = hex2float(&rec_buf[34]);

    printf("Position information :\n");
    printf("Roll : %f, Pitch : %f, Yaw : %f\n", roll, pitch, yaw);
    printf("Velx : %f, Vely : %f, Velz : %f\n", velx, vely, velz);
    printf("Posx : %f, Posy : %f, Posz : %f\n", posx, posy, posz);
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

    unsigned char buffer[2048];
    memset(buffer, 0, 2048);

    int counter = 0;
    unsigned char sequenceNumber = -1;
    while(1){
        struct sockaddr_in src;
        socklen_t src_len = sizeof(src);
        memset(&src, 0, sizeof(src));
	
     // 阻塞住接受消息
        printf("start receive\n");
        int sz = recvfrom(sockfd, buffer, 2048, 0, (sockaddr*)&src, &src_len);
        if (sz > 0){
            buffer[sz] = 0;
            printf("length : %d\n", sz);
            // printf("Get Message %d: %s\n", counter++, buffer);
            u_short receive_port = src.sin_port;   //
            // std::cout <<  addr.sin_addr << std::endl;
            std::cout << receive_port << std::endl;
                        

            // for (int i = 0; i < sz; i ++){
            //     printf("%x ", buffer[i]);
            // }
            parseMessage(buffer, 2048);

            // =======================================================================
            sequenceNumber = getNext(sequenceNumber);
            int commandId = -1;
            std::cin >> commandId;
            
            if (commandId == 0){
                float x = 1, y = 2, z = 3, offset = 0.1;
                unsigned short time = 0;
                setPosition(sockfd, addr, commandId, sequenceNumber, time, x, y, z, offset);
            }else if (commandId == 1){
                // setSeed(sockfd, addr, commandId, 255);
                // setSeed(sockfd, addr, commandId, sequenceNumber);
                float x = 1, y = 2, z = 3, offset = 0.1;
                unsigned short time = 0;
                setSeed(sockfd, addr, commandId, sequenceNumber, time, x, y, z, offset);
            }else if (commandId == 2){
                float x = 1, y = 2, z = 3, offset = 0.1;
                unsigned short time = 0;
                setPose(sockfd, addr, commandId, sequenceNumber, time, x, y, z, offset);
            }else if (commandId == 3){
                quirePositionv1(sockfd, addr, commandId, sequenceNumber);
            }else if (commandId == 4){
                
            }else if (commandId == 5){

            }else if (commandId == 6){
                land(sockfd, addr, commandId, sequenceNumber);
            }else if (commandId == 7){
                quirePositionv2(sockfd, addr, commandId, sequenceNumber);
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
