// ----------------------------------------------------------------------------
//
//        Filename:  socket_imple.h
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __net__posix__socket_impl_h__
#define __net__posix__socket_impl_h__

// ----------------------------------------------------------------------------
namespace inet
{

namespace tcp
{

class socket;

// ----------------------------------------------------------------------------
namespace posix
{

// ----------------------------------------------------------------------------
class socket_impl
{
public:
  socket_impl();
  socket_impl(int fd);
public:
  virtual ~socket_impl() noexcept;
public:
  void   bind(const struct sockaddr *addr, socklen_t addrlen);
  void   listen(int backlog);
  inet::tcp::socket accept(struct sockaddr *addr, socklen_t *addrlen);
  void   connect(const struct sockaddr *serv_addr, socklen_t addrlen);
  size_t recv(void *buf, size_t len, int flags);
  size_t send(const void *buf, size_t len, int flags);
public:
  void nonblocking(bool value);
public:
  int get_fd();
private:
  int m_fd;
};

// ----------------------------------------------------------------------------
} // posix

} // tcp

} // net

// ----------------------------------------------------------------------------
#endif // __net__posix__socket_impl_h__
