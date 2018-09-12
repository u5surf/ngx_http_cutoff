
/*
 * author u5surf
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
typedef struct {
    ngx_flag_t enable;
} ngx_http_cutoff_filter_conf_t;

typedef struct {
    ngx_chain_t         *free;
    ngx_chain_t         *busy;
} ngx_http_cutoff_filter_ctx_t;


static ngx_int_t ngx_http_cutoff_filter_init(ngx_conf_t *);

static void ngx_http_close_handler(ngx_event_t *);

static char *ngx_http_cutoff_filter_merge_conf(ngx_conf_t *, void *, void *);
static void *ngx_http_cutoff_filter_create_conf(ngx_conf_t *);

static ngx_command_t ngx_http_cutoff_filter_commands[] = {
    { ngx_string("cutoff"),
      NGX_HTTP_SRV_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_HTTP_SRV_CONF_OFFSET,
      offsetof(ngx_http_cutoff_filter_conf_t, enable),
      NULL
    },
    ngx_null_command
};

static ngx_http_module_t  ngx_http_cutoff_filter_module_ctx = {
    NULL,                                  /* preconfiguration */
    ngx_http_cutoff_filter_init,          /* postconfiguration */
    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */
    ngx_http_cutoff_filter_create_conf,                                  /* create server configuration */
    ngx_http_cutoff_filter_merge_conf,                                  /* merge server configuration */
    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};


ngx_module_t  ngx_http_cutoff_filter_module = {
    NGX_MODULE_V1,
    &ngx_http_cutoff_filter_module_ctx,   /* module context */
    ngx_http_cutoff_filter_commands,                                  /* module directives */
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


static ngx_http_request_body_filter_pt    ngx_http_next_body_filter;


static ngx_int_t
ngx_http_cutoff_body_filter(ngx_http_request_t *r, ngx_chain_t *in)
{
    ngx_http_cutoff_filter_conf_t *conf;
    conf = ngx_http_get_module_srv_conf(r, ngx_http_cutoff_filter_module);
    if (!conf->enable) {
        return ngx_http_next_body_filter(r, in);
    }

    ngx_event_t *ngx_http_close_timer = ngx_pcalloc(r->pool, sizeof(ngx_event_t));
    ngx_http_close_timer->log = r->connection->log;
    ngx_http_close_timer->handler = ngx_http_close_handler;
    ngx_http_close_timer->data = NULL;
    ngx_http_close_timer->timer_set = 0;
    ngx_add_timer(ngx_http_close_timer,(ngx_msec_t)5000);
 
    return ngx_http_next_body_filter(r, in);
}


static ngx_int_t
ngx_http_cutoff_filter_init(ngx_conf_t *cf)
{
    ngx_http_next_body_filter = ngx_http_top_body_filter;
    ngx_http_top_body_filter = ngx_http_cutoff_body_filter;

    return NGX_OK;
}

static void *
ngx_http_cutoff_filter_create_conf(ngx_conf_t *cf) {
    ngx_http_cutoff_filter_conf_t *conf;
    conf = ngx_pcalloc(cf->pool, sizeof(ngx_http_cutoff_filter_conf_t));
    if (conf == NULL) {
        return NULL;
    }
    conf->enable = NGX_CONF_UNSET;
    return conf;
}

static char *
ngx_http_cutoff_filter_merge_conf(ngx_conf_t *cf, void *parent, void *child)
{
    ngx_http_cutoff_filter_conf_t *prev = parent;
    ngx_http_cutoff_filter_conf_t *conf = child;
    ngx_conf_merge_value(conf->enable, prev->enable, 0);
    return NGX_CONF_OK;
}

static void
ngx_http_close_handler(ngx_event_t *ev)
{
    ngx_connection_t *c;
    ngx_pool_t *pool;
    c = ev->data;
    pool = c->pool;
    c->close = 0;
    ev->delayed = 0;
    ngx_close_connection(c);
    ngx_destroy_pool(pool);
}


