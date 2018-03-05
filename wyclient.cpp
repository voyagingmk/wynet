#include "wyclient.h"
#include "protocol.h"
#include "protocol_define.h"
#include "wynet.h"

namespace wynet
{

void OnTcpMessage(struct aeEventLoop *eventLoop,
                  int fd, void *clientData, int mask)
{
    log_debug("OnTcpMessage");
    Client *client = (Client *)(clientData);
    TCPClient& tcpClient = client->tcpClient;
    SockBuffer& buf = tcpClient.buf;
    int ret = buf.readIn(fd);
    if (ret == 0)
    {
        tcpClient.Close();
        return;
    }
    // validate packet
    while(buf.hasPacketHeader()) {
        
    }
    PacketHeader header;
    int n = Readn(fd, (char *)(&header), HeaderBaseLength);
    Readn(fd, (char *)(&header) + HeaderBaseLength, header.getHeaderLength() - HeaderBaseLength);
    if (!header.isFlagOn(HeaderFlag::PacketLen))
    {
        return;
    }
    Protocol protocol = header.getProtocol();
    uint32_t packetLen = header.getUInt(HeaderFlag::PacketLen);
    switch (protocol)
    {
    case Protocol::Handshake:
    {
        protocol::Handshake handShake;
        assert(packetLen == header.getHeaderLength() + sizeof(protocol::Handshake));
        Readn(fd, (char *)(&handShake), sizeof(protocol::Handshake));
        log_debug("clientId %d, udpPort %d", handShake.clientId, handShake.udpPort);
        break;
    }
    default:
        break;
    }
}

Client::Client(WyNet *net, const char *host, int tcpPort) : net(net),
                                                            tcpClient(this, host, tcpPort),
                                                            udpClient(NULL),
                                                            kcpDict(NULL),
                                                            onTcpConnected(NULL)
{
}

Client::~Client()
{
    log_info("[Client] close tcp sockfd %d", tcpClient.m_sockfd);
    tcpClient.Close();
}

void Client::_onTcpConnected()
{
    aeCreateFileEvent(net->aeloop, tcpClient.m_sockfd, AE_READABLE,
                      OnTcpMessage, (void *)this);
    if (onTcpConnected)
        onTcpConnected(this);
}
};
