#include <xinu.h>
static uint32 lock_t_count = 0;

syscall initlock(lock_t *l)
{
    if (lock_t_count >= NLOCKS) 
        return SYSERR;
    else
    {
        l->flag = FALSE;
        l->guard = FALSE;
        l->q = newqueue(); 
        lock_t_count += 1;
        //kprintf("\n\nlock_t_count = %d\n\n",lock_t_count);
        return OK;
    }
}

syscall lock(lock_t *l)
{
    while (test_and_set(&l->guard, TRUE));

    if (!(l->flag))
    {
        l->flag = TRUE;
        l->guard = FALSE;
    }
    else
    {
        enqueue(currpid, l->q);
        setpark(currpid);
        l->guard = FALSE;
        park(currpid);
    }
    return OK;
}

syscall unlock(lock_t *l)
{
    while (test_and_set(&l->guard, TRUE));

    if (isempty(l->q))
        l->flag = FALSE;
    else
        unpark(dequeue(l->q));

    l->guard = FALSE;
    return OK;
}

syscall setpark(pid32 pid)
{
    intmask mask = disable();
    proctab[pid].park_flag = TRUE;

    restore(mask);
    return OK;
}

syscall park(pid32 pid)
{
    intmask mask = disable();
    if (proctab[pid].park_flag)
    {
        proctab[pid].prstate = PR_PARK;
        resched();  //to run other process
        proctab[pid].park_flag = FALSE;
    }

    restore(mask);
    return OK;
}

syscall unpark(pid32 pid)
{
    intmask mask = disable();
    proctab[pid].park_flag = FALSE;
    proctab[pid].prstate = PR_READY;
    insert(pid, readylist, proctab[pid].prprio);

    restore(mask);
    return OK;
}