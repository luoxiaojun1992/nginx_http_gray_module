#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hiredis/hiredis.h"

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

char* getGrayPolicy();

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
  getGrayPolicy();

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

char* getGrayPolicy(ngx_http_request_t *r)
{
  redisContext *c;
  redisReply *reply;
  const char *hostname = "127.0.0.1";
  int port = 6379;

  struct timeval timeout = { 1, 500000 }; // 1.5 seconds
  c = redisConnectWithTimeout(hostname, port, timeout);
  if (c == NULL || c->err) {
      if (c) {
          printf("Connection error: %s\n", c->errstr);
          redisFree(c);
      } else {
          printf("Connection error: can't allocate redis context\n");
      }
      exit(1);
  }

  /* Try a GET and two INCR */
  reply = redisCommand(c,"GET test_gray");

  ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "%s", reply->str);

  char *result;
  result = reply->str;
  freeReplyObject(reply);

  /* Disconnects and frees the context */
  redisFree(c);

  return result;
}
