#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

ngx_uint_t isGray = 0;

static ngx_str_t new_variable_is_gray = ngx_string("is_gray");
static ngx_str_t new_variable_is_not_gray = ngx_string("is_not_gray");

/*上下文*/
typedef struct{
    ngx_str_t stock[6];
}ngx_http_gray_ctx_t;

static ngx_int_t ngx_http_gray_add_variable(ngx_conf_t *cf);

/*在ngx_http_gray中设置的回调，启动subrequest子请求*/
static ngx_int_t
ngx_http_gray_handler(ngx_http_request_t *r);

/*配置项处理函数*/
static char*
ngx_http_gray(ngx_conf_t *cf ,ngx_command_t *cmd ,void *conf);

static ngx_int_t ngx_http_gray_init(ngx_conf_t *cf);

static ngx_int_t ngx_http_isgray_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v, ngx_uint_t data);

static ngx_int_t ngx_http_isnotgray_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v, ngx_uint_t data);

/*模块 commands*/
static ngx_command_t  ngx_http_gray_commands[] =
{

    {
        ngx_string("gray"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_NOARGS,
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
    ngx_http_gray_add_variable,   /* preconfiguration */
    ngx_http_gray_init,   /* postconfiguration */
    NULL,   /* create main configuration */
    NULL,   /* init main configuration */
    NULL,   /* create server configuration */
    NULL,   /* merge server configuration */
    NULL,   /* create location configuration */
    NULL    /* merge location configuration */
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

/**
 * init gray模块
 * @param cf
 * @return
 */
static ngx_int_t
ngx_http_gray_init(ngx_conf_t *cf)
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

/*启动subrequest子请求*/
static ngx_int_t
ngx_http_gray_handler(ngx_http_request_t * r)
{
  /*创建上下文*/
    ngx_http_mytest_ctx_t* myctx = ngx_http_get_module_ctx(r, ngx_http_mytest_module);

    if(NULL == myctx)
    {
        myctx = ngx_palloc(r->pool,sizeof(ngx_http_mytest_ctx_t));
        if (myctx == NULL)
        {
            return NGX_ERROR;
        }

        /*将上下文设置到原始请求r中*/
        ngx_http_set_ctx(r, myctx, ngx_http_mytest_module);
    }

    /*子请求的回调方法将在此结构体中设置*/
    ngx_http_post_subrequest_t *psr=ngx_palloc(r->pool, sizeof(ngx_http_post_subrequest_t));

    if (psr == NULL)
    {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    /*设置子请求处理完毕时回调方法为mytest_subrequest_post_handler*/
    psr->handler = mytest_subrequest_post_handler;
    /*回调函数的data参数*/
    psr->data = myctx;


    /*构造子请求*/
    /*sina服务器要求*/
    ngx_str_t sub_prefix = ngx_string("/isgray");

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

static ngx_int_t ngx_http_gray_add_variable(ngx_conf_t *cf)
{
    ngx_http_variable_t      *v1;
    v1 = ngx_http_add_variable(cf, &new_variable_is_gray, NGX_HTTP_VAR_CHANGEABLE);

    if (v1 == NULL) {
      return NGX_ERROR;
    }

    v1->get_handler = ngx_http_isgray_variable;
    v1->data = 0;

		ngx_http_variable_t      *v2;
    v2 = ngx_http_add_variable(cf, &new_variable_is_not_gray, NGX_HTTP_VAR_CHANGEABLE);

    if (v2 == NULL) {
      return NGX_ERROR;
    }

    v2->get_handler = ngx_http_isnotgray_variable;
    v2->data = 0;

    return NGX_OK;
}

static ngx_int_t ngx_http_isgray_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v, ngx_uint_t data)
{
	if (ngx_random() % 2 == 0) {
		isGray = 1;
	} else {
		isGray = 0;
	}

	if (isGray) {
  	*v = ngx_http_variable_true_value;
	} else {
		*v = ngx_http_variable_null_value;
	}

  return NGX_OK;
}

static ngx_int_t ngx_http_isnotgray_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v, ngx_uint_t data)
{
	if (!isGray) {
  	*v = ngx_http_variable_true_value;
	} else {
		*v = ngx_http_variable_null_value;
	}

  return NGX_OK;
}
