#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>


/*上下文*/
typedef struct{
    ngx_str_t stock[6];
}ngx_http_gray_ctx_t;

/**
 *  定义模块配置结构   命名规则为ngx_http_[module-name]_[main|srv|loc]_conf_t。其中main、srv和loc分别用于表示同一模块在三层block中的配置信息。
 */
typedef struct {
    ngx_str_t ed;  //该结构体定义在这里 https://github.com/nginx/nginx/blob/master/src/core/ngx_string.h
} ngx_http_gray_loc_conf_t;


/*在ngx_http_gray中设置的回调，启动subrequest子请求*/
static ngx_int_t
ngx_http_gray_handler(ngx_http_request_t *r);

/*父请求重新被激活后的回调方法*/
static void
gray_post_handler(ngx_http_request_t * r);

/*配置项处理函数*/
static char*
ngx_http_gray(ngx_conf_t *cf ,ngx_command_t *cmd ,void *conf);

/*子请求结束时的回调函数*/
static ngx_int_t
gray_subrequest_post_handler(ngx_http_request_t *r, void *data, ngx_int_t rc);

static void *ngx_http_gray_create_loc_conf(ngx_conf_t *cf);
static char *ngx_http_gray_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);
static ngx_int_t ngx_http_gray_init(ngx_conf_t *cf);

/*模块 commands*/
static ngx_command_t  ngx_http_gray_commands[] =
{

    {
        ngx_string("gray"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE1,
        ngx_http_gray,
        NGX_HTTP_LOC_CONF_OFFSET,
        0,
        NULL
    },

    ngx_null_command
};

/*http模块上下文*/
static ngx_http_module_t ngx_http_gray_module_ctx=
{
    NULL,   /* preconfiguration */
    ngx_http_gray_init,   /* postconfiguration */
    NULL,   /* create main configuration */
    NULL,   /* init main configuration */
    NULL,   /* create server configuration */
    NULL,   /* merge server configuration */
    ngx_http_gray_create_loc_conf,   /* create location configuration */
    ngx_http_gray_merge_loc_conf    /* merge location configuration */
};

/*nginx 模块*/
ngx_module_t  ngx_http_gray_module =
{
    NGX_MODULE_V1,
    &ngx_http_gray_module_ctx,           /* module context */
    ngx_http_gray_commands,              /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


/**
 * 初始化一个配置结构体
 * @param cf
 * @return
 */
static void *
ngx_http_echo_create_loc_conf(ngx_conf_t *cf)
{
        ngx_http_gray_loc_conf_t *conf;
        conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_gray_loc_conf_t)); //gx_pcalloc用于在Nginx内存池中分配一块空间，是pcalloc的一个包装
        if(conf == NULL) {
                return NGX_CONF_ERROR;
        }
        conf->ed.len = 0;
        conf->ed.data = NULL;
        return conf;
}
/**
 * 将其父block的配置信息合并到此结构体 实现了配置的继承
 * @param cf
 * @param parent
 * @param child
 * @return ngx status code
 *
 * ngx_conf_merge_str_value不是一个函数，而是一个宏，其定义在https://github.com/nginx/nginx/blob/master/src/core/ngx_conf_file.h#L205中
 */
static char *
ngx_http_echo_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_gray_loc_conf_t *prev = parent;
    ngx_http_gray_loc_conf_t *conf = child;
    ngx_conf_merge_str_value(conf->ed, prev->ed, '"');
    return NGX_CONF_OK;
}

/**
 * init echo模块
 * @param cf
 * @return
 */
static ngx_int_t
ngx_http_echo_init(ngx_conf_t *cf)
{
    return NGX_OK;
}


/*配置项处理函数*/
static char*
ngx_http_gray(ngx_conf_t *cf,ngx_command_t*cmd,void *conf)
{

    ngx_http_core_loc_conf_t  *clcf;

    /*找到gray配置项所属的配置块*/
    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    /*当请求URI匹配时将调用handler来处理请求*/
    clcf->handler = ngx_http_gray_handler;

    return NGX_CONF_OK;
}



/*子请求结束时回调该方法*/
static ngx_int_t
gray_subrequest_post_handler(ngx_http_request_t*r,void*data,ngx_int_t rc)
{

    /*当前请求是子请求*/
    ngx_http_request_t          *pr = r->parent;

    /*取得上下文*/
    ngx_http_gray_ctx_t* myctx = ngx_http_get_module_ctx(pr, ngx_http_gray_module);

    pr->headers_out.status=r->headers_out.status;
    /*访问服务器成功，开始解析包体*/
    if(NGX_HTTP_OK == r->headers_out.status)
    {
        int flag = 0;

        ngx_buf_t* pRecvBuf = &r->upstream->buffer;
        /*内容解析到stock数组中*/
        for (; pRecvBuf->pos != pRecvBuf->last; pRecvBuf->pos++)
        {
            if (*pRecvBuf->pos == ',' || *pRecvBuf->pos == '\"')
            {
                if (flag > 0)
                {
                    myctx->stock[flag - 1].len = pRecvBuf->pos - myctx->stock[flag - 1].data;
                }
                flag++;
                myctx->stock[flag - 1].data = pRecvBuf->pos + 1;
            }
            if (flag > 6)
                break;
        }

    }
    /*设置父请求的回调方法*/
    pr->write_event_handler = gray_post_handler;

    return NGX_OK;

}

/*激活父请求回调*/
static void
gray_post_handler(ngx_http_request_t * r)
{

    /*如果没有返回200则直接把错误码发回用户*/
    if (r->headers_out.status != NGX_HTTP_OK)
    {
    ngx_http_finalize_request(r, r->headers_out.status);
        return;
    }


    /*当前请求是父请求*/
    ngx_http_gray_ctx_t* myctx = ngx_http_get_module_ctx(r, ngx_http_gray_module);


    /*发送给客户端的包体内容*/

    ngx_str_t output_format = ngx_string("stock[%V],Today current price: %V, volumn: %V");

    /*计算待发送包体的长度*/
    /*-6减去占位符*/
    int bodylen = output_format.len + myctx->stock[0].len
                  + myctx->stock[1].len + myctx->stock[4].len - 6;

    r->headers_out.content_length_n = bodylen;

    /*内存池上分配内存保存将要发送的包体*/
    ngx_buf_t* b = ngx_create_temp_buf(r->pool, bodylen);
    ngx_snprintf(b->pos, bodylen, (char*)output_format.data,
                 &myctx->stock[0], &myctx->stock[1], &myctx->stock[4]);


    b->last = b->pos + bodylen;
    b->last_buf = 1;

    ngx_chain_t out;
    out.buf = b;
    out.next = NULL;

    /*设置Content-Type，汉子编码是GBK*/
    static ngx_str_t type = ngx_string("text/plain; charset=GBK");

    r->headers_out.content_type = type;
    r->headers_out.status = NGX_HTTP_OK;

    r->connection->buffered |= NGX_HTTP_WRITE_BUFFERED;

    /*发送http头部;包括响应行*/
    ngx_int_t ret = ngx_http_send_header(r);
    /*发送http包体*/
    ret = ngx_http_output_filter(r, &out);

    /*需要手动调用*/
    ngx_http_finalize_request(r,ret);
}


/*启动subrequest子请求*/
static ngx_int_t
ngx_http_gray_handler(ngx_http_request_t * r)
{

    /*创建上下文*/
    ngx_http_gray_ctx_t* myctx = ngx_http_get_module_ctx(r, ngx_http_gray_module);

    if(NULL == myctx)
    {
        myctx = ngx_palloc(r->pool,sizeof(ngx_http_gray_ctx_t));
        if (myctx == NULL)
        {
            return NGX_ERROR;
        }

        /*将上下文设置到原始请求r中*/
        ngx_http_set_ctx(r, myctx, ngx_http_gray_module);
    }

    /*子请求的回调方法将在此结构体中设置*/
    ngx_http_post_subrequest_t *psr=ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));

    if (psr == NULL)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    /*设置子请求处理完毕时回调方法为gray_subrequest_post_handler*/
    psr->handler = gray_subrequest_post_handler;
    /*回调函数的data参数*/
    psr->data = myctx;


    /*构造子请求*/
    /*sina服务器要求*/
    ngx_str_t sub_prefix = ngx_string("/list=");

    /*URL构造=前缀+参数*/
    ngx_str_t sub_location;
    sub_location.len = sub_prefix.len + r->args.len;
    sub_location.data = ngx_palloc(r->pool, sub_location.len);
    ngx_snprintf(sub_location.data, sub_location.len,
        "%V%V", &sub_prefix, &r->args);


    /*sr即为子请求*/
    ngx_http_request_t *sr;
    /*创建子请求*/
    /*参数分别是：*/
    //1.父请求 2.请求URI 3.子请求的URI参数 4.返回创建好的子请求
    //5.子请求结束的回调
    //6.subrequest_in_memory 标识
    ngx_int_t rc = ngx_http_subrequest(r, &sub_location, NULL, &sr, psr, NGX_HTTP_SUBREQUEST_IN_MEMORY);

    if (rc != NGX_OK)
    {
        return NGX_ERROR;
    }

    /*必须返回NGX_DONE*/
    /*父请求不会被销毁，而是等待再次被激活*/
    return NGX_DONE;
}
