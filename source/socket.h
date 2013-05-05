// ----------------------------------------------------------------------------
//
//        Filename:  socket.h
//
//          Author:  Benny Bach
//
// --- Description: -----------------------------------------------------------
//
//
// ----------------------------------------------------------------------------
#ifndef __net_socket_h__
#define __net_socket_h__

// ----------------------------------------------------------------------------
#include <socket_error.h>
#include <inet_socket_address.h>
#include <posix/socket_impl.h>

// ----------------------------------------------------------------------------
#include <memory>

// ----------------------------------------------------------------------------
using inet::tcp::posix::socket_impl;

// ----------------------------------------------------------------------------
namespace inet
{

// ----------------------------------------------------------------------------
namespace tcp
{

// ----------------------------------------------------------------------------
class socket
{
public:
	socket();
	socket(socket&& other);
	socket(int fd);
private:
	socket(const socket& other) = delete;
public:
	virtual ~socket() noexcept;
public:
	void   bind(const struct sockaddr *addr, socklen_t addrlen);
	void   bind(const socket_address& addr);
	void   listen(int backlog);
	socket accept(struct sockaddr *addr, socklen_t *addrlen);
	socket accept(socket_address& remote_addr);
	void   connect(const struct sockaddr *serv_addr, socklen_t addrlen);
	void   connect(const socket_address& addr);
	size_t recv(void *buf, size_t len, int flags);
	size_t send(const void *buf, size_t len, int flags);
public:
	void nonblocking(bool value);
public:
	void close()
	{
		pimpl.reset();
	}
public:
	int get_fd();
private:
	std::unique_ptr<socket_impl> pimpl;
};

// ----------------------------------------------------------------------------
} // tcp

// ----------------------------------------------------------------------------
} // net

#endif // __net_socket_h__
