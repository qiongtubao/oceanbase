#include <sys/prctl.h>
#define MAX_PN_LISTEN 256
#define MAX_PN_GRP (1<<17)
#define MAX_PN_PER_GRP 64
#ifdef PERF_MODE
#define CHUNK_SIZE ((1<<21) - (17<<10))
#else
#define CHUNK_SIZE (1<<14) - 128
#endif

typedef struct pn_listen_t
{
  listen_t l;
  serve_cb_t serve_cb;
  pthread_t pd;
} pn_listen_t;

struct pn_t;
typedef struct pn_grp_t
{
  int count;
  struct pn_t* pn_array[MAX_PN_PER_GRP];
} pn_grp_t;

typedef struct pn_t
{
  pthread_t pd;
  int accept_qfd;
  eloop_t ep;
  int gid;
  int tid;
  serve_cb_t serve_cb;
  fifo_alloc_t server_ctx_alloc;
  cfifo_alloc_t server_resp_alloc;
  cfifo_alloc_t client_req_alloc;
  cfifo_alloc_t client_cb_alloc;
  chunk_cache_t server_ctx_chunk_alloc;
  chunk_cache_t server_resp_chunk_alloc;
  chunk_cache_t client_req_chunk_alloc;
  chunk_cache_t client_cb_chunk_alloc;
  pkts_t pkts;
  pktc_t pktc;
} pn_t;

static int next_pn_listen_idx;
static pn_listen_t pn_listen_array[MAX_PN_LISTEN];
static pn_grp_t* pn_grp_array[MAX_PN_GRP];
int64_t pnio_keepalive_timeout;
PN_API int64_t pn_set_keepalive_timeout(int64_t user_timeout) {
  if (user_timeout >= 0) {
    pnio_keepalive_timeout = user_timeout;
  }
  return pnio_keepalive_timeout;
}
static pn_listen_t* locate_listen(int idx)
{
  return pn_listen_array + idx;
}

static void* listen_thread_func(void* arg)
{
  thread_counter_reg();
  prctl(PR_SET_NAME, "pnlisten");
  pn_listen_t* l = (typeof(l))arg;
  eloop_run(&l->l.ep);
  return NULL;
}

static void* pn_thread_func(void* arg)
{
  thread_counter_reg();
  prctl(PR_SET_NAME, "pnio");
  pn_t* pn = (typeof(pn))arg;
  eloop_run(&pn->ep);
  return NULL;
}

static int pnl_dispatch_accept(int fd, const void* b, int sz);
PN_API int pn_listen(int port, serve_cb_t cb)
{
  int idx = FAA(&next_pn_listen_idx, 1);
  pn_listen_t* pnl = locate_listen(idx);
  addr_t addr;
  addr_init(&addr, "0.0.0.0", port);
  if (listen_init(&pnl->l, addr, pnl_dispatch_accept) < 0) {
    idx = -1;
  } else {
    pnl->serve_cb = cb;
    pthread_create(&pnl->pd, NULL, listen_thread_func, pnl);
  }
  return idx;
}

static pn_grp_t* create_grp()
{
  pn_grp_t* grp = (typeof(grp))salloc(sizeof(*grp));
  if (grp) {
    memset(grp, 0, sizeof(*grp));
  }
  return grp;
}

static pn_grp_t* locate_grp(int gid)
{
  return pn_grp_array[gid];
}

static pn_grp_t* ensure_grp(int gid)
{
  pn_grp_t** pgrp = pn_grp_array + gid;
  if (unlikely(NULL == *pgrp)) {
    *pgrp = create_grp();
  }
  return *pgrp;
}

static int dispatch_fd_to(int fd, uint32_t gid, uint32_t tid)
{
  int err = 0;
  pn_grp_t* grp = locate_grp(gid);
  if (NULL == grp || grp->count == 0) {
    err = -ENOENT;
  } else {
    pn_t* pn = grp->pn_array[tid % grp->count];
    int wbytes = 0;
    while((wbytes = write(pn->accept_qfd, (const char*)&fd, sizeof(fd))) < 0
          && EINTR == errno);
    if (wbytes != sizeof(fd)) {
      err = -EIO;
    }
  }
  return err;
}

static int pnl_dispatch_accept(int fd, const void* b, int sz)
{
  int err = 0;
  uint64_t dispatch_id = 0;
  if ((uint64_t)sz < sizeof(dispatch_id)) {
    // abort();
    err = dispatch_fd_to(fd, 1, 0);
  } else {
    dispatch_id = *(uint64_t*)b;
    uint32_t gid = dispatch_id >> 32;
    uint32_t tid = dispatch_id & ((1ULL<<32) - 1);
    err = dispatch_fd_to(fd, gid, tid);
  }
  return err;
}

static int pn_pkts_handle_func(pkts_t* pkts, void* req_handle, const char* b, int64_t s, uint64_t chid);

static pn_t* pn_alloc()
{
  pn_t* pn = (typeof(pn))salloc(sizeof(*pn));
  if (pn) {
    memset(pn, 0, sizeof(*pn));
  }
  return pn;
}

static void pn_destroy(pn_t* pn)
{
  if (pn->accept_qfd >= 0) {
    close(pn->accept_qfd);
    pn->accept_qfd = -1;
  }
  sfree(pn);
}

static uint64_t calc_dispatch_id(uint32_t gid, uint32_t tid)
{
  return ((uint64_t)gid)<<32 | tid;
}

static int pn_init_pkts(int listen_id, pn_t* pn)
{
  int err = 0;
  int pfd[2] = {-1, -1};
  pkts_cfg_t cfg = {.handle_func = pn_pkts_handle_func };
  if (listen_id < 0) {
  } else if (0 != pipe2(pfd, O_NONBLOCK|O_CLOEXEC)) {
    err = errno;
  } else {
    pn->accept_qfd = pfd[1];
    cfg.accept_qfd = pfd[0];
    if (0 != (err = pkts_init(&pn->pkts, &pn->ep, &cfg))) {
    } else {
      pn_listen_t* pnl = locate_listen(listen_id);
      pn->serve_cb = pnl->serve_cb;
    }
  }
  return err;
}

static pn_t* pn_create(int listen_id, int gid, int tid)
{
  int err = 0;
  pn_t* pn = NULL;
  if (NULL == (pn = pn_alloc())) {
  } else if (0 != (err = eloop_init(&pn->ep))) {
  } else if (0 != (err = pktc_init(&pn->pktc, &pn->ep, calc_dispatch_id(gid, tid))))  {
  } else if (0 != (err = pn_init_pkts(listen_id, pn))) {
  } else {
    pn->gid = gid;
    pn->tid = tid;
    chunk_cache_init(&pn->server_ctx_chunk_alloc, CHUNK_SIZE, MOD_SERVER_CTX_CHUNK);
    chunk_cache_init(&pn->server_resp_chunk_alloc, CHUNK_SIZE, MOD_SERVER_RESP_CHUNK);
    chunk_cache_init(&pn->client_req_chunk_alloc, CHUNK_SIZE, MOD_CLIENT_REQ_CHUNK);
    chunk_cache_init(&pn->client_cb_chunk_alloc, CHUNK_SIZE, MOD_CLIENT_CB_CHUNK);

    fifo_alloc_init(&pn->server_ctx_alloc, &pn->server_ctx_chunk_alloc);
    cfifo_alloc_init(&pn->server_resp_alloc, &pn->server_resp_chunk_alloc);
    cfifo_alloc_init(&pn->client_req_alloc, &pn->client_req_chunk_alloc);
    cfifo_alloc_init(&pn->client_cb_alloc, &pn->client_cb_chunk_alloc);
  }
  if (0 != err && NULL != pn) {
    pn_destroy(pn);
  }
  return pn;
}


PN_API int pn_provision(int listen_id, int gid, int thread_count)
{
  int err = 0;
  int count = 0;
  pn_grp_t* pn_grp = ensure_grp(gid);
  ef(pn_grp == NULL);
  count = pn_grp->count;
  while(0 == err && count < thread_count) {
    pn_t* pn = pn_create(listen_id, gid, count);
    if (NULL == pn) {
      err = ENOMEM;
    } else if (0 != (err = pthread_create(&pn->pd, NULL, pn_thread_func, pn))) {
      pn_destroy(pn);
    } else {
      pn_grp->pn_array[count++] = pn;
    }
  }
  pn_grp->count = count;
  return pn_grp->count;
  el();
  return -1;
}

typedef struct pn_pktc_cb_t
{
  pktc_cb_t cb;
  client_cb_t client_cb;
  void* arg;
} pn_pktc_cb_t;

typedef struct pn_client_req_t
{
  pktc_req_t req;
  easy_head_t head;
} pn_client_req_t;

typedef struct pn_client_slice_t
{
  int64_t ref_;
  pn_pktc_cb_t cb_;
  pn_client_req_t req_;
} pn_client_slice_t;

#ifdef PERF_MODE
static void pn_pktc_flush_cb(pktc_req_t* r)
{
  pn_client_req_t* pn_req = structof(r, pn_client_req_t, req);
  pn_client_slice_t* slice = structof(pn_req, pn_client_slice_t, req_);
  if (0 == --slice->ref_) {
    pn_pktc_cb_t* pn_cb = &slice->cb_;
    PNIO_DELAY_WARN(STAT_TIME_GUARD(eloop_client_cb_count, eloop_client_cb_time));
    pn_cb->client_cb(pn_cb->arg, (&pn_cb->cb)->errcode, NULL, 0);
  }
}

static void pn_pktc_resp_cb(pktc_cb_t* cb, const char* resp, int64_t sz)
{
  pn_pktc_cb_t* pn_cb = structof(cb, pn_pktc_cb_t, cb);
  pn_client_slice_t* slice = structof(pn_cb, pn_client_slice_t, cb_);
  if (0 == ++slice->ref_) {
    PNIO_DELAY_WARN(STAT_TIME_GUARD(eloop_client_cb_count, eloop_client_cb_time));
    pn_cb->client_cb(pn_cb->arg, cb->errcode, resp, sz);
  }
}
#else
static void pn_pktc_flush_cb(pktc_req_t* r)
{
  pn_client_req_t* pn_req = structof(r, pn_client_req_t, req);
  cfifo_free(pn_req);
}

static void pn_pktc_resp_cb(pktc_cb_t* cb, const char* resp, int64_t sz)
{
  pn_pktc_cb_t* pn_cb = structof(cb, pn_pktc_cb_t, cb);
  PNIO_DELAY_WARN(STAT_TIME_GUARD(eloop_client_cb_count, eloop_client_cb_time));
  pn_cb->client_cb(pn_cb->arg, cb->errcode, resp, sz);
  cfifo_free(pn_cb);
}
#endif

static pktc_req_t* pn_create_pktc_req(pn_t* pn, uint64_t pkt_id, addr_t dest, const char* req, int64_t req_sz, int16_t categ_id, int64_t expire_us, client_cb_t client_cb, void* arg)
{
#ifdef PERF_MODE
  pn_client_slice_t* slice = ((typeof(slice))req) - 1;
  if (unlikely(NULL == slice)) {
    return NULL;
  }
  pn_client_req_t* pn_req = &slice->req_;
  struct pn_pktc_cb_t* pn_cb = &slice->cb_;
  slice->ref_ = 0;
#else
  pn_client_req_t* pn_req = (typeof(pn_req))cfifo_alloc(&pn->client_req_alloc, sizeof(*pn_req) + req_sz);
  if (unlikely(NULL == pn_req)) {
    return NULL;
  }
  struct pn_pktc_cb_t* pn_cb = (typeof(pn_cb))cfifo_alloc(&pn->client_cb_alloc, sizeof(*pn_cb));
  if (unlikely(NULL == pn_cb)) {
    cfifo_free(pn_req);
    return NULL;
  }
#endif
  pktc_cb_t* cb = &pn_cb->cb;
  pktc_req_t* r = &pn_req->req;
  pn_cb->client_cb = client_cb;
  pn_cb->arg = arg;
  cb->id = pkt_id;
  cb->expire_us = expire_us;
  cb->resp_cb = pn_pktc_resp_cb;
  cb->errcode = PNIO_OK;
  r->flush_cb = pn_pktc_flush_cb;
  r->resp_cb = cb;
  r->dest = dest;
  r->categ_id = categ_id;
  eh_copy_msg(&r->msg, cb->id, req, req_sz);
  return r;
}

static uint64_t global_next_pkt_id;
static uint64_t gen_pkt_id()
{
  static __thread uint64_t next_pkt_id = 0;
  if (0 == (next_pkt_id & 0xff)) {
    next_pkt_id = FAA(&global_next_pkt_id, 256);
  }
  return next_pkt_id++;
}

static pn_t* get_pn_for_send(pn_grp_t* pgrp, int tid)
{
  return pgrp->pn_array[tid % pgrp->count];
}

PN_API int pn_send(uint64_t gtid, struct sockaddr_in* addr, const char* buf, int64_t sz, int16_t categ_id, int64_t expire_us, client_cb_t cb, void* arg)
{
  int err = 0;
  pn_grp_t* pgrp = locate_grp(gtid>>32);
  pn_t* pn = get_pn_for_send(pgrp, gtid & 0xffffffff);
  addr_t dest = {.ip=addr->sin_addr.s_addr, .port=htons(addr->sin_port), .tid=0};
  if (addr->sin_addr.s_addr == 0 || htons(addr->sin_port) == 0) {
    err = -EINVAL;
    rk_error("invalid sin_addr: %x:%d", addr->sin_addr.s_addr, addr->sin_port);
  }
  pktc_req_t* r = pn_create_pktc_req(pn, gen_pkt_id(), dest, buf, sz, categ_id, expire_us, cb, arg);
  if (NULL == r) {
    err = ENOMEM;
  } else {
    if (NULL != arg) {
      *((void**)arg) = r;
    }
    err = pktc_post(&pn->pktc, r);
  }
  return err;
}

typedef struct pn_resp_ctx_t
{
  pn_t* pn;
  void* req_handle;
  uint64_t sock_id;
  uint64_t pkt_id;
#ifdef PERF_MODE
  char reserve[1<<10];
#else
  char reserve[sizeof(pkts_req_t)];
#endif
} pn_resp_ctx_t;

static pn_resp_ctx_t* create_resp_ctx(pn_t* pn, void* req_handle, uint64_t sock_id, uint64_t pkt_id)
{
  int64_t gid = pn->tid;
  unused(gid);
  pn_resp_ctx_t* ctx = (typeof(ctx))fifo_alloc(&pn->server_ctx_alloc, sizeof(*ctx));
  if (ctx) {
    ctx->pn = pn;
    ctx->req_handle = req_handle;
    ctx->sock_id = sock_id;
    ctx->pkt_id = pkt_id;
  }
  return ctx;
}

static int pn_pkts_handle_func(pkts_t* pkts, void* req_handle, const char* b, int64_t s, uint64_t chid)
{
  int err = 0;
  uint64_t pkt_id = eh_packet_id(b);
  pn_t* pn = structof(pkts, pn_t, pkts);
  pn_resp_ctx_t* ctx = create_resp_ctx(pn, req_handle, chid, pkt_id);
  if (NULL == ctx) {
    rk_info("create_resp_ctx failed, errno = %d", errno);
  } else {
    PNIO_DELAY_WARN(STAT_TIME_GUARD(eloop_server_process_count, eloop_server_process_time));
    err = pn->serve_cb(pn->gid, b, s, (uint64_t)ctx);
  }
  return err;
}

typedef struct pn_resp_t
{
  pn_resp_ctx_t* ctx;
  pkts_req_t req;
  easy_head_t head;
} pn_resp_t;

static void pn_pkts_flush_cb_func(pkts_req_t* req)
{
  pn_resp_t* resp = structof(req, pn_resp_t, req);
  if ((uint64_t)resp->ctx->reserve == (uint64_t)resp) {
    fifo_free(resp->ctx);
  } else {
    fifo_free(resp->ctx);
    cfifo_free(resp);
  }
}

static void pn_pkts_flush_cb_error_func(pkts_req_t* req)
{
  pn_resp_ctx_t* ctx = (typeof(ctx))structof(req, pn_resp_ctx_t, reserve);
  fifo_free(ctx);
}

PN_API int pn_resp(uint64_t req_id, const char* buf, int64_t sz)
{
  pn_resp_ctx_t* ctx = (typeof(ctx))req_id;
#ifdef PERF_MODE
  ref_free(ctx->req_handle);
#endif
  pn_resp_t* resp = NULL;
  if (sizeof(pn_resp_t) + sz <= sizeof(ctx->reserve)) {
    resp = (typeof(resp))(ctx->reserve);
  } else {
    resp = (typeof(resp))cfifo_alloc(&ctx->pn->server_resp_alloc, sizeof(*resp) + sz);
  }
  pkts_req_t* r = NULL;
  if (NULL != resp) {
    r = &resp->req;
    resp->ctx = ctx;
    r->errcode = 0;
    r->flush_cb = pn_pkts_flush_cb_func;
    r->sock_id = ctx->sock_id;
    r->categ_id = 0;
    eh_copy_msg(&r->msg, ctx->pkt_id, buf, sz);
  } else {
    r = (typeof(r))(ctx->reserve);
    r->errcode = ENOMEM;
    r->flush_cb = pn_pkts_flush_cb_error_func;
    r->sock_id = ctx->sock_id;
    r->categ_id = 0;
  }
  pkts_t* pkts = &ctx->pn->pkts;
  return pkts_resp(pkts, r);
}
