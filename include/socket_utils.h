#ifndef WY_SOCKET_UTILS_H
#define WY_SOCKET_UTILS_H

#include "common.h"

namespace wynet
{
namespace socketUtils
{

extern int sock_socket(int family, int type, int protocol);

extern void sock_close(int fd);

extern void sock_listen(int fd, int backlog);

extern void sock_setsockopt(int fd, int level, int optname, const void *optval, socklen_t optlen);

extern void sock_linger(SockFd sockfd);

extern bool valid(SockFd sockfd);

extern bool isIPv4(sockaddr_storage sockAddr);

extern bool isIPv6(sockaddr_storage sockAddr);

extern int sock_fcntl(int sockfd, int cmd, int arg);

extern void getSrcAddr(SockFd sockfd, sockaddr_storage *addr, socklen_t &addrlen);

extern void getDestAddr(SockFd sockfd, sockaddr_storage *addr, socklen_t &addrlen);

extern bool isSelfConnect(SockFd sockfd);

extern void getNameInfo(struct sockaddr *addr, socklen_t addrlen, char *ipBuf, size_t ipBufSize, char *portBuf, size_t portBufSize);

extern void log_debug_addr(struct sockaddr *addr, socklen_t addrlen, const char *tag = "");

extern void log_debug_addr(struct sockaddr_storage *addr, socklen_t addrlen, const char *tag = "");

extern void setTcpNoDelay(SockFd sockfd, bool enabled);

extern int setTcpNonBlock(SockFd sockfd);

extern void setTcpKeepAlive(SockFd sockfd, bool enabled);

extern void setTcpKeepInterval(SockFd sockfd, int seconds);

extern void setTcpKeepIdle(SockFd sockfd, int seconds);

extern void setTcpKeepCount(SockFd sockfd, int c);

extern void printTcpInfo(SockFd sockfd);

extern int getSockSendBufSpace(int sockfd, int *space);

extern int setSockSendBufSize(SockFd sockfd, int newSndBuf, bool force = false);

extern int setSockRecvBufSize(SockFd sockfd, int newRcvBuf, bool force = false);

#ifdef DEBUG_MODE

extern void LogSocketState(SockFd sockfd);

#else

#define LogSocketState(fd)

#endif

}; // namespace socketUtils
}; // namespace wynet

#endif
