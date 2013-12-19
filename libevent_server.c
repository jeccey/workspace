 /**
 * Usage:
 * 1. Create a new class which inherits from Connection.
 * 2. Create a server instance: Server<YourConnectionClass> server;
 * 3. Call setup with a port to listen on: server.setup(1234)
 * 4. Call update regurlarly to update the event list
 *
 */
 

class MyConnection : public Connection {    
	public:
 		MyConnection(evutil_socket_t fd, struct bufferevent* bev, void* server) 
 			:Connection(fd, bev, server)
 		{
 		}
 		
 		void onRead(const char* data, const size_t& numBytes) {
 			const char* msg = "hoi\n";
 			send(msg,strlen(msg));
 		}
};

int main(void)
{
	evutil_socket_t sockfd;
    struct bufferevent *bev;

	MyConnection myconnec();

	return 0;
}
 
