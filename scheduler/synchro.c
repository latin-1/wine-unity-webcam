/*
 * Win32 process and thread synchronisation
 *
 * Copyright 1997 Alexandre Julliard
 */

#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>
#include "k32obj.h"
#include "heap.h"
#include "process.h"
#include "thread.h"
#include "winerror.h"
#include "syslevel.h"
#include "server.h"
#include "debug.h"

/***********************************************************************
 *           SYNC_BuildWaitStruct
 */
static BOOL32 SYNC_BuildWaitStruct( DWORD count, const HANDLE32 *handles,
                                    BOOL32 wait_all, BOOL32 wait_msg, 
                                    WAIT_STRUCT *wait )
{
    DWORD i;
    K32OBJ **ptr;

    SYSTEM_LOCK();
    wait->count    = count;
    wait->signaled = WAIT_FAILED;
    wait->wait_all = wait_all;
    wait->wait_msg = wait_msg;
    for (i = 0, ptr = wait->objs; i < count; i++, ptr++)
    {
        if (!(*ptr = HANDLE_GetObjPtr( PROCESS_Current(), handles[i],
                                       K32OBJ_UNKNOWN, SYNCHRONIZE,
                                       &wait->server[i] )))
        { 
            ERR(win32, "Bad handle %08x\n", handles[i]); 
            break; 
        }
        if (!K32OBJ_OPS( *ptr )->signaled)
        {
            /* This object type cannot be waited upon */
            ERR(win32, "Cannot wait on handle %08x\n", handles[i]); 
            K32OBJ_DecCount( *ptr );
            break;
        }

    }
    if (i != count)
    {
        /* There was an error */
        wait->wait_msg = FALSE;
        while (i--) K32OBJ_DecCount( wait->objs[i] );
    }
    SYSTEM_UNLOCK();
    return (i == count);
}


/***********************************************************************
 *           SYNC_FreeWaitStruct
 */
static void SYNC_FreeWaitStruct( WAIT_STRUCT *wait )
{
    DWORD i;
    K32OBJ **ptr;
    SYSTEM_LOCK();
    wait->wait_msg = FALSE;
    for (i = 0, ptr = wait->objs; i < wait->count; i++, ptr++)
        K32OBJ_DecCount( *ptr );
    SYSTEM_UNLOCK();
}


/***********************************************************************
 *           SYNC_CheckCondition
 */
static BOOL32 SYNC_CheckCondition( WAIT_STRUCT *wait, DWORD thread_id )
{
    DWORD i;
    K32OBJ **ptr;

    SYSTEM_LOCK();
    if (wait->wait_all)
    {
        for (i = 0, ptr = wait->objs; i < wait->count; i++, ptr++)
        {
            if (!K32OBJ_OPS( *ptr )->signaled( *ptr, thread_id ))
            {
                SYSTEM_UNLOCK();
                return FALSE;
            }
        }
        /* Wait satisfied: tell it to all objects */
        wait->signaled = WAIT_OBJECT_0;
        for (i = 0, ptr = wait->objs; i < wait->count; i++, ptr++)
            if (K32OBJ_OPS( *ptr )->satisfied( *ptr, thread_id ))
                wait->signaled = WAIT_ABANDONED_0;
        SYSTEM_UNLOCK();
        return TRUE;
    }
    else
    {
        for (i = 0, ptr = wait->objs; i < wait->count; i++, ptr++)
        {
            if (K32OBJ_OPS( *ptr )->signaled( *ptr, thread_id ))
            {
                /* Wait satisfied: tell it to the object */
                wait->signaled = WAIT_OBJECT_0 + i;
                if (K32OBJ_OPS( *ptr )->satisfied( *ptr, thread_id ))
                    wait->signaled = WAIT_ABANDONED_0 + i;
                SYSTEM_UNLOCK();
                return TRUE;
            }
        }
        SYSTEM_UNLOCK();
        return FALSE;
    }
}


/***********************************************************************
 *           SYNC_WaitForCondition
 */
void SYNC_WaitForCondition( WAIT_STRUCT *wait, DWORD timeout )
{
    DWORD i, thread_id = GetCurrentThreadId();
    LONG count;
    K32OBJ **ptr;
    sigset_t set;

    SYSTEM_LOCK();
    if (SYNC_CheckCondition( wait, thread_id ))
        goto done;  /* Condition already satisfied */
    if (!timeout)
    {
        /* No need to wait */
        wait->signaled = WAIT_TIMEOUT;
        goto done;
    }

    /* Add ourselves to the waiting list of all objects */

    for (i = 0, ptr = wait->objs; i < wait->count; i++, ptr++)
        K32OBJ_OPS( *ptr )->add_wait( *ptr, thread_id );

    /* Release the system lock completely */

    count = SYSTEM_LOCK_COUNT();
    for (i = count; i > 0; i--) SYSTEM_UNLOCK();

    /* Now wait for it */

    TRACE(win32, "starting wait (%p %04x)\n",
		 THREAD_Current(), THREAD_Current()->teb_sel );

    sigprocmask( SIG_SETMASK, NULL, &set );
    sigdelset( &set, SIGUSR1 );
    sigdelset( &set, SIGALRM );
    if (timeout != INFINITE32)
    {
        while (wait->signaled == WAIT_FAILED)
        {
            struct itimerval timer;
            DWORD start_ticks, elapsed;
            timer.it_interval.tv_sec = timer.it_interval.tv_usec = 0;
            timer.it_value.tv_sec = timeout / 1000;
            timer.it_value.tv_usec = (timeout % 1000) * 1000;
            start_ticks = GetTickCount();
            setitimer( ITIMER_REAL, &timer, NULL );
            sigsuspend( &set );
            if (wait->signaled != WAIT_FAILED) break;
            /* Recompute the timer value */
            elapsed = GetTickCount() - start_ticks;
            if (elapsed >= timeout) wait->signaled = WAIT_TIMEOUT;
            else timeout -= elapsed;
        }
    }
    else
    {
        while (wait->signaled == WAIT_FAILED)
        {
            sigsuspend( &set );
        }
    }

    /* Grab the system lock again */

    while (count--) SYSTEM_LOCK();
    TRACE(win32, "wait finished (%p %04x)\n",
		 THREAD_Current(), THREAD_Current()->teb_sel );

    /* Remove ourselves from the lists */

    for (i = 0, ptr = wait->objs; i < wait->count; i++, ptr++)
        K32OBJ_OPS( *ptr )->remove_wait( *ptr, thread_id );

done:
    SYSTEM_UNLOCK();
}


/***********************************************************************
 *           SYNC_DummySigHandler
 *
 * Dummy signal handler
 */
static void SYNC_DummySigHandler(void)
{
}


/***********************************************************************
 *           SYNC_SetupSignals
 *
 * Setup signal handlers for a new thread.
 * FIXME: should merge with SIGNAL_Init.
 */
void SYNC_SetupSignals(void)
{
    sigset_t set;
    SIGNAL_SetHandler( SIGUSR1, SYNC_DummySigHandler, 0 );
    /* FIXME: conflicts with system timers */
    SIGNAL_SetHandler( SIGALRM, SYNC_DummySigHandler, 0 );
    sigemptyset( &set );
    /* Make sure these are blocked by default */
    sigaddset( &set, SIGUSR1 );
    sigaddset( &set, SIGALRM );
    sigprocmask( SIG_BLOCK , &set, NULL);
}


/***********************************************************************
 *           SYNC_WakeUp
 */
void SYNC_WakeUp( THREAD_QUEUE *wait_queue, DWORD max )
{
    THREAD_ENTRY *entry;

    if (!max) max = INFINITE32;
    SYSTEM_LOCK();
    if (!*wait_queue)
    {
        SYSTEM_UNLOCK();
        return;
    }
    entry = (*wait_queue)->next;
    for (;;)
    {
        THDB *thdb = entry->thread;
        if (SYNC_CheckCondition( &thdb->wait_struct, THDB_TO_THREAD_ID(thdb) ))
        {
            TRACE(win32, "waking up %04x (pid %d)\n", thdb->teb_sel, thdb->unix_pid );
            if (thdb->unix_pid)
	    	kill( thdb->unix_pid, SIGUSR1 );
	    else
	    	FIXME(win32,"have got unix_pid 0\n");
            if (!--max) break;
        }
        if (entry == *wait_queue) break;
        entry = entry->next;
    }
    SYSTEM_UNLOCK();
}

/***********************************************************************
 *           SYNC_MsgWakeUp
 */
void SYNC_MsgWakeUp( THDB *thdb )
{
    SYSTEM_LOCK();

    if (!thdb) 
    {
        SYSTEM_UNLOCK();
        return;
    }

    if (thdb->wait_struct.wait_msg)
    {
        thdb->wait_struct.signaled = thdb->wait_struct.count;

        TRACE(win32, "waking up %04x for message\n", thdb->teb_sel );
        if (thdb->unix_pid)
            kill( thdb->unix_pid, SIGUSR1 );
        else
            FIXME(win32,"have got unix_pid 0\n");
    }

    SYSTEM_UNLOCK();
}

/***********************************************************************
 *           SYNC_DoWait
 */
DWORD SYNC_DoWait( DWORD count, const HANDLE32 *handles,
                   BOOL32 wait_all, DWORD timeout, 
                   BOOL32 alertable, BOOL32 wait_msg )
{
    WAIT_STRUCT *wait = &THREAD_Current()->wait_struct;

    if (count > MAXIMUM_WAIT_OBJECTS)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return WAIT_FAILED;
    }

    if (alertable)
        FIXME(win32, "alertable not implemented\n" );

    SYSTEM_LOCK();
    if (!SYNC_BuildWaitStruct( count, handles, wait_all, wait_msg, wait ))
        wait->signaled = WAIT_FAILED;
    else
    {
        int i;
        /* Check if we can use a server wait */
        for (i = 0; i < count; i++)
            if (wait->server[i] == -1) break;
        if (i == count)
        {
            int flags = 0;
            SYSTEM_UNLOCK();
            if (wait_all) flags |= SELECT_ALL;
            if (alertable) flags |= SELECT_ALERTABLE;
            if (wait_msg) flags |= SELECT_MSG;
            if (timeout != INFINITE32) flags |= SELECT_TIMEOUT;
            return CLIENT_Select( count, wait->server, flags, timeout );
        }
        else
        {
            /* Now wait for it */
            SYNC_WaitForCondition( wait, timeout );
            SYNC_FreeWaitStruct( wait );
        }
    }
    SYSTEM_UNLOCK();
    return wait->signaled;
}

/***********************************************************************
 *              Sleep  (KERNEL32.679)
 */
VOID WINAPI Sleep( DWORD timeout )
{
    SYNC_DoWait( 0, NULL, FALSE, timeout, FALSE, FALSE );
}

/******************************************************************************
 *              SleepEx   (KERNEL32.680)
 */
DWORD WINAPI SleepEx( DWORD timeout, BOOL32 alertable )
{
    DWORD ret = SYNC_DoWait( 0, NULL, FALSE, timeout, alertable, FALSE );
    if (ret != WAIT_IO_COMPLETION) ret = 0;
    return ret;
}


/***********************************************************************
 *           WaitForSingleObject   (KERNEL32.723)
 */
DWORD WINAPI WaitForSingleObject( HANDLE32 handle, DWORD timeout )
{
    return SYNC_DoWait( 1, &handle, FALSE, timeout, FALSE, FALSE );
}


/***********************************************************************
 *           WaitForSingleObjectEx   (KERNEL32.724)
 */
DWORD WINAPI WaitForSingleObjectEx( HANDLE32 handle, DWORD timeout,
                                    BOOL32 alertable )
{
    return SYNC_DoWait( 1, &handle, FALSE, timeout, alertable, FALSE );
}


/***********************************************************************
 *           WaitForMultipleObjects   (KERNEL32.721)
 */
DWORD WINAPI WaitForMultipleObjects( DWORD count, const HANDLE32 *handles,
                                     BOOL32 wait_all, DWORD timeout )
{
    return SYNC_DoWait( count, handles, wait_all, timeout, FALSE, FALSE );
}


/***********************************************************************
 *           WaitForMultipleObjectsEx   (KERNEL32.722)
 */
DWORD WINAPI WaitForMultipleObjectsEx( DWORD count, const HANDLE32 *handles,
                                       BOOL32 wait_all, DWORD timeout,
                                       BOOL32 alertable )
{
    return SYNC_DoWait( count, handles, wait_all, timeout, alertable, FALSE );
}


/***********************************************************************
 *           WIN16_WaitForSingleObject   (KERNEL.460)
 */
DWORD WINAPI WIN16_WaitForSingleObject( HANDLE32 handle, DWORD timeout )
{
    DWORD retval;

    SYSLEVEL_ReleaseWin16Lock();
    retval = WaitForSingleObject( handle, timeout );
    SYSLEVEL_RestoreWin16Lock();

    return retval;
}

/***********************************************************************
 *           WIN16_WaitForMultipleObjects   (KERNEL.461)
 */
DWORD WINAPI WIN16_WaitForMultipleObjects( DWORD count, const HANDLE32 *handles,
                                           BOOL32 wait_all, DWORD timeout )
{
    DWORD retval;

    SYSLEVEL_ReleaseWin16Lock();
    retval = WaitForMultipleObjects( count, handles, wait_all, timeout );
    SYSLEVEL_RestoreWin16Lock();

    return retval;
}

