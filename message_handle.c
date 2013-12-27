
void http_handler(struct evhttp_request *req, void *arg)
{
    struct wevbuffer *buf;buf = evbuffer_new();
    
    // 分析请求
    // 从http头中获取参数

    // evhttp_add_header(req->output_headers, "Content-Type", "text/html; charset=UTF-8");
    // evhttp_add_header(req->output_headers, "Server", "my_httpd");
    // evhttp_add_header(req->output_headers, "Connection", "keep-alive");
    //
    // evhttp_add_header(req->output_headers, "Connection", "close");
    
    // 将要输出的值写入输出缓存
    // if(request_value != NULL) {
    //     evbuffer_add_printf(buf, "%s", request_value);
    // else {
    //     evbuffer_add_printf(buf, "%s", "no error.");
    // }
    //输出
    //evhttp_send_reply(req, HTTP_OK, "OK", buf);
    // 内存释放
    //evhttp_clear_headers(&http_query);
    //evbuffer_free(buf);
}

