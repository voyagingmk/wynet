#ifndef WY_SOCKBASE_H
#define WY_SOCKBASE_H

#include "common.h"
#include "wybuffer.h"
#include "protocol.h"

namespace wynet
{
    
// ring buffer
// buffer size must pow of 2
class SockBuffer {
    BufferRef bufRef;
    size_t len;
    uint32_t headIdx;
    uint32_t tailIdx;
public:
    SockBuffer():
        len(0),
        headIdx(0),
        tailIdx(0)
    {
    }
    
    /*
     size = 10
     0  1   2   3   4   5   6   7
            h      <=       t
     h = 2
     t = 6
     size - t = 8 - 6 = 2
     h - 0 = 2 - 0 = 2
     leftSpace: 2 + 2 = 4 slots
     
     0  1   2   3   4   5   6   7
     t              <           h
     h = 7
     t = 0
     leftSpace: h - t = 7 - 0 = 7 slots
    */
    int leftSpace() {
        Buffer* p = bufRef.get();
        if (headIdx <= tailIdx) {
            return p->size - (tailIdx - headIdx);
        } else {
            return (headIdx - tailIdx);
        }
    }
    
    int recvSize() {
        Buffer* p = bufRef.get();
        if (headIdx <= tailIdx) {
            return (tailIdx - headIdx);
        } else {
            return p->size - (headIdx - tailIdx);
        }
    }
    
    int readIn(int sockfd) {
        Buffer* p = bufRef.get();
        int npend;
        ioctl(sockfd, FIONREAD, &npend);
        
        // 1. make sure there is enough space for recv
        bool expanded = false;
        size_t oldSize = p->size;
        while (npend > leftSpace()) {
            expanded = true;
            p->expand(p->size + npend);
        }
        if (expanded && tailIdx < headIdx) {
            memcpy(p->buffer + oldSize, p->buffer, tailIdx);
            tailIdx = oldSize;
        }
        int nreadTotal = 0;
        int nread;
        // 2. read into ring buffer (slicely)
        do {
            nread = recv(sockfd, p->buffer + tailIdx, std::min(leftSpace(), (int)(p->size - tailIdx)), 0);
            if (nread == 0) {
                // closed
                return 0;
            }
            if (nread > 0) {
                nreadTotal += nread;
                tailIdx += nread;
                tailIdx = tailIdx & (p->size - 1);
            }
            if (nread < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                } else {
                    err_msg("[SockBuffer] sockfd %d readIn err: %d %s",
                            sockfd, errno, strerror (errno));
                }
            }
        } while(nread > 0);
        return nreadTotal;
    }
    
    bool hasPacketHeader() {
        return recvSize() >= sizeof(PacketHeader);
    }
    
    bool validatePacket() {
        Buffer* p = bufRef.get();
        uint8_t* buffer = p->buffer;
        PacketHeader* header = new(buffer + headIdx)PacketHeader();
        if (!header->isFlagOn(HeaderFlag::PacketLen))
        {
            return false;
        }
        // validate whether it is a legal packet
        return true;
    }
    
};

class SocketBase
{
protected:
  SocketBase() {}

public:
  sockaddr_in6 m_sockaddr;
  socklen_t m_socklen;
  int m_sockfd;
  int m_family;
  SockBuffer buf;
  bool isIPv4() { return m_family == PF_INET; }
  bool isIPv6() { return m_family == PF_INET6; }
};
};

#endif
