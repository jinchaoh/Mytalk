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
#include <errno.h>

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

        for(int i=0;i<user_counter+1;++i)
        {

            if((fds[i].fd == sockfd)&&(fds[i].revents&POLLIN))
            {
                struct sockaddr_in client_address;
                socklen_t client_addlent = sizeof(client_address);
                int connfd = accept(sockfd,(struct sockaddr*)&client_address,&client_addlent);
                if(connfd<0)
                {
                    printf("errno is:%d\n",errno);
                    continue;
                }
                //if the requester too many ,close the new one;
                if(user_counter>=USER_LIMIT)
                {
                    const char *info ="too many users\n";
                    printf("%s",info);
                    send(connfd,info,strlen(info),0);
                    close(connfd);
                    continue;
                }
                user_counter++;
                users[connfd].address = client_address;
                setnoblocking(connfd);
                fds[user_counter].fd=connfd;
                fds[user_counter].events=POLLIN |POLLHUP |POLLERR;
                fds[user_counter].revents=0;
                printf("comes a new user, now have %d users\n",user_counter);
            }
            else if(fds[i].revents&POLLERR)
            {
                printf("get an error from %d\n",fds[i].fd);
                continue;

            }
            else if(fds[i].revents&POLLRDHUP)
            {
                users[fds[i].fd]=users[fds[user_counter].fd];
                close(fds[i].fd);
                fds[i] = fds[user_counter];
                i--;
                user_counter--;
                printf("a user left\n");
            }
            else if(fds[i].revents&POLLIN)
            {
                  int connfd = fds[i].fd;
                memset( users[connfd].buf, '\0', BUFFER_SIZE );
                int ret = recv( connfd, users[connfd].buf, BUFFER_SIZE-1, 0 );
                //printf( "get %d bytes of client data %s from %d\n", ret, users[connfd].buf, connfd );
                if( ret < 0 )
                {
                    if( errno != EAGAIN )
                    {
                        close( connfd );
                        users[fds[i].fd] = users[fds[user_counter].fd];
                        fds[i] = fds[user_counter];
                        i--;
                        user_counter--;
                    }
                }
                else if( ret == 0 )
                {
                    //printf( "code should not come to here\n" );
                }
                else
                {
                    for( int j = 1; j <= user_counter; ++j )
                    {
                        if( fds[j].fd == connfd )
                        {
                            continue;
                        }

                        fds[j].events |= ~POLLIN;
                        fds[j].events |= POLLOUT;
                        users[fds[j].fd].write_buf = users[connfd].buf;
                    }
                }
            }
            else if(fds[i].revents&POLLOUT)
            {
                printf("out out out out out out out out out out out out \n");
                int connfd = fds[i].fd;
                if( ! users[connfd].write_buf )
                {
                    continue;
                }
                int ret = send( connfd, users[connfd].write_buf, strlen( users[connfd].write_buf ), 0 );
                users[connfd].write_buf = NULL;
                fds[i].events |= ~POLLOUT;
                fds[i].events |= POLLIN;
            }
        }
    }

    delete [] users;
    close(sockfd);
    return 0;
}
