#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

typedef struct {
  ngx_http_upstream_conf_t upstream;
} ngx_http_gray_conf_t;

//请求上下文
typedef struct {
  ngx_http_status_t  status;
} ngx_http_gray_ctx_t;

ngx_uint_t isGray = 0;

static ngx_str_t new_variable_is_gray = ngx_string("is_gray");
static ngx_str_t new_variable_is_not_gray = ngx_string("is_not_gray");



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

static void* ngx_http_gray_create_loc_conf(ngx_conf_t *cf);

static char *ngx_http_gray_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)



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




static void* ngx_http_gray_create_loc_conf(ngx_conf_t *cf)
{
  ngx_http_gray_conf_t *mycf;
  mycf = (ngx_http_gray_conf_t*)ngx_pcalloc(cf->pool, sizeof(ngx_http_gray_conf_t));
  if (mycf == NULL) {
    return NULL;
  }

  mycf->upstream.connect_timeout = 60000;
  mycf->upstream.send_timeout = 60000;
  mycf->upstream.read_timeout = 60000;
  mycf->upstream.store_access = 0600;
  mycf->upstream.buffering = 0;
  mycf->upstream.bufs.num = 8;
  mycf->upstream.bufs.size = ngx_pagesize;
  mycf->upstream.buffer_size = ngx_pagesize;
  mycf->upstream.busy_buffers_size = 2*ngx_pagesize;
  mycf->upstream.temp_file_write_size = 2*ngx_pagesize;
  mycf->upstream.max_temp_file_size = 1024 * 1024 * 1024;
  mycf->upstream.hide_headers = NGX_CONF_UNSET_PTR;
  mycf->upstream.pass_headers = NGX_CONF_UNSET_PTR;
  return mycf;
}

static char *ngx_http_gray_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
  ngx_http_gray_conf_t *prev = (ngx_http_gray_conf_t *)parent;

  ngx_http_gray_conf_t *conf = (ngx_http_gray_conf_t *)child;

  ngx_hash_init_t          hash;

  hash.max_size = 100;
  hash.bucket_size = 1024;
  hash.name = 'proxy_headers_hash';

  if (ngx_http_upstream_hide_headers_hash(cf, &conf->upstream, &prev->upstream, ngx_http_proxy_hide_headers, &hash) != NGX_OK) {
    return NGX_CONF_ERROR;
  }

  return NGX_CONF_OK;
}

static ngx_int_t gray_upstream_create_request(ngx_http_request_t *r)
{
  //HTTP请求头
  static ngx_str_t backendQueryLine = ngx_string("GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n");

  ngx_int_t queryLineLen = backendQueryLine.len;

  ngx_buf_t* b = ngx_create_temp_buf(r->pool, queryLineLen);

  if (b == NULL) {
    return NGX_ERROR;
  }

  b->last = b->pos + queryLineLen;

  //todo
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
  return NGX_OK;
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
