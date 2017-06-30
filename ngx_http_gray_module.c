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

static ngx_int_t
gray_subrequest_post_handler(ngx_http_request_t*r,void*data,ngx_int_t rc);

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

    pr->headers_out.status=r->headers_out.status;
    /*访问服务器成功，开始解析包体*/
    if(NGX_HTTP_OK == r->headers_out.status)
    {
        char *res = "";
        ngx_buf_t* pRecvBuf = &r->upstream->buffer;
        /*内容解析到stock数组中*/
        for (; pRecvBuf->pos != pRecvBuf->last; pRecvBuf->pos++)
        {
          strcat(res, *pRecvBuf->pos);
        }
    }

    return NGX_OK;

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
