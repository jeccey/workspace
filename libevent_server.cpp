#include "Server.h"
#include "Connection.h"
#include <iostream>
class MyConnection : public Connection {
    public:
	MyConnection(evutil_socket_t fd, struct bufferevent * bev, void* server)
		:Connection(fd, bev, server)
	{}

	void onRead(const char* data, const size_t& numBytes) {
		const char*msg = "server recved message:\n";
		send(msg, strlen(msg));
		send(data, numBytes);
	}
};

int main()
{
	const unsigned short port = 1234;

	MyConnection myconnec();
	Server<MyConnection> server;
	server.setup(port);
	server.start();

	server.update();

	return 0;
}

