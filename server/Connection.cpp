#include "Connection.hpp"

std::string Connection::ReceiveString(size_t len)
{
    if (this->m_SslConnection == nullptr)
    {
        return m_ClientSocket.ReceiveString(len);
    }
    return this->m_SslConnection->Read(len);
}

bool Connection::Bad()
{
    if (this->m_SslConnection != nullptr && this->m_SslConnection->Bad())
    {
        return true;
    }
    return this->m_ClientSocket.Bad();
}

bool Connection::Eof()
{
    return this->m_ClientSocket.Eof();
}

void Connection::SendString(const std::string& str)
{
    if (this->m_SslConnection == nullptr)
    {
        m_ClientSocket.SendString(str);
        return;
    }

    m_SslConnection->Write(str);
}