/*
    https://github.com/TheNNX/InetSocketWrapper/

    MIT License

    Copyright (c) 2022 Artur Kręgiel, Marcin Jabłoński

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#include <iostream>
#include <string>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "inet-socket-wrapper.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define close(x) closesocket(x)

#ifdef gai_strerror
#undef gai_strerror
#endif
#define gai_strerror(x) gai_strerrorA(x)

static std::string GetLastStringError()
{
    return std::to_string(WSAGetLastError());
}

WSADATA InetSocketWrapper::WsaReference::wsaData = { 0 };
int InetSocketWrapper::WsaReference::wsaReferenced = 0;

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

static std::string GetLastStringError()
{
    return std::string(strerror(errno));
}
#endif

namespace InetSocketWrapper
{
    // tworzenie gniazda do odbierania
    bool InetSocket::BindToAddress(const char* host, const char* port)
    {
        if (this->sockfd > 0)
            close(this->sockfd);

        struct addrinfo hints, * result, * p;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = this->domain;
        hints.ai_socktype = this->type;
        hints.ai_flags = AI_PASSIVE;
        hints.ai_protocol = this->protocol;
        hints.ai_canonname = NULL;
        hints.ai_addr = NULL;
        hints.ai_next = NULL;

        int gai_err;
        if ((gai_err = getaddrinfo(host, port, &hints, &result)) != 0)
        {
            std::string err = std::string("getaddrinfo: ") + std::string(gai_strerror(gai_err));
            freeaddrinfo(result);
            throw std::runtime_error(err);
        }

        for (p = result; p != NULL; p = p->ai_next)
        {
            this->sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (this->sockfd < 0) /* utworzenie gniazda do odbierania */
                continue;

            int reuse = 1;
            if (setsockopt(this->sockfd, SOL_SOCKET, SO_REUSEADDR, (char*) &reuse, sizeof(reuse)) < 0)
            {
                std::string err = std::string("setsockopt: ") + GetLastStringError();
                freeaddrinfo(result);
                throw std::runtime_error(err);
            }


            /* powiazanie gniazda z adresem IP i portem */
            if (bind(this->sockfd, p->ai_addr, p->ai_addrlen) == 0)
            {
                break;
            }

            close(this->sockfd);
        }

        if (p == NULL)
        {
            freeaddrinfo(result);
            return false;
        }

        freeaddrinfo(result);

        return true;
    }

    // tworzenie gniazda do wysylania
    bool InetSocket::ConnectToRemote(const char* addr, const char* port)
    {
        if (this->sockfd > 0)
            close(this->sockfd);

        struct addrinfo hints, * result, * p;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = this->domain;
        hints.ai_socktype = this->type;
        hints.ai_flags = 0;
        hints.ai_protocol = this->protocol;

        int gai_err;
        if ((gai_err = getaddrinfo(addr, port, &hints, &result)) != 0)
        {
            std::string err = std::string("getaddrinfo: ") + std::string(gai_strerror(gai_err));
            freeaddrinfo(result);
            throw std::runtime_error(err);
        }

        for (p = result; p != NULL; p = p->ai_next)
        {
            this->sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (this->sockfd < 0) /* utworzenie gniazda do wysylania */
                continue;

            if (connect(this->sockfd, p->ai_addr, p->ai_addrlen) == 0)
            {
                break;
            }

            close(this->sockfd);
        }

        if (p == NULL)
        {
            freeaddrinfo(result);
            return false;
        }

        freeaddrinfo(result);

        return true;
    }

    sockaddr* InetSocket::GetRemoteHostInfo(const char* addr, const char* port)
    {
        size_t reslen;

        if (this->domain == IPv4)
        {
            reslen = sizeof(sockaddr_in);
        }
        else if (this->domain == IPv6)
        {
            reslen = sizeof(sockaddr_in6);
        }
        sockaddr* res = (sockaddr*) malloc(reslen);
        if (res == NULL)
        {
            throw std::runtime_error("malloc: Allocation failed.");
        }

        struct addrinfo hints, * result, * p;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = this->domain;
        hints.ai_socktype = this->type;
        hints.ai_flags = 0;
        hints.ai_protocol = this->protocol;

        int gai_err;
        if ((gai_err = getaddrinfo(addr, port, &hints, &result)) != 0)
        {
            std::string err = std::string("getaddrinfo: ") + std::string(gai_strerror(gai_err));
            freeaddrinfo(result);
            throw std::runtime_error(err);
        }

        for (p = result; p != NULL; p = p->ai_next)
        {
            SocketDescriptor sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
            if (sock < 0) /* utworzenie gniazda do wysylania */
                continue;

            if (connect(sock, p->ai_addr, p->ai_addrlen) == 0)
            {
                memcpy(res, p->ai_addr, p->ai_addrlen);
                close(sock);
                break;
            }

            close(sock);
        }

        freeaddrinfo(result);

        return res;
    }

    int InetSocket::Send(const byte* data, size_t len, int flags)
    {
        int n;
        if ((n = send(this->sockfd, (char*) data, len, flags)) < 0)
        {
            std::string err = std::string("send: ") + GetLastStringError();
            throw std::runtime_error(err);
        }
        if (n == 0)
        {
            eof = true;
        }
        return n;
    }

    int InetSocket::Receive(byte* buffer, size_t len, int flags)
    {
        memset(buffer, 0, len);
        int n;
        if ((n = recv(this->sockfd, (char*) buffer, len, flags)) < 0)
        {
            std::string err = std::string("recv: ") + GetLastStringError();
            throw std::runtime_error(err);
        }
        if (n == 0)
        {
            eof = true;
        }
        return n;
    }

    int InetSocket::SendTo(const byte* data, size_t len, sockaddr* addr, socklen_t addrlen, int flags)
    {
        int n;
        if ((n = sendto(this->sockfd, (char*) data, len, flags, addr, addrlen)) < 0)
        {
            std::string err = std::string("sendto: ") + GetLastStringError();
            throw std::runtime_error(err);
        }
        return n;
    }

    int InetSocket::ReceiveFrom(byte* buffer, size_t len, sockaddr* addr, socklen_t& addrlen, int flags)
    {
        memset(buffer, 0, len);
        int n;
        if ((n = recvfrom(this->sockfd, (char*) buffer, len, flags, addr, &addrlen)) < 0)
        {
            std::string err = std::string("recvfrom: ") + GetLastStringError();
            throw std::runtime_error(err);
        }
        return n;
    }

    SocketDescriptor InetSocket::Accept(sockaddr* addr, socklen_t& addrlen)
    {
        SocketDescriptor newsockfd;
        if ((newsockfd = accept(this->sockfd, addr, &addrlen)) < 0)
        {
            std::string err = std::string("accept: ") + GetLastStringError();
            throw std::runtime_error(err);
        }

        return newsockfd;
    }


    InetSocket::InetSocket(InternetProtocol ip, SocketType type)
    {
        if (type == TCP || type == UDP)
        {
            this->type = type;
        }
        else
        {
            throw std::runtime_error("InetSocket: Invalid socket type");
        }

        if (ip == IPv4 || ip == IPv6)
        {
            this->domain = ip;
        }
        else
        {
            throw std::runtime_error("InetSocket: Invalid Internet Protocol");
        }
    }

    InetListenSocket::InetListenSocket(
        const std::string& ip,
        uint16_t port,
        InternetProtocol protocol,
        SocketType type,
        int backlog) :
        InetSocket(protocol, type)
    {
        if (!Bind(SocketAddress{ ip, port }))
        {
            close(this->sockfd);
            this->sockfd = INVALID_SOCKET;
            throw std::runtime_error("");
        }

        Listen(backlog);
    }

    InetSocket::InetSocket(InetSocket&& rvRef) noexcept :
        domain(rvRef.domain),
        sockfd(rvRef.sockfd),
        type(rvRef.type)
    {
        rvRef.sockfd = INVALID_SOCKET;
    }

    InetSocket::InetSocket(SocketDescriptor sockd) : sockfd(sockd) {}

    InetSocket::~InetSocket()
    {
        Close();
    }

    bool InetSocket::CreateSocketDescriptor()
    {
        SocketDescriptor sock = socket(this->domain, this->type, this->protocol);
        if (sock < 0)
        {
            return false;
        }

        this->sockfd = sock;
        return true;
    }

    bool InetSocket::CheckDataReady()
    {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sockfd, &rfds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        int active = select(sockfd + 1, &rfds, NULL, NULL, &tv);
        return active != 0;
    }

    bool InetSocket::Bind(SocketAddress addr)
    {
        char port[6];
        snprintf(port, 6, "%d", addr.port);

        return BindToAddress(addr.host.c_str(), port);
    }

    bool InetSocket::Connect(SocketAddress addr)
    {
        char port[6];
        snprintf(port, 6, "%d", addr.port);

        return ConnectToRemote(addr.host.c_str(), port);
    }

    void InetSocket::Listen(int backlog)
    {
        if (listen(this->sockfd, backlog) < 0)
        {
            std::string err = std::string("listen: ") + GetLastStringError();
            throw std::runtime_error(err);
        }
    }

    void InetSocket::Close()
    {
        close(this->sockfd);
        sockfd = -1;
    }

    InetSocket InetSocket::AcceptConnection(SocketAddress& ClientAddress)
    {
        SocketDescriptor newsockfd;

        if (this->domain == IPv4)
        {
            sockaddr_in addr;
            socklen_t addrlen = sizeof(addr);

            newsockfd = Accept((sockaddr*) &addr, addrlen);

            char addrstr[INET_ADDRSTRLEN];

            inet_ntop(this->domain, &addr.sin_addr.s_addr, addrstr, INET_ADDRSTRLEN);
            std::string s(addrstr);

            ClientAddress.host = s;
            ClientAddress.port = ntohs(addr.sin_port);
        }
        else if (this->domain == IPv6)
        {
            sockaddr_in6 addr;
            socklen_t addrlen = sizeof(addr);

            newsockfd = Accept((sockaddr*) &addr, addrlen);

            char addrstr[INET6_ADDRSTRLEN];

            inet_ntop(this->domain, &addr.sin6_addr.s6_addr, addrstr, INET6_ADDRSTRLEN);
            std::string s(addrstr);

            ClientAddress.host = s;
            ClientAddress.port = ntohs(addr.sin6_port);
        }

        return InetSocket(newsockfd);
    }

    bool InetSocket::SendBytes(const byte* data, size_t len, int flags)
    {
        return Send(data, len, flags);
    }

    int InetSocket::SendString(const std::string& data, int flags)
    {
        return Send((byte*) data.c_str(), data.length(), flags);
    }

    int InetSocket::ReceiveBytes(byte* buffer, size_t len, int flags)
    {
        return Receive(buffer, len, flags);
    }

    std::string InetSocket::ReceiveString(size_t len, int flags)
    {
        byte* buf = new byte[len];
        std::string buffer = "";
        int result = Receive(buf, len, flags);
        if (result)
        {
            std::string s((const char*) buf);
            buffer = s;
        }

        delete[] buf;
        return buffer;
    }

    int InetSocket::SendBytesTo(byte* data, size_t len, SocketAddress target, int flags)
    {
        char port[6];
        snprintf(port, 6, "%d", target.port);

        if (this->domain == IPv4)
        {
            sockaddr_in* addr = (sockaddr_in*) GetRemoteHostInfo(target.host.c_str(), port);
            socklen_t addrlen = sizeof(sockaddr_in);
            int n = SendTo(data, len, (sockaddr*) addr, addrlen, flags);
            free(addr);
            return n;
        }
        else if (this->domain == IPv6)
        {
            sockaddr_in6* addr = (sockaddr_in6*) GetRemoteHostInfo(target.host.c_str(), port);
            socklen_t addrlen = sizeof(sockaddr_in6);
            int n = SendTo(data, len, (sockaddr*) addr, addrlen, flags);
            free(addr);
            return n;
        }

        return -1;
    }

    int InetSocket::SendStringTo(const std::string& data, SocketAddress target, int flags)
    {
        return SendBytesTo((byte*) data.c_str(), data.length(), target, flags);
    }

    int InetSocket::ReceiveBytesFrom(byte* buffer, size_t len, SocketAddress& source, int flags)
    {
        int n;
        if (this->domain == IPv4)
        {
            sockaddr_in addr;
            socklen_t addrlen = sizeof(addr);
            n = ReceiveFrom(buffer, len, (sockaddr*) &addr, addrlen, flags);

            char addrstr[INET_ADDRSTRLEN];

            inet_ntop(this->domain, &addr.sin_addr.s_addr, addrstr, INET_ADDRSTRLEN);
            std::string s(addrstr);

            source.host = s;
            source.port = ntohs(addr.sin_port);
        }
        else if (this->domain == IPv6)
        {
            sockaddr_in6 addr;
            socklen_t addrlen = sizeof(addr);
            n = ReceiveFrom(buffer, len, (sockaddr*) &addr, addrlen, flags);

            char addrstr[INET6_ADDRSTRLEN];

            inet_ntop(this->domain, &addr.sin6_addr.s6_addr, addrstr, INET6_ADDRSTRLEN);
            std::string s(addrstr);

            source.host = s;
            source.port = ntohs(addr.sin6_port);
        }

        return n;
    }

    std::string InetSocket::ReceiveStringFrom(size_t len, SocketAddress& source, int flags)
    {
        byte* buf = new byte[len];
        ReceiveBytesFrom(buf, len, source, flags);
        return std::string((char*) buf);
    }

    bool InetSocket::Bad()
    {
        return this->sockfd < 0;
    }

    bool InetSocket::Eof()
    {
        return this->eof;
    }

#ifdef _WIN32
    WsaReference::WsaReference()
    {
        if (wsaReferenced == 0)
        {
            int err;
            err = WSAStartup(MAKEWORD(2, 2), &wsaData);
            if (err != 0)
            {
                std::string error = std::string("WSAStartup failed with error: ");
                error += std::to_string(err);
                throw std::runtime_error(error.c_str());
            }
        }
        wsaReferenced++;
    }

    WsaReference::~WsaReference()
    {
        assert(wsaReferenced > 0);
        wsaReferenced--;
        if (wsaReferenced == 0)
        {
            WSACleanup();
        }
    }

    WsaReference::WsaReference(const WsaReference&)
    {
        wsaReferenced++;
    }

    void InetSocket::DisableNagle()
    {
        int yes = 1;
        int result = setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*) &yes, sizeof(int));
        /* TODO */
    }
#endif
}