#pragma once

#include "Https.hpp"

#include "InetSocketWrapper.h"

struct Connection
{
    Connection(InetSocketWrapper::InetSocket&& socket,
               InetSocketWrapper::SocketAddress clientAddress,
               std::unique_ptr<SslContext>& sslContext) :
               m_ClientSocket(std::move(socket)),
               m_ClientAddress(clientAddress)
    {
        if (sslContext != nullptr)
        {
            m_SslConnection = std::make_unique<SslConnection>(*sslContext, m_ClientSocket);
        }
    }

    Connection(Connection&& connection) = default;

    bool Bad();
    
    bool Eof();

    void SendString(const std::string& str);
    
    std::string ReceiveString(size_t len = 8192);

    InetSocketWrapper::SocketAddress GetAddress() 
    {
        return this->m_ClientAddress;
    }
private:
    InetSocketWrapper::InetSocket m_ClientSocket;
    const InetSocketWrapper::SocketAddress m_ClientAddress;
    std::unique_ptr<SslConnection> m_SslConnection = nullptr;
};