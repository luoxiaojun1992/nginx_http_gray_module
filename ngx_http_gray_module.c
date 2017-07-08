#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hiredis/hiredis.h"

#define DEFAULT_REDIS_KEY = "test_gray_test"
#define DEFAULT_REDIS_HOST = "127.0.0.1"
#define DEFAULT_REDIS_PORT = 6379

typedef struct {
  ngx_str_t    redis_key;
  ngx_str_t    redis_host;
  ngx_unint_t  redis_port;
} ngx_http_gray_loc_conf_t;

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

int getGrayPolicy(ngx_http_request_t *r);

static void *
ngx_http_gray_create_loc_conf(ngx_conf_t *cf);

static char *
ngx_http_gray_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child);

/*模块 commands*/
static ngx_command_t  ngx_http_gray_commands[] =
{

    {
        ngx_string("gray"),
        NGX_HTTP_MAIN_CONF | NGX_HTTP_SRV_CONF | NGX_HTTP_LOC_CONF | NGX_HTTP_LMT_CONF | NGX_CONF_TAKE3,
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
  getGrayPolicy(r);

	// if (ngx_random() % 2 == 0) {
	// 	isGray = 1;
	// } else {
	// 	isGray = 0;
	// }

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

int getGrayPolicy(ngx_http_request_t *r)
{
  ngx_http_gray_loc_conf_t *elcf;
  elcf = ngx_http_get_module_loc_conf(r, ngx_http_gray_module);

  isGray = 0;

  redisContext *c;
  redisReply *reply;
  const char *hostname = elcf->redis_host.data;
  int port = elcf->redis_port.data;
  struct timeval timeout = { 1, 500000 }; // 1.5 seconds
  c = redisConnectWithTimeout(hostname, port, timeout);
  if (c == NULL || c->err) {
      if (c) {
          ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Connection error: %s", c->errstr);
          redisFree(c);
      } else {
          ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, "Connection error: can't allocate redis context");
      }

      isGray = 0;
      return 1;
  }

  //Check Gray Test Switch
  reply = redisCommand(c, "GET %s_switch", elcf->redis_key.data);
  if (reply->str) {
    if (ngx_strcmp("on", reply->str)) {
      freeReplyObject(reply);
      redisFree(c);
      isGray = 0;
      return 1;
    }
  }
  freeReplyObject(reply);

  ngx_table_elt_t *h;
	ngx_list_part_t *part;
	ngx_uint_t i;
  part = &r->headers_in.headers.part;
	do {
	  h = part->elts;
	  for (i = 0; i < part->nelts; i++) {
      //Check App Version
      if ((h[i].key.len == ngx_strlen("X-API-ENV")) && (ngx_strcmp("X-API-ENV", h[i].key.data) == 0)) {
        if (h[i].value.data) {
          reply = redisCommand(c, "GET %s_gray_env", elcf->redis_key.data);
          if (reply->str) {
            if (!ngx_strcmp(h[i].value.data, reply->str)) {
              freeReplyObject(reply);
              redisFree(c);
              isGray = 1;
              return 1;
            }
          }
          freeReplyObject(reply);
        }
			}
    }
    part = part->next;
  } while ( part != NULL );

  redisFree(c);

  return 0;
}

static void *
ngx_http_gray_create_loc_conf(ngx_conf_t *cf)
{
    ngx_http_gray_loc_conf_t  *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_gray_loc_conf_t));
    if (conf == NULL) {
        return NGX_CONF_ERROR;
    }

    // ngx_str_null(&conf->redis_key);

    conf->redis_key.len = 0;
    conf->redis_key.data = NULL;

    conf->redis_host.len = 0;
    conf->redis_host.data = NULL;

    conf->redis_port = NGX_CONF_UNSET;

    return conf;
}

static char *
ngx_http_gray_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_gray_loc_conf_t *prev = parent;
    ngx_http_gray_loc_conf_t *conf = child;

    ngx_conf_merge_str_value(conf->redis_key, prev->redis_key, DEFAULT_REDIS_KEY);
    ngx_conf_merge_str_value(conf->redis_host, prev->redis_host, DEFAULT_REDIS_HOST);

    ngx_conf_merge_value(conf->redis_port, prev->redis_port, DEFAULT_REDIS_PORT);

    return NGX_CONF_OK;
}
