// ----------------------------------------------------------------------------
//
//     Filename:  socket_error.cpp
//
//  Description:
//
//       Author:  Benny Bach
//      Company:
//
// ---------------------------------------------------------------------------
#include <sstream>
#include <string.h>

#include <socket_error.h>

// ----------------------------------------------------------------------------
namespace inet
{

// ----------------------------------------------------------------------------
socket_error::socket_error()
try
    : std::runtime_error("socket_error"), m_msg(), m_errno(0)
{
}
catch(...) {
    // Not much we can do.
}

// ----------------------------------------------------------------------------
socket_error::socket_error(int errno_)
try
    : std::runtime_error("socket_error"), m_msg(), m_errno(errno_)
{
}
catch(...) {
    // Not much we can do.
}

// ----------------------------------------------------------------------------
socket_error::~socket_error() noexcept
{
}

// ----------------------------------------------------------------------------
const char* socket_error::what() const noexcept
{
    try
    {
        std::stringstream e;
        e << strerror(errno);
        return (m_msg = std::move(e.str())).c_str();
    }
    catch (...) {
        return "socket error : <unknown>";
    }
}

// ----------------------------------------------------------------------------
} // inet