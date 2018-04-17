#ifndef WY_CLIENT_H
#define WY_CLIENT_H

#include "common.h"
#include "wykcp.h"
#include "wytcpclient.h"
#include "wyudpclient.h"
#include "wyconnection.h"
#include "noncopyable.h"
#include "eventloop.h"

namespace wynet
{

class WyNet;
class Test;

class Client: public Noncopyable 
{
    WyNet *m_net;
    TcpConnectionForClient m_conn;
    TCPClient m_tcpClient;
    UDPClient *m_udpClient;
    
public:
    friend class TCPClient;
    typedef void (*OnTcpConnected)(Client *);
    typedef void (*OnTcpDisconnected)(Client *);
    typedef void (*OnTcpRecvUserData)(Client *, uint8_t*, size_t);
    
    OnTcpConnected onTcpConnected;
    OnTcpDisconnected onTcpDisconnected;
    OnTcpRecvUserData onTcpRecvUserData;

    Client(WyNet *net, const char *host, int tcpPort);

    ~Client();

    void sendByTcp(const uint8_t *data, size_t len);
    
    void sendByTcp(PacketHeader *header);
    
    const TCPClient& getTcpClient() const {
        return m_tcpClient;
    }
    
    const UDPClient* getUdpClient() const {
        return m_udpClient;
    }
    
    WyNet* getNet() const {
        return m_net;
    }

private:
    
    void _onTcpConnected();

    void _onTcpMessage();
    
    void _onTcpDisconnected();
    
    friend void OnTcpMessage(EventLoop *loop, int fd, void *clientData, int mask);
};
};

#endif
