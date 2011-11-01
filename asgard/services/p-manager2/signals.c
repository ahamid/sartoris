/*
*	Process and Memory Manager Service
*
*	Copyright (C) 2002, 2003, 2004, 2005
*       
*	Santiago Bazerque 	sbazerque@gmail.com			
*	Nicolas de Galarreta	nicodega@gmail.com
*
*	
*	Redistribution and use in source and binary forms, with or without 
* 	modification, are permitted provided that conditions specified on 
*	the License file, located at the root project directory are met.
*
*	You should have received a copy of the License along with the code,
*	if not, it can be downloaded from our project site: sartoris.sourceforge.net,
*	or you can contact us directly at the email addresses provided above.
*
*
*/

#include "types.h"
#include "task_thread.h"
#include <services/pmanager/signals.h>
#include "kmalloc.h"
#include "interrupts.h"
#include "layout.h"
#include <sartoris/syscall.h>

UINT32 ticks = 0;			// clock ticks, when it reaches 0xFFFFFFFF it will return to 0 again
INT32 direction = 0;		// each time ticks overflows this will be set to !direction
struct signals signals;		// signals global list

/* Initialize signals container. */
void init_signals()
{
	signals.first_thr = NULL;
	signals.first = NULL;
	ticks = 0;
	direction = 0;
}

void init_thr_signals(struct pm_thread *thr)
{
	thr->signals.next = thr->signals.prev = NULL;
	thr->signals.first = thr->signals.blocking_signal = NULL;
    thr->signals.pending_int = 0;
    thr->signals.stack = NULL;
}

void init_tsk_signals(struct pm_task *tsk)
{
    tsk->signals.handler_ep = NULL;
}

/* Handle incoming signal messages */
void process_signals()
{
	struct wait_for_signal_cmd signal;
	INT32 id;

	/* Get signal requests from signals port */
	while(get_msg_count(PMAN_SIGNALS_PORT) > 0)
	{
		get_msg(PMAN_SIGNALS_PORT, &signal, &id);

		switch(signal.command)
		{
			case WAIT_FOR_SIGNAL:
				wait_signal(&signal, TRUE, id);
				break;
			case WAIT_FOR_SIGNAL_NBLOCK:
				wait_signal(&signal, FALSE, id);
				break;
			case DISCARD_SIGNAL:
				discard_signal((struct discard_signal_cmd*)&signal, id);				
				break;
            case SET_SIGNAL_HANDLER:				
				set_signal_handler((struct set_signal_handler_cmd*)&signal, id);				
				break;
            case SET_SIGNAL_STACK:				
				set_signal_stack((struct set_signal_stack_cmd*)&signal, id);				
				break;
		}
	}
}

/* Handle incoming events */
void process_events()
{
	struct event_cmd evt;
	INT32 id;

	/* Get signal requests from signals port */
	while(get_msg_count(PMAN_EVENTS_PORT) > 0)
	{
		get_msg(PMAN_EVENTS_PORT, &evt, &id);

		if(evt.command == EVENT)
		{
			event(&evt, id);
		}
	}
}

/* Timer handling */
void timer_tick()
{
	struct thr_signal *signal = NULL;
	UINT32 oticks = ticks;
	ticks++;

	if(oticks > ticks)
	{
		// overflow
		direction = !direction;
	}

	/* Now check for timeout on thread signals using global list */
	signal = signals.first;

	while(signal != NULL && signal->dir == direction && signal->timeout == ticks)
	{
		if(signal->task == PMAN_TASK && signal->event_type == PMAN_SLEEP)
		{
     		// it was a sleep command
			send_signal(signal, NULL, SIGNAL_OK);
		}
		else
		{
			send_signal(signal, NULL, SIGNAL_TIMEOUT);
		}

        if(signal->thread->signals.blocking_signal == signal)
	    {
            if(signal->timeout != PMAN_SIGNAL_REPEATING)
                signal->thread->signals.blocking_signal = NULL;

		    /* Reactivate thread */
		    sch_activate(signal->thread);
	    }

		if(signal->timeout != PMAN_SIGNAL_REPEATING)
		{
			remove_signal(signal, signal->thread);
			signal = signals.first;
            kfree(signal);
		}        
	}
}

/* Create a blocking signal for the process */
void wait_signal(struct wait_for_signal_cmd *signal, BOOL blocking, UINT16 task)
{
	/* 
		Now, the process might have sent multiple blocking signal requests 
		if it did, then the last signal expected will be the one unblocking 
		the thread.
	*/
	struct signal_cmd signal_msg;
	struct pm_thread *thread = NULL;
	struct thr_signal *nsignal = NULL;
	struct pm_task *ptask;
	UINT32 otimeout;
    
    thread = thr_get(signal->thr_id);
	ptask = tsk_get(task);

    if(ptask == NULL) return;

    signal_msg.command = SIGNAL;
	signal_msg.thr_id = signal->thr_id;
	signal_msg.event_type = signal->event_type;
	signal_msg.id = signal->id;
	signal_msg.task = signal->task;
	signal_msg.res = 0;
	signal_msg.ret = SIGNAL_FAILED;

	if(thread == NULL || (thread->state != THR_RUNNING && thread->state != THR_BLOCKED 
        && thread->state != THR_WAITING && thread->state != THR_DBG) || (blocking && thread->signals.blocking_signal != NULL))
	{
		// fail
		send_msg(task, signal->signal_port, &signal_msg);
		return;
	}

	/* Fail if we are mmapping. */
	if(ptask->state == TSK_MMAPPING)
	{
		// fail
		send_msg(task, signal->signal_port, &signal_msg);
		return;
	}
    	
	/* Create the signal struct */
	nsignal = (struct thr_signal *)kmalloc(sizeof(struct thr_signal));

	if(nsignal == NULL || signal->thr_id >= MAX_THR || thread->task_id != task
		  || thread->state == THR_NOTHING
		  || thread->state == THR_KILLED
		  || thread->state == THR_EXCEPTION)
	{
		// fail
		send_msg(task, signal->signal_port, &signal_msg);
		return;
	}

	nsignal->thread = thread;
	nsignal->event_type = signal->event_type;
	nsignal->id = signal->id;
	nsignal->task = signal->task;
	nsignal->signal_param = signal->signal_param;
	nsignal->signal_port = signal->signal_port;

	nsignal->infinite = 0;

	if(signal->timeout == PMAN_SIGNAL_TIMEOUT_INFINITE)
	{
		nsignal->infinite = 1;
	}
    else if(signal->timeout == PMAN_SIGNAL_REPEATING)
	{
        if(blocking)
        {
            // cannot set a blocking signal to repeating
            send_msg(task, signal->signal_port, &signal_msg);
            return;
        }
		nsignal->infinite = 1;
        nsignal->timeout = PMAN_SIGNAL_REPEATING;
	}
	else
	{
		otimeout = ROUND_TIMEOUT(signal->timeout);

		nsignal->timeout = otimeout + ticks;

		if(nsignal->timeout < ticks || nsignal->timeout < otimeout)
		{
			nsignal->dir = !direction;
			nsignal->timeout = otimeout - (0xFFFFFFFF - ticks);
		}
		else
		{
			nsignal->dir = direction;
		}
	}

	nsignal->tnext = NULL;
	nsignal->gnext = NULL;
	nsignal->tprev = NULL;
	nsignal->gprev = NULL;

	/* Set blocking signal */
	if(blocking)
	{
		thread->signals.blocking_signal = nsignal;

		sch_deactivate(thread);
	}

	if(signal->event_type == PMAN_INTR && signal->task == PMAN_TASK && !int_signal(thread, nsignal))
	{
		// fail
		send_msg(task, signal->signal_port, &signal_msg);
		kfree(signal);
		return;
	}

	/* Insert signal */
	insert_signal(nsignal, nsignal->thread);
}

void set_signal_handler(struct set_signal_handler_cmd *cmd, UINT16 task)
{
    struct set_signal_handler_res res;
    struct pm_task *tsk = NULL;
    
    res.command = SET_SIGNAL_HANDLER;
    res.thr_id = cmd->thr_id;
    res.result = SIGNAL_FAILED;

    tsk = tsk_get(task);

    if(tsk == NULL 
		  || tsk->state == TSK_NOTHING
		  || tsk->state == TSK_KILLED
		  || tsk->state == TSK_KILLING)
	{
		send_msg(task, cmd->ret_port, &res);
		return;
	}
        
    // check pointers are valid
    if( (cmd->stack != NULL && cmd->handler_ep == NULL)
        || (cmd->handler_ep != NULL 
            && loader_is_exec(tsk, cmd->handler_ep)))
    {
		send_msg(task, cmd->ret_port, &res);
		return;
	}
    
    tsk->signals.handler_ep = cmd->handler_ep;
    tsk->exeptions.exceptions_port = cmd->exceptions_port;
    
    res.result = SIGNAL_OK;
    send_msg(task, cmd->ret_port, &res);
}

void set_signal_stack(struct set_signal_stack_cmd *cmd, UINT16 task)
{
    struct set_signal_handler_res res;
    struct pm_thread *thread = thr_get(cmd->thr_id);
    struct pm_task *tsk = NULL;
    
    res.command = SET_SIGNAL_HANDLER;
    res.thr_id = cmd->thr_id;
    res.result = SIGNAL_FAILED;

    tsk = tsk_get(thread->task_id);

    if(thread == NULL || thread->task_id != task
		  || thread->state == THR_NOTHING
		  || thread->state == THR_KILLED
		  || thread->state == THR_EXCEPTION)
	{
		send_msg(task, cmd->ret_port, &res);
		return;
	}
   
    if(cmd->stack != NULL && (UINT32)cmd->stack < tsk->vmm_info.max_addr)
    {
		send_msg(task, cmd->ret_port, &res);
		return;
	}

    thread->signals.stack = cmd->stack;
    
    res.result = SIGNAL_OK;
    send_msg(task, cmd->ret_port, &res);
}

void discard_signal(struct discard_signal_cmd *dsignal, UINT16 task)
{
	struct pm_thread *thread = thr_get(dsignal->thr_id);
	struct thr_signal *signal = NULL;

	if(thread == NULL || dsignal->thr_id >= MAX_THR || thread->task_id != task
		  || thread->state == THR_NOTHING
		  || thread->state == THR_KILLED
		  || thread->state == THR_EXCEPTION)
	{
		pman_print_dbg("PMAN: DISCARD SIGNAL FAILED\n");
		// fail
		return;
	}

	/* Find a matching signal */
	signal = thread->signals.first;

	while(signal != NULL)
	{
		if(signal->task == dsignal->task 
            && signal->event_type == dsignal->event_type
			&& signal->signal_param == dsignal->signal_param
			&& signal->id == dsignal->id 
			&& signal->signal_port == dsignal->signal_port ) 
			break;
		signal = signal->tnext;		
	}

	if(signal != NULL)
    {
        if(thread->signals.blocking_signal == signal)
	    {
            thread->signals.blocking_signal = NULL;

		    /* Reactivate thread */
		    sch_activate(thread);
	    }
		remove_signal(signal, signal->thread);
        
		kfree(signal);		
	}
}

void event(struct event_cmd *evt, UINT16 task)
{
	struct thr_signal *signal = NULL, *nsignal = NULL;
	struct pm_thread *currthr = NULL;
	struct pm_task *tsk = NULL;

	/* 
	Process events. 
	This will go through signals looking for a match. if event task is 
	PMAN_GLOBAL_EVENT all signals will be considered, if 
	PMAN_SIGNAL_PARAM_IGNORE is set, param won't be taken into account. 
	*/
	if(evt->task == PMAN_GLOBAL_EVENT)
	{
		// as inifinite timeout events are kept outside the 
		// global list, this will go through signals 
		// on each thread
		currthr = signals.first_thr;

		while(currthr != NULL)
		{
			/* Check all signals */
			signal = currthr->signals.first;

			while(signal != NULL)
			{
				nsignal = signal->tnext;
				if(signal->task == task && signal->event_type == evt->event_type 
					&& (signal->signal_param == evt->param || signal->signal_param == (UINT16)PMAN_SIGNAL_PARAM_IGNORE))
				{
                    // ok it matches!
					send_signal(signal, evt, SIGNAL_OK);
					
                    if(signal->thread->signals.blocking_signal == signal)
	                {
                        if(signal->timeout != PMAN_SIGNAL_REPEATING)
                            signal->thread->signals.blocking_signal = NULL;

		                /* Reactivate thread */
		                sch_activate(signal->thread);
	                }

					if(signal->timeout != PMAN_SIGNAL_REPEATING)
					{
						remove_signal(signal, signal->thread);                        
						kfree(signal);
					}
				}
				signal = nsignal;
			}

			/* Continue with next thread */
			currthr = currthr->signals.next;
		}
	}
	else
	{
		if(task >= MAX_TSK) return;

		tsk = tsk_get(evt->task);

        if(tsk == NULL) return;

		/* Go through threads of the specified task */
		currthr = tsk->first_thread;

		while(currthr != NULL)
		{
			/* Check all signals */
			signal = currthr->signals.first;

			while(signal != NULL)
			{
				nsignal = signal->tnext;
				if(signal->task == task && signal->event_type == evt->event_type 
					&& (signal->signal_param == evt->param || signal->signal_param == (UINT16)PMAN_SIGNAL_PARAM_IGNORE))
				{
                    // ok it matches!
					send_signal(signal, evt, SIGNAL_OK);

                    if(signal->thread->signals.blocking_signal == signal)
	                {
                        if(signal->timeout != PMAN_SIGNAL_REPEATING)
                            signal->thread->signals.blocking_signal = NULL;

		                /* Reactivate thread */
		                sch_activate(signal->thread);
	                }

					if(signal->timeout != PMAN_SIGNAL_REPEATING)
					{
						remove_signal(signal, signal->thread);
						kfree(signal);
					}
				}
				signal = nsignal;
			}

			/* Continue with next thread */
			currthr = currthr->next_thread;
		}
	}
}

/* 
Send a signal message based on an event.
NOTE: This won't remove the signal, nor will it modify thread state, or perform any checks.
if evt is NULL it'll send a message with ret.
*/
void send_signal_ex(struct thr_signal *signal, struct event_cmd *evt, INT32 ret, void *eaddr)
{
	struct signal_cmd signal_msg;
	struct pm_thread *thread = signal->thread;
    struct pm_task *tsk = NULL;

	signal_msg.command = SIGNAL;
	signal_msg.thr_id = (UINT16)thread->id;
	signal_msg.event_type = signal->event_type;
	signal_msg.id = signal->id;
	signal_msg.task = signal->task;
					
	if(evt != NULL)
	{
		signal_msg.res = evt->event_res;
		signal_msg.ret = ret;
	}
	else
	{
		signal_msg.res = 0;
		signal_msg.ret = ret;
	}
	
	send_msg(thread->task_id, signal->signal_port, &signal_msg);

    tsk = tsk_get(thread->task_id);

    /* Does the task have a handler for signals? */
    if(tsk->signals.handler_ep != NULL)
    {
        // let the scheduler know this thread must run a soft int
        thread->signals.pending_int = 1;
    }
}

/* Insert a signal */
void insert_signal(struct thr_signal *signal, struct pm_thread *thread)
{
	/* Insert on thread list */
	struct thr_signal *ofirst = thread->signals.first;

    if(thread == NULL) return;

	thread->signals.first = signal;
	signal->tnext = ofirst;
	signal->tprev = NULL;

	if(ofirst != NULL) ofirst->tprev = signal;

	if(!signal->infinite)
	{
		/* Insert on global list (ordered) */
		struct thr_signal *prv = NULL, *curr = signals.first;

		while(curr != NULL && signal_comp(signal, curr) >= 0)
		{
			curr = curr->gnext;
		}

		/* prv should contain the previous signal, and curr the next one */
		ofirst = signals.first;

		if(prv != NULL)
		{
			prv->gnext = signal;
		}
		else
		{
			signals.first = signal;
			ofirst->gprev = signal;
		}

		if(curr != NULL)
		{
			curr->gprev = signal;
		}

		signal->gnext = curr;
		signal->gprev = prv;
	}

	/* If thread is not on the list, add it */
	if(thread->signals.first->tnext == NULL)
	{
		struct pm_thread *otfirst = signals.first_thr;

		signals.first_thr = thread;		
		thread->signals.next = signals.first_thr;
		thread->signals.prev = NULL;
		if(otfirst != NULL) otfirst->signals.prev = thread;
	}
}

/* Remove a signal (it won't free the signal structure */
void remove_signal(struct thr_signal *signal, struct pm_thread *thread)
{
    struct pm_thread *thr = NULL;

	if(thread == NULL) return;

	/* If its the blocking signal, set to NULL */
	if(thread->signals.blocking_signal == signal)
	{
		thread->signals.blocking_signal = NULL;
	}

	/* Remove from global list */
	if(!signal->infinite)
	{
		if(signal->gprev != NULL)
		{
			signal->gprev->gnext = signal->gnext;
		}
		else
		{
			signals.first = signal->gnext;
		}
	
        if(signal->gnext != NULL)
		    signal->gnext->gprev = signal->gprev;
	}
    	
	/* Remove from thread list */
	if(signal->tprev != NULL)
	{
		signal->tprev->tnext = signal->tnext;
	}
	else
	{
		thread->signals.first = signal->tnext;
	}
	if(signal->tnext != NULL)
		signal->tnext->tprev = signal->tprev;

	/* Check if the thread has any more signals 
	   and update first thread.
	*/
	if(thread->signals.first == NULL)
	{
		if(thread->signals.prev == NULL)
		{
			signals.first_thr = thread->signals.next;
		}
		else 
		{
			thr = thr_get(thread->signals.prev->id);
			thr->signals.next = thread->signals.next;
            
            if(thread->signals.next != NULL)
			{
				thr = thr_get(thread->signals.next->id);
				thr->signals.prev = thread->signals.prev;
			}
		}
		thread->signals.next = thread->signals.prev = NULL;
	}

    /*
    if it's an interrupt signal, remove it from the list
    */
    if(signal->event_type == PMAN_INTR && signal->task == PMAN_TASK)
    {
        int_signal_remove(signal);
    }
}

/*
This function will remove all signals for a given thread.
*/
void remove_thr_signals(struct pm_thread *thread)
{
    struct thr_signal *signal = thread->signals.first;

    while(signal)
    {
        remove_signal(signal, thread);
        kfree(signal);
        signal = thread->signals.first;
    }
}

/* Compare two signals, by using also global direction flag 
return: 0 equal, < 0 then s0 < s1, > 0 then s0 > s1.
*/
int signal_comp(struct thr_signal *s0, struct thr_signal *s1)
{
	if(s0->dir == s1->dir)
		return s0->timeout - s1->timeout;

	if(direction)
	{
		/* If direction is 1, then it means signals with dir 0 will be greater than signals with dir 1 */
		return s1->dir - s0->dir;
	}
	else
	{
		/* If direction is 0, then it means signals with dir 1 will be greater than signals with dir 0 */
		return s0->dir - s1->dir;
	}
}


