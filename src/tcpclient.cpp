#include "tcpclient.h"
#include "client.h"
#include "net.h"
#include "utils.h"

namespace wynet
{

// http://man7.org/linux/man-pages/man2/connect.2.html
void TCPClient::OnTcpWritable(EventLoop *eventLoop, std::weak_ptr<FDRef> fdRef, int mask)
{
    log_debug("OnTcpWritable");
    std::shared_ptr<FDRef> sfdRef = fdRef.lock();
    if (!sfdRef)
    {
        return;
    }
    std::shared_ptr<TCPClient> tcpClient = std::dynamic_pointer_cast<TCPClient>(sfdRef);
    int error;
    socklen_t len;
    len = sizeof(error);
    if (getsockopt(tcpClient->sockfd(), SOL_SOCKET, SO_ERROR, &error, &len) < 0)
    {
        // close it
    }
    else
    {
        // connect ok, remove event
        tcpClient->getLoop().deleteFileEvent(tcpClient, LOOP_EVT_WRITABLE);
        tcpClient->_onTcpConnected();
    }
}

TCPClient::TCPClient(PtrClient client) : onTcpConnected(nullptr),
                                         onTcpDisconnected(nullptr),
                                         onTcpRecvMessage(nullptr)
{

    m_parent = client;
}

void TCPClient::connect(const char *host, int port)
{
    int n;
    struct addrinfo hints, *res, *ressave;

    bzero(&hints, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    char buf[5];
    sprintf(buf, "%d", port);
    const char *serv = (char *)&buf;

    if ((n = getaddrinfo(host, serv, &hints, &res)) != 0)
        err_quit("tcp_connect error for %s, %s: %s",
                 host, serv, gai_strerror(n));
    ressave = res;
    int ret;
    do
    {
        setSockfd(socket(res->ai_family, res->ai_socktype, res->ai_protocol));
        if (sockfd() < 0)
            continue; /* ignore this one */

        int flags = Fcntl(sockfd(), F_GETFL, 0);
        Fcntl(sockfd(), F_SETFL, flags | O_NONBLOCK);

        SetSockRecvBufSize(sockfd(), 32 * 1024);
        SetSockSendBufSize(sockfd(), 32 * 1024);

        ret = ::connect(sockfd(), res->ai_addr, res->ai_addrlen);
        log_debug("connect: %d", ret);
        if ((ret == -1) && (errno == EINPROGRESS))
        {
            getLoop().createFileEvent(shared_from_this(), LOOP_EVT_WRITABLE,
                                      TCPClient::OnTcpWritable);
            break;
        }
        if (ret == 0)
            break; /* success */

        ::Close(sockfd()); /* ignore this one */
    } while ((res = res->ai_next) != NULL);

    if (res == NULL) /* errno set from final connect() */
        err_sys("tcp_connect error for %s, %s", host, serv);

    m_family = res->ai_family;
    memcpy(&m_sockaddr, res->ai_addr, res->ai_addrlen);
    m_socklen = res->ai_addrlen; /* return size of protocol address */

    freeaddrinfo(ressave);
    if (ret == 0)
    {
        _onTcpConnected();
    }
}

EventLoop &TCPClient::getLoop()
{
    return m_parent->getNet()->getLoop();
}

void TCPClient::_onTcpConnected()
{
    m_conn = std::make_shared<TcpConnection>();
    m_conn->setEventLoop(&getLoop());
    m_conn->setCtrl(shared_from_this());
    m_conn->setSockfd(sockfd());
    m_conn->setCallBack_Connected(onTcpConnected);
    m_conn->setCallBack_Disconnected(onTcpDisconnected);
    m_conn->setCallBack_Message(onTcpRecvMessage);
    getLoop().runInLoop(std::bind(&TcpConnection::onEstablished, m_conn));
}

void TCPClient::_onTcpDisconnected()
{
    m_conn = nullptr;
}

}; // namespace wynet
