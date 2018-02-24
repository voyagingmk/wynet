#ifndef WY_TCPCLIENT_H
#define WY_TCPCLIENT_H

#include "common.h"
#include "wysockbase.h"

namespace wynet
{
class TCPClient : public SocketBase
{
  sockaddr_in m_serSockaddr;
  struct hostent *h;

public:
  TCPClient(const char *host, int port);
  ~TCPClient();
  void Send(const char *data, size_t len);
  void Recvfrom();
};
};

#endif