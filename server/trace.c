/* File generated automatically by tools/make_requests; DO NOT EDIT!! */

#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include "server.h"
#include "server/thread.h"

static void dump_new_thread_request( struct new_thread_request *req )
{
    printf( " pid=%p", req->pid );
}

static void dump_new_thread_reply( struct new_thread_reply *req )
{
    printf( " tid=%p,", req->tid );
    printf( " thandle=%d,", req->thandle );
    printf( " pid=%p,", req->pid );
    printf( " phandle=%d", req->phandle );
}

static void dump_init_thread_request( struct init_thread_request *req )
{
    printf( " unix_pid=%d", req->unix_pid );
}

static void dump_terminate_process_request( struct terminate_process_request *req )
{
    printf( " handle=%d,", req->handle );
    printf( " exit_code=%d", req->exit_code );
}

static void dump_terminate_thread_request( struct terminate_thread_request *req )
{
    printf( " handle=%d,", req->handle );
    printf( " exit_code=%d", req->exit_code );
}

static void dump_get_process_info_request( struct get_process_info_request *req )
{
    printf( " handle=%d", req->handle );
}

static void dump_get_process_info_reply( struct get_process_info_reply *req )
{
    printf( " pid=%p,", req->pid );
    printf( " exit_code=%d", req->exit_code );
}

static void dump_get_thread_info_request( struct get_thread_info_request *req )
{
    printf( " handle=%d", req->handle );
}

static void dump_get_thread_info_reply( struct get_thread_info_reply *req )
{
    printf( " pid=%p,", req->pid );
    printf( " exit_code=%d", req->exit_code );
}

static void dump_close_handle_request( struct close_handle_request *req )
{
    printf( " handle=%d", req->handle );
}

static void dump_dup_handle_request( struct dup_handle_request *req )
{
    printf( " src_process=%d,", req->src_process );
    printf( " src_handle=%d,", req->src_handle );
    printf( " dst_process=%d,", req->dst_process );
    printf( " dst_handle=%d,", req->dst_handle );
    printf( " access=%08x,", req->access );
    printf( " inherit=%d,", req->inherit );
    printf( " options=%d", req->options );
}

static void dump_dup_handle_reply( struct dup_handle_reply *req )
{
    printf( " handle=%d", req->handle );
}

static void dump_open_process_request( struct open_process_request *req )
{
    printf( " pid=%p,", req->pid );
    printf( " access=%08x,", req->access );
    printf( " inherit=%d", req->inherit );
}

static void dump_open_process_reply( struct open_process_reply *req )
{
    printf( " handle=%d", req->handle );
}

static void dump_select_request( struct select_request *req )
{
    printf( " count=%d,", req->count );
    printf( " flags=%d,", req->flags );
    printf( " timeout=%d", req->timeout );
}

static void dump_select_reply( struct select_reply *req )
{
    printf( " signaled=%d", req->signaled );
}

static void dump_create_event_request( struct create_event_request *req )
{
    printf( " manual_reset=%d,", req->manual_reset );
    printf( " initial_state=%d,", req->initial_state );
    printf( " inherit=%d", req->inherit );
}

static void dump_create_event_reply( struct create_event_reply *req )
{
    printf( " handle=%d", req->handle );
}

static void dump_event_op_request( struct event_op_request *req )
{
    printf( " handle=%d,", req->handle );
    printf( " op=%d", req->op );
}

static void dump_create_mutex_request( struct create_mutex_request *req )
{
    printf( " owned=%d,", req->owned );
    printf( " inherit=%d", req->inherit );
}

static void dump_create_mutex_reply( struct create_mutex_reply *req )
{
    printf( " handle=%d", req->handle );
}

static void dump_release_mutex_request( struct release_mutex_request *req )
{
    printf( " handle=%d", req->handle );
}

static void dump_create_semaphore_request( struct create_semaphore_request *req )
{
    printf( " initial=%08x,", req->initial );
    printf( " max=%08x,", req->max );
    printf( " inherit=%d", req->inherit );
}

static void dump_create_semaphore_reply( struct create_semaphore_reply *req )
{
    printf( " handle=%d", req->handle );
}

static void dump_release_semaphore_request( struct release_semaphore_request *req )
{
    printf( " handle=%d,", req->handle );
    printf( " count=%08x", req->count );
}

static void dump_release_semaphore_reply( struct release_semaphore_reply *req )
{
    printf( " prev_count=%08x", req->prev_count );
}

static void dump_open_named_obj_request( struct open_named_obj_request *req )
{
    printf( " type=%d,", req->type );
    printf( " access=%08x,", req->access );
    printf( " inherit=%d", req->inherit );
}

static void dump_open_named_obj_reply( struct open_named_obj_reply *req )
{
    printf( " handle=%d", req->handle );
}

struct dumper
{
    void (*dump_req)();
    void (*dump_reply)();
    unsigned int size;
};

static const struct dumper dumpers[REQ_NB_REQUESTS] =
{
    { (void(*)())dump_new_thread_request,
      (void(*)())dump_new_thread_reply,
      sizeof(struct new_thread_request) },
    { (void(*)())dump_init_thread_request,
      (void(*)())0,
      sizeof(struct init_thread_request) },
    { (void(*)())dump_terminate_process_request,
      (void(*)())0,
      sizeof(struct terminate_process_request) },
    { (void(*)())dump_terminate_thread_request,
      (void(*)())0,
      sizeof(struct terminate_thread_request) },
    { (void(*)())dump_get_process_info_request,
      (void(*)())dump_get_process_info_reply,
      sizeof(struct get_process_info_request) },
    { (void(*)())dump_get_thread_info_request,
      (void(*)())dump_get_thread_info_reply,
      sizeof(struct get_thread_info_request) },
    { (void(*)())dump_close_handle_request,
      (void(*)())0,
      sizeof(struct close_handle_request) },
    { (void(*)())dump_dup_handle_request,
      (void(*)())dump_dup_handle_reply,
      sizeof(struct dup_handle_request) },
    { (void(*)())dump_open_process_request,
      (void(*)())dump_open_process_reply,
      sizeof(struct open_process_request) },
    { (void(*)())dump_select_request,
      (void(*)())dump_select_reply,
      sizeof(struct select_request) },
    { (void(*)())dump_create_event_request,
      (void(*)())dump_create_event_reply,
      sizeof(struct create_event_request) },
    { (void(*)())dump_event_op_request,
      (void(*)())0,
      sizeof(struct event_op_request) },
    { (void(*)())dump_create_mutex_request,
      (void(*)())dump_create_mutex_reply,
      sizeof(struct create_mutex_request) },
    { (void(*)())dump_release_mutex_request,
      (void(*)())0,
      sizeof(struct release_mutex_request) },
    { (void(*)())dump_create_semaphore_request,
      (void(*)())dump_create_semaphore_reply,
      sizeof(struct create_semaphore_request) },
    { (void(*)())dump_release_semaphore_request,
      (void(*)())dump_release_semaphore_reply,
      sizeof(struct release_semaphore_request) },
    { (void(*)())dump_open_named_obj_request,
      (void(*)())dump_open_named_obj_reply,
      sizeof(struct open_named_obj_request) },
};

static const char * const req_names[REQ_NB_REQUESTS] =
{
    "new_thread",
    "init_thread",
    "terminate_process",
    "terminate_thread",
    "get_process_info",
    "get_thread_info",
    "close_handle",
    "dup_handle",
    "open_process",
    "select",
    "create_event",
    "event_op",
    "create_mutex",
    "release_mutex",
    "create_semaphore",
    "release_semaphore",
    "open_named_obj",
};

void trace_request( enum request req, void *data, int len, int fd )
{
    current->last_req = req;
    printf( "%08x: %s(", (unsigned int)current, req_names[req] );
    dumpers[req].dump_req( data );
    if (len > dumpers[req].size)
    {
        unsigned char *ptr = (unsigned char *)data + dumpers[req].size;
	len -= dumpers[req].size;
        while (len--) printf( ", %02x", *ptr++ );
    }
    if (fd != -1) printf( " ) fd=%d\n", fd );
    else printf( " )\n" );
}

void trace_timeout(void)
{
    printf( "%08x: *timeout*\n", (unsigned int)current );
}

void trace_kill( int exit_code )
{
    printf( "%08x: *killed* exit_code=%d\n",
            (unsigned int)current, exit_code );
}

void trace_reply( struct thread *thread, int type, int pass_fd,
                  struct iovec *vec, int veclen )
{
    if (!thread) return;
    printf( "%08x: %s() = %d",
            (unsigned int)thread, req_names[thread->last_req], type );
    if (veclen)
    {
	printf( " {" );
	if (dumpers[thread->last_req].dump_reply)
	{
	    dumpers[thread->last_req].dump_reply( vec->iov_base );
	    vec++;
	    veclen--;
	}
	for (; veclen; veclen--, vec++)
	{
	    unsigned char *ptr = vec->iov_base;
	    int len = vec->iov_len;
	    while (len--) printf( ", %02x", *ptr++ );
	}
	printf( " }" );
    }
    if (pass_fd != -1) printf( " fd=%d\n", pass_fd );
    else printf( "\n" );
}
