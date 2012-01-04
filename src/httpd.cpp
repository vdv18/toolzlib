#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <netinet/tcp.h>

#include <toolz/httpd.h>
#include <toolz/net.h>

#include <event2/event.h>
#include <event2/util.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/http_struct.h>
#include <event2/thread.h>
#include <event2/event_struct.h>

CHttpd::CHttpd()
{
    evthread_use_pthreads();
};

CHttpd::~CHttpd()
{
;
};

const char *CHttpd::get_header(lz_httpd_req_t *req, const char *name){
    return evhttp_find_header(evhttp_request_get_input_headers(req->evreq), name);
}

const char *CHttpd::get_query_param(lz_httpd_req_t *req, const char *name){
    if( !req->query_params_parsed ){ 
        evhttp_parse_query(req->evreq->uri, &req->query_params); 
    }
    return evhttp_find_header(&req->query_params, name);
}

void CHttpd::add_header(lz_httpd_req_t *req, const char *name, const char *value){
    evhttp_add_header(evhttp_request_get_output_headers(req->evreq), name, value);
}

int CHttpd::get_response_code(lz_httpd_req_t *req){
    return evhttp_request_get_response_code(req->evreq);
}

int CHttpd::get_post_data(lz_httpd_req_t *req, size_t *size, char **data){
    struct evbuffer *evb;
    evb = evhttp_request_get_input_buffer(req->evreq);
    *size = evbuffer_get_length(evb);
    *data = (char *)evbuffer_pullup(evb, *size);
    return 0;
}


int CHttpd::add_printf(lz_httpd_req_t *req, const char *fmt, ...){
    int res = -1;
    va_list ap;

    va_start(ap, fmt);
    res = evbuffer_add_vprintf(evhttp_request_get_output_buffer(req->evreq), fmt, ap);
    va_end(ap);

    return (res);
}

int CHttpd::print_common_status(lz_httpd_req_t *req){

    struct timeval ti;
//    gettimeofday(&ti, NULL);
//    int time_offset=ti.tv_sec - conf->start_time;
//    int64_t wdays = time_offset/86400; 
//    int64_t whours = (time_offset - wdays*86400)/3600;
//    int64_t wmin = (time_offset - wdays*86400  - whours*3600) / 60;
//    int64_t wsec = time_offset - wdays*86400  - whours*3600 - wmin*60;
//    char work_str[256];
    int64_t avg_req = 0;
    int period_delta;

//    lx_snprintf(work_str, sizeof(work_str), "%ld days %ld hours %ld minutes %ld seconds", 
//        wdays, whours, wmin, wsec);
//    char work_time[128];
//    if(!lx_strftimestamp(work_time, sizeof(work_time), conf->start_time, "%x %X")) {
//        ERROR_LOG(L_ERROR, "Failed to fill time buffer");
//        return -1;
//    }

/*
    char *git_commit_str_commit = strdup(conf->git_commit);
    str_replace(git_commit_str_commit, ' ', '\0');
    evbuffer_add_printf(out,
            "<html>\n<meta http-equiv=\"Content-Type\" content=\"text/html;"
            "charset=utf-8\" />\n<head>\n<title>Welcome %s</title>\n</head>\n<body>\n"
            "<h1>server-status</h1>\n<p>your ip:%s, port %d</p>\n"
            "<p>country: %s</p>"
            "<p>request uri: %s</p>"
            "<p>timeoffset: %i</p>"
            "<p>git commit: <a href=\"https://github.com/medianet/luxup/commit/%s\">%s</a></p>",
            lxreq->ip, lxreq->ip,
            req->remote_port, lxreq->geo, lxreq->uri,time_offset, git_commit_str_commit, conf->git_commit
            );
    free(git_commit_str_commit);
    evbuffer_add_printf(out,
            "<p>worktime: %s</p>"  
            "<p>start time: %s</p>",
            work_str, work_time);

    const map<string, status_val> *status = get_status();
    map<string, status_val>::const_iterator status_i;
    if(status->size() > 0) {
        evbuffer_add_printf(out,
            "<table border=1 cellspacing='0' cellpadding='3'>\n"  
            "<tr><th>action/result</th><th>count</th>"
            "<th>per sec</th><th>per sec in last</th>"
            "<th>min exec time</th><th>max exec time</th>"
            "<th>avg exec time</th><th>Request more then 1 sec</th></tr>\n");
    }
    FailedMap::const_iterator ci;
    for (status_i  = status->begin(); status_i != status->end(); status_i++) {
        const status_val &psv = status_i->second;
        period_delta = 0;
        if( psv.prev_period_req > 0) {
            period_delta += AVG_REQUESTS_SWITCH_PERIOD;
        }
        period_delta += ti.tv_sec - psv.period_start_time;
        avg_req = (psv.prev_period_req > 0 ? psv.prev_period_req : psv.cur_period_req);
        evbuffer_add_printf(out, "<tr><td>%s</td><td>%d</td><td>%d</td><td>%d minutes: %ld</td>"
                                "<td>%f</td><td>%f</td><td>%f</td><td>%ld</td></tr>\n",
                status_i->first.c_str(),
                psv.hitcount,
                (time_offset>0 ? psv.hitcount/time_offset : 1),
                (period_delta)/60,
                (psv.prev_period_req + psv.cur_period_req)/(period_delta + 1),// + 1 if it is not well defined
                                                                              // to prevent devision by zero
                psv.min_exec_time, 
                psv.max_exec_time, 
                psv.avg_exec_time, 
                psv.more_1sec_exec_time);
        if((ci=failed.find(status_i->first))!=failed.end()) {
            uint64_t real_hits = psv.hitcount - ci->second.count;
            evbuffer_add_printf(out, "<tr><td>real %s</td><td>%ld</td><td>%ld</td><td>%d minutes: %ld</td>"
                                "<td>%f</td><td>%f</td><td>%f</td><td>%ld</td></tr>\n",
                status_i->first.c_str(),
                real_hits ,
                (time_offset>0 ? real_hits/time_offset : 1),
                (period_delta)/60,
                (psv.prev_period_req + psv.cur_period_req)/(period_delta + 1),// + 1 if it is not well defined
                                                                              // to prevent devision by zero
                psv.min_exec_time, 
                psv.max_exec_time, 
                psv.avg_exec_time, 
                psv.more_1sec_exec_time);

        }
    }
    if(status->size() > 0) {
        evbuffer_add_printf(out,"</table>");
    }
*/
    return 0;
}

int CHttpd::accept(const char *bind_str, void *arg){
    int sock = getSocket(bind_str, arg);
    return accept(sock); // TODO CLOSE
}

int CHttpd::accept(int socket){
   return evhttp_accept_socket(ev_http, socket); // TODO CLOSE
}

void CHttpd::shutdown(){
    event_base_loopbreak(ev_base);
}

int CHttpd::Init(eventMapNode *eventMap)
{
    int32_t i, res, err = 0;
    sockaddr_in  sa;

    int32_t on              =  1;
    int32_t server_sock     = -1;
    //struct linger ling      = {1, conf->timeout_timewait};
    int flag = 1;



    // check event map
    if (eventMap[0].name == NULL){
        //LOG(L_ERROR, "CShttpd::CShttpd eventMap could not be empty\n");
        goto fail;
    }
    _eventMap = eventMap;

/*
    // init all requests and mark as free
    for (i = 0; i < MAX_REQ_QUEUE; i++){
        lx_req *lxreq = new lx_req(this);
        requests[i]   = lxreq;
        free_requests.push_back(lxreq);
    }

    // create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        ERROR_LOG(L_ERROR, "Error socket()\n");
        goto fail;
    }

    sa.sin_family      = AF_INET;
    sa.sin_port        = htons(conf->bind_port);
    sa.sin_addr.s_addr = inet_addr(conf->bind_addr);

    res = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (res == -1){
        ERROR_LOG(L_ERROR, "Error setsockopt()\n");
        goto fail;
    }

    res = setsockopt(server_sock, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
    if (res != 0){
        ERROR_LOG(L_ERROR, "setsockopt failed\n");
        goto fail;
    }
    
    res = setsockopt(server_sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    if (res != 0){
        ERROR_LOG(L_ERROR, "setsockopt failed\n");
        goto fail;
    }

    // Bind server socket to ip:port
    if (bind(server_sock, (const sockaddr*)&sa, sizeof(sa)) == -1) {
        ERROR_LOG(L_ERROR, "Error bind(): for addr: '%s', port: '%d'\n",
            conf->bind_addr,
            conf->bind_port
        );
        goto fail;
    }

    // Make server to listen
    if (listen(server_sock, SERVER_BACKLOG) == -1) {
        ERROR_LOG(L_ERROR, "Error listen()\n");
        goto fail;
    }

    // Init Accesslog
    if (conf->use_access_log_file != NULL){
        string accesslog_path = string(conf->root_dir)
            + string("/")
            + string(conf->use_access_log_file);
        recursivly_mkdir((const char*)accesslog_path.c_str());
        accessLog = new AccessLog(conf, (char *)accesslog_path.c_str(), 32768);
        if(accessLog->Init() < 0 ) {
            ERROR_LOG(L_ERROR, "Failed to init accessLog\n");
            goto ret;
        }
    }

    // Init events
    ev_base = event_base_new();
    ev_http = evhttp_new(ev_base);

    
    serv_base           = (event_base *)event_init();
    this->_http_server  = evhttp_new(serv_base);

    res = evhttp_accept_socket(this->_http_server, server_sock);
    if (res == -1){
        ERROR_LOG(L_ERROR, "Error evhttp_accept_socket()\n");
        goto fail;
    }

    // Set HTTP request callback
    evhttp_set_gencb(this->_http_server, &CShttpd::on_request, (void *)this);

    evhttp_set_timeout(this->_http_server, conf->evhttp_connection_timeout);

    // Dispatch events
    ERROR_LOG(L_NOTE, "http server is running (%s:%d)\n",
        conf->bind_addr, conf->bind_port);

    // store server base to config
    conf->serv_base = serv_base;
*/
    ev_base = event_base_new();
    ev_http = evhttp_new(ev_base);


    err = 0;


ret:
    return err;
fail:
    err = -1;
    goto ret;
}

void CHttpd::run()
{
    evhttp_set_gencb(ev_http, CHttpd::dispatch, this);
    event_base_loop(ev_base, 0);
}


// action is done with this 
int CHttpd::send_reply(lz_httpd_req_t *req){
    evhttp_send_reply(req->evreq, HTTP_OK, "", evhttp_request_get_output_buffer(req->evreq));

    if( req->query_params_parsed ){
        evhttp_clear_headers(&req->query_params);
    }

    push_free_req(req);
    return 0;
}

void CHttpd::process_request(eventMapNode *event, lz_httpd_req_t *lz_req)
{
    event->func(lz_req);
    // lx_req can be free'd here
}


lz_httpd_req_t *CHttpd::get_free_req(){
    return (lz_httpd_req_t *) malloc(sizeof(lz_httpd_req_t));
}

void CHttpd::push_free_req(lz_httpd_req_t *req){
    free(req);
}


void CHttpd::dispatch(struct evhttp_request *evreq, void *arg){
    uint32_t flags = 0;
    CHttpd *me = (CHttpd *)arg;
    const char *action = NULL;
    eventMapNode *eventMapCursor = NULL;


    const char str_delims[] = "/";
    char *strbuf_it = strdupa(evreq->uri);

    action       = strsep(&strbuf_it, str_delims);
    action       = strsep(&strbuf_it, str_delims);

    lz_httpd_req_t *lz_req = me->get_free_req();
    if( lz_req == NULL ){
        goto ret;
    }
    lz_req->query_params_parsed = 0;
    lz_req->evreq = evreq;

    eventMapCursor = me->_eventMap;
    while (eventMapCursor->name != NULL){
        if (strcmp(action, eventMapCursor->name) == 0){
            flags = eventMapCursor->flags;
            me->process_request(eventMapCursor, lz_req);
            goto ret;
        }
        eventMapCursor++;
    }

    // if we didnt find any node, call last node, that is default
    // now check for addditional actions
    eventMapCursor = me->_eventMap;
    while (eventMapCursor->name != NULL){
        if (strcmp(action, eventMapCursor->name) == 0){
            flags = eventMapCursor->flags;
            me->process_request(eventMapCursor, lz_req);
            goto ret;
        }
        eventMapCursor++;
    }

    // if we didnt find any node, call last node, that is default
    // now check for addditional actions
    /*
    eventMapNode *eventMapCursor = me->_eventMap;
    while (eventMapCursor->name != NULL){
        if (strcmp(action, "actions") == 0){
            flags = eventMapCursor->flags;
            me->process_request(eventMapCursor, lz_req);
            goto ret;
        }
        eventMapCursor++;
    }*/

    // no action found show error
    goto fail;

ret:
    return;
fail:
    evhttp_send_reply(evreq, HTTP_BADREQUEST, "", NULL);
    goto ret;
}

int CHttpd::update_statistic(const char *statistic_key, double exec_time){
;
}

void CHttpd::request_callb(struct evhttp_request *req, void *arg){
    lz_httpd_req_t *r = (lz_httpd_req_t *)arg;
    r->callb(r, r->callb_arg); // we must provide r OR req ???
    //push_free_req(r);
}

lz_httpd_req_t *CHttpd::new_request(void (*callb)(lz_httpd_req_t *req, void *arg), void *arg){
    // todo free mem
    lz_httpd_req_t *r = get_free_req();
    if( r == NULL )
        return NULL;
    r->callb = callb;
    r->callb_arg = arg;
    r->evreq = evhttp_request_new(CHttpd::request_callb, r);
    if( r->evreq == NULL )
        return NULL;
    return r;
}

int CHttpd::make_request(int destination, lz_httpd_req_t *req, int http_type, const char *query){
    destination_t *d = NULL;
    int err;

    // 1.  create connection
    evhttp_connection *evcon_todaemon = evhttp_connection_base_new(
        ev_base,
        NULL,
        d->addr,
        d->port 
    );
    if (evcon_todaemon == NULL){
        LOG(L_DEBUG, "toolzlib", "Cannot create evhttp_connection_new()"
            " for IP: '%s' port: '%d'\n",
            d->addr,
            d->port
        );
        return -1;
    }
    // 2. set timeout
    evhttp_connection_set_timeout(evcon_todaemon, d->conn_timeout);

    // 5. do request
    err = evhttp_make_request(evcon_todaemon, req->evreq,
        EVHTTP_REQ_GET, query
    );
    if (err == -1){
        LOG(L_DEBUG, "toolzlib", "Cannot create evhttp_make_request()"
            " for IP: '%s' port: '%d'\n",
            d->addr,
            d->port
        );
        return -1;
    }
    return 0;
}

void CHttpd::eShowActions(lz_httpd_req_t *req){
    add_printf(req,
        "<HTML>\n"
        "<HEAD></HEAD>\n"
        "<BODY>\n"
        "<TABLE>\n"
    );

    add_printf(req,
        "<tr><td>All supported actions</td></tr>\n"
    );
/*
    while (eventMapCursor->name != NULL){
        add_printf(req,
            "<tr><td><a href=\"/%s\">/%s</a></td></tr>\n",
            eventMapCursor->name,
            eventMapCursor->name
        );
        eventMapCursor++;
    }
*/
    add_printf(req,
        "</TABLE>\n"
        "</BODY>\n"
        "</HTML>\n"
    );

    send_reply(req);
}
