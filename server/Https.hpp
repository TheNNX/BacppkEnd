#pragma once

#include <memory>
#include <openssl/tls1.h>
#include <openssl/ssl.h>
#include "InetSocketWrapper.h"
#include <stdexcept>

struct SslContext;
struct SslConnection;

struct SslContext
{
    SslContext(const std::string& certFile, const std::string& keyFile)
    {
        m_Ctx = SSL_CTX_new(TLS_server_method());
        if (!m_Ctx)
        {
            throw std::runtime_error("Unable to create SSL context");
        }

        if (SSL_CTX_use_certificate_file(m_Ctx, certFile.c_str(), SSL_FILETYPE_PEM) <= 0)
        {
            throw std::runtime_error("Unable to use the certificate file");
        }

        if (SSL_CTX_use_PrivateKey_file(m_Ctx, keyFile.c_str(), SSL_FILETYPE_PEM) <= 0)
        {
            throw std::runtime_error("Unable to use the private key file");
        }
    }

    ~SslContext()
    {
        if (m_Ctx != nullptr)
        {
            SSL_CTX_free(m_Ctx);
            m_Ctx = nullptr;
        }
    }

private:
    SSL_CTX* m_Ctx;

    friend struct SslConnection;
};

struct SslConnection
{
    SslConnection(const SslContext& context, 
                  InetSocketWrapper::InetSocket& clientSocket)
    {
        m_Ssl = SSL_new(context.m_Ctx);

        if (m_Ssl == nullptr)
        {
            throw std::runtime_error("Couldn't allocate SSL connection");
        }

        if (SSL_set_fd(m_Ssl, clientSocket.GetNativeDescriptor()) <= 0)
        {
            throw std::runtime_error("Couldn't set file descriptor for SSL connection");
        }

        if (SSL_accept(m_Ssl) <= 0)
        {
            throw std::runtime_error("Couldn't accept SSL connection");
        }
    }

    ~SslConnection()
    {
        if (m_Ssl != nullptr)
        {
            SSL_shutdown(m_Ssl);
            SSL_free(m_Ssl);
            m_Ssl = nullptr;
        }
    }

    void Write(const std::string& str)
    {
        auto curSize = SSL_write(m_Ssl, str.c_str(), str.length());
        if (curSize <= 0)
        {
            m_Bad = true;
        }
    }

    std::string Read(size_t maxSize = 8192)
    {
        std::unique_ptr<char[]> buffer = std::make_unique<char[]>(maxSize);

        auto curSize = SSL_read(m_Ssl, buffer.get(), maxSize);
        if (curSize <= 0)
        {
            m_Bad = true;
            return "";
        }

        return std::string(buffer.get(), buffer.get() + curSize);
    }

    bool Bad()
    {
        return m_Bad;
    }
private:
    SSL* m_Ssl;
    bool m_Bad = false;
};