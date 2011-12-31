/*  
 *   Sartoris system call header
 *   
 *   Copyright (C) 2002, 2003, Santiago Bazerque and Nicolas de Galarreta
 *   
 *   sbazerqu@dc.uba.ar                
 *   nicodega@gmail.com
 */


/* sartoris system calls */

#ifndef SYSCALL
#define SYSCALL

#include <sartoris/kernel.h>
#ifdef _METRICS_
#include <sartoris/metrics.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* multitasking */
int create_task(int id, struct task *tsk);
int init_task(int task, int *src, unsigned int size);
int destroy_task(int task_num);
int get_current_task(void);

/* threading */
int create_thread(int id, struct thread *thr);
int destroy_thread(int id);
int run_thread(int id);
int set_thread_run_perms(int thr_id, struct permissions *perms);
int set_thread_run_mode(int thr_id, int priv, int mode);
int get_current_thread(void);

/* paging */
int page_in(int task, void *linear, void *physical, int level, int attrib);
int page_out(int task, void *linear, int level);
int flush_tlb(void);
int get_page_fault(struct page_fault *pf);

/* interrupt handling */
int create_int_handler(int number, int thread, int nesting, int priority);
int destroy_int_handler(int number, int thread);
int ret_from_int(void);
int get_last_int(unsigned int *error_code);
void *get_last_int_addr();
int pop_int();
int push_int(int number);
int resume_int();

/* message passing */
int open_port(int port, int priv, enum usage_mode mode);
int close_port(int port);
int set_port_perm(int port, struct permissions *perms);
int set_port_mode(int port, int priv, enum usage_mode mode);
int send_msg(int to_address, int port, void *msg);
int get_msg(int port, void *msg, int *id);
int get_msgs(int port, int *msgs, int *ids, int maxlen);
int get_msg_count(int port);
int get_msg_counts(int *ports, int *counts, int len);

/* memory sharing */
int share_mem(int target_task, void *addr, int size, int perms);
int claim_mem(int smo_id);
int read_mem(int smo_id, int off, int size, void *dest);
int write_mem(int smo_id, int off, int size, void *src);
int pass_mem(int smo_id, int target_task);
int mem_size(int smo_id);

/* tracing */
int ttrace_begin(int thr_id, int task_id);
int ttrace_end(int thr_id, int task_id);
int ttrace_reg(int thr_id, int reg, void *value, int set);
int ttrace_mem_read(int thr_id, void *src, void *dst, int size);
int ttrace_mem_write(int thr_id, void *src, void *dst, int size);

/* Errors */
int last_error();

/* Events */
#define SARTORIS_EVT_MSG            1
#define SARTORIS_EVT_PORT_CLOSED    2
#define SARTORIS_EVT_INT            3
#define SARTORIS_EVT_INTS           4

struct evt_msg
{
    int evt;
    int id;
    int param;
    int param2;
}  __attribute__ ((__packed__));

struct ints_evt_param{
	unsigned int mask;
	int base;
};

int evt_set_listener(int thread, int port, int interrupt);
int evt_wait(int id, int evt, int evt_param);
int evt_disable(int id, int evt, int evt_param);

#ifdef _METRICS_
int get_metrics(struct sartoris_metrics *m);
#endif

#ifdef __cplusplus
}
#endif

#endif
