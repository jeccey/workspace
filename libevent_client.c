#include <event.h>
#include <event2/event.h>
#include <event2/bufferevent.h>

#include <sys/socket.h>
#include <netinet/tcp.h>

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

int64_t total_bytes_read = 0;
int64_t total_messages_read = 0;

/* 发生了致命错误，输入错误信息，退出程序 */
void error_quit(const char *str)
{
    /*  perror()将上一个函数发生错误的原因输出到标准错误(stderr);
        参数 s 所指的字符串会先打印出,后面再加上错误原因字符串;
        此错误原因依照全局变量 errno 的值来决定要输出的字符串。
    */
    perror(str);
    exit(1);
}

static void set_tcp_no_delay(evutil_socket_t fd)
{
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof one);
}

static void timeoutcb(evutil_socket_t fd, short what, void *arg)
{
    struct event_base *base = arg;
    printf("timeout\n");

    event_base_loopexit(base, NULL);
}

/* 事件回调函数，连接状态改变时回调的函数 */
void event_cb(struct bufferevent *bev, short events, void *ptr)
{
    struct event_base *tbase = (struct event_base*)ptr;
    if ( events & BEV_EVENT_CONNECTED){
	/* We're connected to server. Ordinarily we'd do
           something here, like start reading or writing. */
        evutil_socket_t fd = bufferevent_getfd(bev);
        set_tcp_no_delay(fd);
	    printf("Server is connected!\n");
    }else { //如果连接不成功的消息，就停止事件循环
    	bufferevent_free(bev);
    	event_base_loopbreak(tbase);
    	printf("Server connect erro!! or The connect have been shutdown: %X\n", events);
    }
}

/* 服务器传信息过来了
   this callback is invoked when there is data to read on bev.
*/
void sock_readcb(struct bufferevent *bev, void *ptr)
{
    struct evbuffer *input = bufferevent_get_input(bev);
    evbuffer_write(input, STDOUT_FILENO);

	++total_messages_read;
    total_bytes_read += evbuffer_get_length(input);
}

/* 标准输入传消息过来了 */
void std_readcb(struct bufferevent *bev, void *ptr)
{
    struct bufferevent *sockbev = (struct bufferevent*)ptr;
    struct evbuffer *input = bufferevent_get_input(bev);

    bufferevent_write_buffer(sockbev, input);
}



int main(int argc, char **argv)
{
    struct sockaddr_in servaddr;
    int res;
    int timeout_seconds;

    struct event_base *base;
    struct event *evtimeout;
    struct timeval timeout;
    struct bufferevent *sockbev;
    struct bufferevent *stdbev;

    if( argc != 3 ){
    	error_quit("Using: mytelnet <IP Address> <Port>");
    }
    /* 初始化连接地址 */
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons( atoi(argv[2]) );
    /*inet_pton 处理IPv4,将“点分十进制” －> “整数 */
    res = inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
    if( res != 1 ){
    	error_quit("inet_pton error");
    }

    timeout_seconds = 60;
    timeout.tv_sec = timeout_seconds;
    timeout.tv_usec = 0;
    base = event_base_new();
	if (!base) {
        error_quit("Couldn't open event base");
    }
    evtimeout = evtimer_new(base, timeoutcb, base);
    evtimer_add(evtimeout, &timeout);

    /* 连接服务器并监听 */
       //bufferevent_socket_new创建基于套接字的bufferevent， 参数为-1 表示并不为此 bufferevent 设置 socket
    if ((sockbev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE)) == NULL){
    	error_quit("bufferevent_socket_new error");
    }
      // 建立连接,并将socket设置为非阻塞,连接成功返回 0 失败返回 -1
    res = bufferevent_socket_connect(sockbev, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if ( res < 0 ){
	bufferevent_free(sockbev);
        error_quit("connect error");
    }
    //为bufferevent event设置回调函数
    bufferevent_setcb(sockbev, sock_readcb, NULL, event_cb, (void*)base);
    //启用读写操作
    bufferevent_enable(sockbev, EV_READ);
    bufferevent_enable(sockbev, EV_WRITE);

    //监听标准输入
    stdbev = bufferevent_socket_new(base, STDIN_FILENO, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(stdbev, std_readcb, NULL, NULL, (void*)sockbev);
    bufferevent_enable(stdbev, EV_READ);
    bufferevent_enable(stdbev, EV_WRITE);

    //开始事件循环
    event_base_dispatch(base);

    //结束处理
    event_free(evtimeout);
    event_base_free(base);

    printf("%zd total bytes read\n", total_bytes_read);
    printf("%zd total messages read\n", total_messages_read);
    printf("%.3f average messages size\n", (double)total_bytes_read / total_messages_read);
    printf("%.3f MiB/s throughtput\n",
	    (double)total_bytes_read / (timeout.tv_sec * 1024 * 1024));

    return 0;
}



