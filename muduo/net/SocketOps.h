#ifndef MUDUO_NET_SOCKETOPS_H
#define MUDUO_NET_SOCKETOPS_H
#include <arpa/inet.h>
#include <unistd.h>//
namespace mulib{
    namespace net{
        namespace socket{
            int createNonblockingOrDie();
            void bindOrDie(int sockfd, const sockaddr_in &addr);
            void listenOrDie(int sockfd);
            int accept(int sockfd, sockaddr_in *addr);
            int accept1(int sockfd, sockaddr_in *addr);
            void close(int sockfd);
            int connect(int sockfd, const sockaddr_in &addr);

            void setNonBlockAndCloseOnExec(int sockfd);
            void shutdownWrite(int sockfd);

            int getSocketError(int sockfd);
            sockaddr_in getLocalAddr(int sockfd);
            sockaddr_in getPeerAddr(int sockfd);
            bool isSelfConnect(int sockfd);

            void fromHostPort(const char *ip, uint16_t port, struct sockaddr_in *addr);
            void toHostPort(char *buf, size_t size, const struct sockaddr_in &addr);
            void toIpPort(char *buf, size_t size, const struct sockaddr *addr);
            void toIp(char *buf, size_t size, const struct sockaddr *addr);
        }
    }
}
#endif