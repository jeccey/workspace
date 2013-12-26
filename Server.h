#ifndef LIBEVENT_SERVERH
#define LIBEVENT_SERVERH

/**
 *
 * Though be aware; if you need zero copy buffer handling you need 
 * to change the way we handle the buffers.
 *
 */

extern "C" {
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <event2/bufferevent.h>
	#include <event2/buffer.h>
	#include <event2/listener.h>
	#include <event2/util.h>
	#include <event2/event.h>
	#include <string.h>
	#include <signal.h>
	#include <errno.h>
}

#include "Connection.h"
#include <map>
using std::map;

template<class T>
class Server {
public:

	Server();
	~Server();
	
	bool setup(const unsigned short& port);
	void start();
	void update();
	void sendToAllClients(const char* data, size_t len);
	void addConnection(evutil_socket_t fd, T* connection);
	void removeConnection(evutil_socket_t fd);
		
	static void listenerCallback(
		 struct evconnlistener* listener
		,evutil_socket_t socket
		,struct sockaddr* saddr
		,int socklen
		,void* server
	);
	
	static void signalCallback(evutil_socket_t sig, short events, void* server);
	static void writeCallback(struct bufferevent*, void* server);
	static void readCallback(struct bufferevent*, void* connection);
	static void eventCallback(struct bufferevent*, short, void* server);
private:
	struct sockaddr_in sin;
	struct event_base* base;
	struct event* signal_event;
	struct evconnlistener* listener;
		
	map<evutil_socket_t, T*> connections;
};

template<class T> 
Server<T>::Server()
	:base(NULL)
	,listener(NULL)
	,signal_event(NULL)
{
}


template<class T> 
Server<T>::~Server() {
	if(signal_event != NULL) {
		event_free(signal_event);
	}
	
	if(listener != NULL) {
		evconnlistener_free(listener);
	}
	
	if(base != NULL) {
		event_base_free(base);
	}
}

template<class T>
bool Server<T>::setup(const unsigned short& port) {

	base = event_base_new();
	if(!base) {
		printf("Server: cannot create base.\n");
		return false;
	}
	
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	//ip??
	sin.sin_addr.s_addr = inet_addr("192.168.80.125");
	listener = evconnlistener_new_bind(
		 base
		,Server::listenerCallback
		,(void*)this
		,LEV_OPT_REUSEABLE|LEV_OPT_CLOSE_ON_FREE
		,-1
		,(struct sockaddr*)&sin
		,sizeof(sin)
	);
	
	if(!listener) {
		printf("Cannot create listener.\n");
		return false;
	}
	
	signal_event = evsignal_new(base, SIGINT, signalCallback, (void*)this);
	if(!signal_event || event_add(signal_event, NULL) < 0) {
		
		printf("Cannog create signal event.\n");
		return false;
	}
	return true;
}

template<class T>
void Server<T>::start() {
	if(base != NULL) {
		event_base_dispatch(base);
	}
}

template<class T>
void Server<T>::update() {
	if(base != NULL) {
		event_base_loop(base, EVLOOP_NONBLOCK);
	}
}

template<class T>
void Server<T>::addConnection(evutil_socket_t fd, T* connection) {
	connections.insert(std::pair<evutil_socket_t, T*>(fd, connection));
}

template<class T>
void Server<T>::removeConnection(evutil_socket_t fd) {
	connections.erase(fd);
}

template<class T>
void Server<T>::sendToAllClients(const char* data, size_t len) {
	typename map<evutil_socket_t, T*>::iterator it = connections.begin();
	while(it != connections.end()) {
		it->second->send(data, len);
		++it;
	}
}

// ------------------------------------

template<class T>
void Server<T>::listenerCallback(
	 struct evconnlistener* listener
	,evutil_socket_t fd
	,struct sockaddr* saddr
	,int socklen
	,void* data
)
{
	Server<T>* server = static_cast<Server<T>* >(data);
	struct event_base* base = (struct event_base*) server->base;
	struct bufferevent* bev;

	//bufferevent_socket_new创建基于套接字的bufferevent
	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	if(!bev) {
		event_base_loopbreak(base);
		printf("Error constructing bufferevent!\n");
		return;
	}
	
	T* conn = new T(fd, bev, (void*)server);
	server->addConnection(fd, conn);

	bufferevent_setcb(bev, Server::readCallback, Server::writeCallback, 
				Server::eventCallback, (void*)conn);
	bufferevent_enable(bev, EV_WRITE);
	bufferevent_enable(bev, EV_READ); 
}
	
template<class T>	
void Server<T>::signalCallback(evutil_socket_t sig, short events, void* data) {
	Server<T>* server = static_cast<Server<T> *>(data);
	struct event_base* base = server->base;
	struct timeval delay = {2,0};
	printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");
	event_base_loopexit(base, &delay);
}

template<class T>
void Server<T>::writeCallback(struct bufferevent* bev, void* data) {
	struct evbuffer* outputbuf = bufferevent_get_output(bev);
	if(evbuffer_get_length(outputbuf) == 0) {

	}
	
	printf("write callback.\n");
}

template<class T>
void Server<T>::readCallback(struct bufferevent* bev, void* connection) {
	T* conn = static_cast<T*>(connection);
	struct evbuffer* inputbuf = bufferevent_get_input(bev);
	char readbuf[1024];
	size_t readn = 0;

	evutil_socket_t fd = bufferevent_getfd(bev);
	//copy all the data from input buffer to the out buffer
	//struct evbuffer* outputbuf = bufferevent_get_output(bev);
	//evbuffer_add_buffer(outputbuf, inputbuf);
	while( (readn = evbuffer_remove(inputbuf, &readbuf, sizeof(readbuf))) > 0) {
		conn->onRead(readbuf, readn);
		readbuf[readn] = '\0';
		printf("fd=%u, recv data: %s\n", fd, readbuf);
	}
}

template<class T>
void Server<T>::eventCallback(struct bufferevent* bev, short events, void* data) {
	T* conn = static_cast<T*>(data);
	Server<T>* server = static_cast<Server<T>* >(conn->server);
	
	if(events & BEV_EVENT_EOF) {
		server->removeConnection(conn->fd);
		bufferevent_free(bev);
		printf("ref: %p, Connection closed.\n", server);
	}
	else if(events & BEV_EVENT_ERROR) {
		printf("Got an error on the connection: %s\n", strerror(errno));
	}
	else {
		printf("unhandled.\n");
	}
}

#endif
