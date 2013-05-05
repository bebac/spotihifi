// ----------------------------------------------------------------------------
//
//        Filename:  socket_imple.cpp
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#include <socket.h>
#include <socket_error.h>

#include <posix/socket_impl.h>

// ----------------------------------------------------------------------------
namespace inet
{

// ----------------------------------------------------------------------------
namespace tcp
{

// ----------------------------------------------------------------------------
namespace posix
{

// ----------------------------------------------------------------------------
socket_impl::socket_impl()
{
  m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
  if ( m_fd < 0 ) {
    throw socket_error(errno);
  }
}

// ----------------------------------------------------------------------------
socket_impl::socket_impl(int fd)
  :
  m_fd(fd)
{
}

// ----------------------------------------------------------------------------
socket_impl::~socket_impl() noexcept
{
  ::close(m_fd);
}

// ----------------------------------------------------------------------------
void socket_impl::bind(const struct sockaddr *addr, socklen_t addrlen)
{
  if ( ::bind(m_fd, addr, addrlen) < 0 ) {
    throw socket_error(errno);
  }
}

// ----------------------------------------------------------------------------
void socket_impl::listen(int backlog)
{
  if ( ::listen(m_fd, backlog) < 0 ) {
    throw socket_error(errno);
  }
}

// ----------------------------------------------------------------------------
socket socket_impl::accept(struct sockaddr *addr, socklen_t *addrlen)
{
  int s = ::accept(m_fd, addr, addrlen);
  if ( s < 0 ) {
    throw socket_error(errno);
  }
  return std::move(socket(s));
}

// ----------------------------------------------------------------------------
void socket_impl::connect(const struct sockaddr *serv_addr, socklen_t addrlen)
{
  if ( ::connect(m_fd, serv_addr, addrlen) < 0 ) {
    throw socket_error(errno);
  }
}

// ----------------------------------------------------------------------------
size_t socket_impl::recv(void *buf, size_t len, int flags)
{
  ssize_t received = ::recv(m_fd, buf, len, flags);
  if ( received < 0 ) {
    throw socket_error(errno);
  }
  return received;
}

// ----------------------------------------------------------------------------
size_t socket_impl::send(const void *buf, size_t len, int flags)
{
  ssize_t sent = ::send(m_fd, buf, len, flags);
  if ( sent < 0 ) {
    throw socket_error(errno);
  }
  return sent;
}

// ----------------------------------------------------------------------------
void socket_impl::nonblocking(bool value)
{
    int blocking = ( value ? 1 : 0 );

    if ( ::ioctl(m_fd, FIONBIO, (char*)&blocking) < 0 ) {
      throw socket_error(errno);
    }
}

// ----------------------------------------------------------------------------
int socket_impl::get_fd()
{
  return m_fd;
}

// ----------------------------------------------------------------------------
} // posix

// ----------------------------------------------------------------------------
} // tcp

// ----------------------------------------------------------------------------
} // net
