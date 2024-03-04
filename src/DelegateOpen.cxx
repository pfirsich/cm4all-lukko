// SPDX-License-Identifier: BSD-2-Clause
// Copyright CM4all GmbH
// author: Max Kellermann <mk@cm4all.com>

#include "DelegateOpen.hxx"
#include "Connection.hxx"
#include "spawn/Interface.hxx"
#include "spawn/Prepared.hxx"
#include "spawn/ProcessHandle.hxx"
#include "event/AwaitableSocketEvent.hxx"
#include "net/EasyMessage.hxx"
#include "net/SocketError.hxx"
#include "net/SocketPair.hxx"
#include "net/SocketProtocolError.hxx"
#include "io/Open.hxx"
#include "io/UniqueFileDescriptor.hxx"
#include "co/Task.hxx"
#include "util/SpanCast.hxx"

static int
OpenFunction(PreparedChildProcess &&)
{
	SocketDescriptor control{3};
	char path[4096];

	auto nbytes = control.Receive(std::as_writable_bytes(std::span{path}));
	if (nbytes < 0)
		throw MakeSocketError("Failed to receive");

	if (nbytes == 0)
		throw SocketClosedPrematurelyError{};

	if (static_cast<std::size_t>(nbytes) >= sizeof(path))
		throw SocketBufferFullError{};

	path[nbytes] = 0;

	auto fd = OpenReadOnly(path);
	EasySendMessage(control, fd);

	return 0;
}

static std::pair<UniqueSocketDescriptor, std::unique_ptr<ChildProcessHandle>>
SpawnOpen(const Connection &ssh_connection)
{
	// TODO this is a horrible and inefficient kludge
	auto [control_socket, control_socket_for_child] = CreateSocketPair(SOCK_SEQPACKET);

	PreparedChildProcess p;
	p.exec_function = OpenFunction;
	p.args.push_back("dummy");

	/* using SFTP mode because this (usually) mounts an empty
	   rootfs; minimalism! */
	ssh_connection.PrepareChildProcess(p, true);

	if (const char *home = p.ns.mount.GetJailedHome())
		p.chdir = home;

	p.SetControl(std::move(control_socket_for_child));

	return {
		std::move(control_socket),
		ssh_connection.GetSpawnService().SpawnChildProcess("connect", std::move(p)),
	};
}

static void
SendOpen(SocketDescriptor s, std::string_view path)
{
	const auto nbytes = s.Send(AsBytes(path));
	if (nbytes < 0)
		throw MakeSocketError("Failed to send");
}

Co::Task<UniqueFileDescriptor>
DelegateOpen(const Connection &ssh_connection, std::string_view path)
{
	auto [control_socket, child_handle] = SpawnOpen(ssh_connection);

	SendOpen(control_socket, path);

	co_await AwaitableSocketEvent(ssh_connection.GetEventLoop(),
				      control_socket, SocketEvent::READ);

	auto fd = EasyReceiveMessageWithOneFD(control_socket);
	if (!fd.IsDefined())
		throw std::runtime_error{"Bad number of fds"};

	co_return fd;
}
