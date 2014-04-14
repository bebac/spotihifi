// ----------------------------------------------------------------------------
//
//        Filename:  socket.cpp
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------

#include <socket.h>
#include <assert.h>

// ----------------------------------------------------------------------------
namespace inet
{

namespace tcp
{

// ----------------------------------------------------------------------------
socket::socket()
	:
  pimpl(new socket_impl())
{
}

// ----------------------------------------------------------------------------
socket::socket(socket&& other)
	:
  pimpl(std::move(other.pimpl))
{
}

// ----------------------------------------------------------------------------
socket::socket(int fd)
	:
  pimpl(new socket_impl(fd))
{
}

// ----------------------------------------------------------------------------
socket::~socket() noexcept
{
}

// ----------------------------------------------------------------------------
void socket::bind(const struct sockaddr *addr, socklen_t addrlen)
{
	assert(pimpl.get());
	pimpl->bind(addr, addrlen);
}

// ----------------------------------------------------------------------------
void socket::bind(const socket_address& addr)
{
  bind(addr.sockaddr_ptr(), addr.sockaddr_len());
}

// ----------------------------------------------------------------------------
void socket::listen(int backlog)
{
	assert(pimpl.get());
	pimpl->listen(backlog);
}

// ----------------------------------------------------------------------------
socket socket::accept(struct sockaddr *addr, socklen_t *addrlen)
{
	assert(pimpl.get());
	return pimpl->accept(addr, addrlen);
}

// ----------------------------------------------------------------------------
socket socket::accept(socket_address& remote_addr)
{
  socklen_t remote_addr_len = remote_addr.sockaddr_len();
  return accept(remote_addr.sockaddr_ptr(), &remote_addr_len);
}

// ----------------------------------------------------------------------------
void socket::connect(const struct sockaddr *serv_addr, socklen_t addrlen)
{
	assert(pimpl.get());
	pimpl->connect(serv_addr, addrlen);
}

// ----------------------------------------------------------------------------
void socket::connect(const socket_address& addr)
{
  connect(addr.sockaddr_ptr(), addr.sockaddr_len());
}

// ----------------------------------------------------------------------------
size_t socket::recv(void *buf, size_t len, int flags)
{
	assert(pimpl.get());
	return pimpl->recv(buf, len, flags);
}

// ----------------------------------------------------------------------------
size_t socket::send(const void *buf, size_t len, int flags)
{
	assert(pimpl.get());
	return pimpl->send(buf, len, flags);
}

// ----------------------------------------------------------------------------
void socket::nonblocking(bool value)
{
	assert(pimpl.get());
	pimpl->nonblocking(value);
}

// ----------------------------------------------------------------------------
void socket::reuseaddr(bool value)
{
    assert(pimpl.get());
    pimpl->reuseaddr(value);
}

// ----------------------------------------------------------------------------
int socket::get_fd()
{
  assert(pimpl.get());
  return pimpl->get_fd();
}

} // tcp

} // net
