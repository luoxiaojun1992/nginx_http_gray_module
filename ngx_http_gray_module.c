#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>

/**
 *  定义模块配置结构   命名规则为ngx_http_[module-name]_[main|srv|loc]_conf_t。其中main、srv和loc分别用于表示同一模块在三层block中的配置信息。
 */
typedef struct {
    int variable_index;
    ngx_str_t variable;
} ngx_http_gray_loc_conf_t;


/*在ngx_http_gray中设置的回调，启动subrequest子请求*/
static ngx_int_t
ngx_http_gray_handler(ngx_http_request_t *r);

/*配置项处理函数*/
static char*
ngx_http_gray(ngx_conf_t *cf ,ngx_command_t *cmd ,void *conf);

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
ngx_http_gray_create_loc_conf(ngx_conf_t *cf)
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
ngx_http_gray_merge_loc_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_gray_loc_conf_t *prev = parent;
    ngx_http_gray_loc_conf_t *conf = child;
    ngx_conf_merge_str_value(conf->ed, prev->ed, '"');
    return NGX_CONF_OK;
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
    ngx_str_t                 *value;
    ngx_http_gray_loc_conf_t  *mycf = conf;

    value = cf->args->elts;
    if (cf->args->nelts != 2) {
      return NGX_CONF_ERROR;
    }

    if (value[1].data[0] == '$') {
      value[1].len--;
      value[1].data++;
      mycf->variable_index = ngx_http_get_variable_index(cf, &value[1])
      if (mycf->variable_index == NGX_ERROR) {
        return NGX_CONF_ERROR;
      }
      mycf->variable = value[1];
    } else {
      return NGX_CONF_ERROR;
    }

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

}
