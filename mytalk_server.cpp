#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <poll.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <assert.h>

int main(int argc,char* argv[])
{
    if(argc<=2)
    {
        printf("usage:%s ip_address port_number\n",basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    int port = atoi(argv[2]);

    int sockfd =  socket(AF_INET,SOCK_STREAM,0);
    assert(sockfd>=0);

    struct sockaddr_in address;
    bzero(&address,sizeof(address));

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET,ip,&address.sin_addr);

    int bind_ret = bind(sockfd,(struct sockaddr*)&address, sizeof(address));
    assert(bind_ret!=-1)

    int listenfd = listen (sockfd,5);
    assert(listenfd!=-1);



    return 0;
}
