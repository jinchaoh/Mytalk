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

#define BUFFER_SIZE 64//读缓冲数据大小
#define FD_LIMIT 65535//文件描述符数量限制
#define USER_LIMIT 5//users' max number

struct client_data//clients
{
        sockaddr_in address;//
        char * write_buf;//写缓冲数据
        char buf[BUFFER_SIZE];//读缓冲数据
};

int setnoblocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option|O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

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
    assert(bind_ret!=-1);

    int listenfd = listen (sockfd,5);
    assert(listenfd!=-1);


    client_data * users = new client_data[FD_LIMIT];
    pollfd fds[FD_LIMIT+1];
    static int user_counter;

    for(int i=1;i<=USER_LIMIT;++i)
    {
        fds[i].fd = -1;
        fds[i].events = 0;
    }
    fds[0].fd = sockfd;
    fds[0].events = POLLIN|POLLERR;
    fds[0].revents = 0;

    while(1)
    {
        int poll_ret = poll(fds,user_counter+1,-1);
        if(poll_ret<0)
        {
            printf("poll failure\n");
            break;
        }

        for(int i=0;i<user_counter+1;i++)
        {

            if((fds[i].fd == sockfd)&&(fds[i].revents&POLLIN))
            {
                struct sockaddr_in client_address;
                socklen_t client_addlent = sizeof(client_address);
                int connfd = accept(fds[i],(const struct *)&client_address,&client_addlent);
                if(connfd<0)
                {
                    printf("errno is:%d\n",errno);
                    continue;
                }
                //if the requester too many ,close the new one;
                if(user_counter>=USER_LIMIT)
                {
                    const info ="too many users\n";
                    printf("%s",info);
                    send(connfd,info,strlen(info),0)
                    close(connfd);
                    continue;
                }
                user_counter++;
                users[connfd].address = client_address.sin_addr;
                setnoblocking(connfd);
                fds[user_counter].fd=connfd;
                fds[user_counter].events=POLLIN |POLLHUP |POLLERR;
                fds[user_counter].revents=0;
                printf("comes a new user, now have %d users\n",user_counter);
            }
            else if(fds[i].revents&POLLERR)
            {


            }
            else if(fds[i].revents&POLLREHUP)
            {


            }
            else if(fds[i].revents&POLLIN)
            {


            }
            else if(fds[i].revents&POLLOUT)
            {


            }
        }
    }

    delete [] users;
    close(sockfd);
    return 0;
}
