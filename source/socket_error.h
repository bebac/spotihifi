// ----------------------------------------------------------------------------
//
//        Filename:  socket_error.h
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __net__socket_error_h__
#define __net__socket_error_h__

// ----------------------------------------------------------------------------
#include <stdexcept>

// ----------------------------------------------------------------------------
namespace inet
{

// ----------------------------------------------------------------------------
class socket_error : public std::runtime_error
{
public:
    socket_error();
    socket_error(int errno_);
public:
    ~socket_error() noexcept;
public:
    virtual const char* what() const noexcept;
private:
    mutable std::string m_msg;
    int                 m_errno;
};

// ----------------------------------------------------------------------------
}

// ----------------------------------------------------------------------------
#endif // __net__socket_error_h_
