// ----------------------------------------------------------------------------
//
//     Filename:  inet_socket_address.h
//
//  Description:
//
//       Author:  Benny Bach
//      Company:
//
// ----------------------------------------------------------------------------
#ifndef __inet_socket_address_h__
#define __inet_socket_address_h__

// ----------------------------------------------------------------------------
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>

// ----------------------------------------------------------------------------
namespace inet 
{ 

// ----------------------------------------------------------------------------
class socket_address
{
public:
    socket_address() {}
    socket_address(const char* ip, unsigned port);
public:
    unsigned port();
public:
    std::string ip();
public:
    const struct sockaddr* sockaddr_ptr() const;
    struct sockaddr* sockaddr_ptr();
public:
    size_t sockaddr_len() const;
private:
    struct sockaddr_in m_sockaddr;
};

// ----------------------------------------------------------------------------
inline socket_address::socket_address(const char* ip, unsigned port)
{
    m_sockaddr.sin_family = AF_INET;
    m_sockaddr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &m_sockaddr.sin_addr);
}

// ----------------------------------------------------------------------------
inline unsigned socket_address::port()
{
    return ntohs(m_sockaddr.sin_port);
}

// ----------------------------------------------------------------------------
inline std::string socket_address::ip()
{
    char dst[128];
    if ( inet_ntop(AF_INET, &m_sockaddr.sin_addr, dst, sizeof(dst)) == 0 ) {
        throw std::runtime_error("inet_ntop failed!");
    }
    return std::move(std::string(dst));
}

// ----------------------------------------------------------------------------
inline const struct sockaddr* socket_address::sockaddr_ptr() const
{
    return reinterpret_cast<const struct sockaddr*>(&m_sockaddr);
}

// ----------------------------------------------------------------------------
inline struct sockaddr* socket_address::sockaddr_ptr()
{
    return reinterpret_cast<struct sockaddr*>(&m_sockaddr);
}

// ----------------------------------------------------------------------------
inline size_t socket_address::sockaddr_len() const
{
    return sizeof(m_sockaddr);
}

// ----------------------------------------------------------------------------
} // inet

#endif // __inet_socket_address_h__
