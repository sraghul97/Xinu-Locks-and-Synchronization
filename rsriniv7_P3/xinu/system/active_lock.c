#include <xinu.h>
static uint32 al_lock_t_count = 0;

syscall al_initlock(al_lock_t *l)
{
    if (al_lock_t_count >= NALOCKS) 
        return SYSERR;
    else
    {
        l->flag = FALSE;
        l->guard = FALSE;
        l->q = newqueue(); 
        l->pid = NULL;
        al_lock_t_count += 1;
        //kprintf("\n\nal_lock_t_count = %d\n\n",al_lock_t_count);
        return OK;
    }
}

syscall al_lock(al_lock_t *l)
{
    while (test_and_set(&l->guard, TRUE))
         sleepms(QUANTUM);

    if (!(l->flag))
    {
        l->flag = TRUE;
        l->pid = currpid;
        //kprintf("al_lock1_%d", l->pid);
        l->guard = FALSE;
    }
    else
    {
        enqueue(currpid, l->q);
        al_setpark(currpid);
        l->guard = FALSE;
        //kprintf("al_lock2_%d", l->pid);
        al_park(currpid, l);
    }
    return OK;
}

syscall al_unlock(al_lock_t *l)
{
    while (test_and_set(&l->guard, TRUE))
        sleepms(QUANTUM);

    if (isempty(l->q))
        l->flag = FALSE;
    else
    {
        l->pid = dequeue(l->q);
        al_unpark(l->pid);
    }

    l->guard = FALSE;
    return OK;
}

bool8 al_trylock(al_lock_t *l)
{
    if(test_and_set(&l->flag, TRUE) == FALSE)
    {
        l->pid = currpid;
        return TRUE;
    }
    else
        return FALSE;
}

syscall al_setpark(pid32 pid)
{
    intmask mask = disable();
    proctab[pid].park_flag = TRUE;

    restore(mask);
    return OK;
}

syscall al_park(pid32 pid, al_lock_t *l)
{
    intmask mask = disable();
    if (proctab[pid].park_flag)
    {
        proctab[pid].prstate = PR_PARK;
        proctab[pid].al_lock_acquired_pid = l->pid;
        deadlock_detection(l->pid);
        resched();  //to run other process
        proctab[pid].park_flag = FALSE;
    }

    restore(mask);
    return OK;
}

syscall al_unpark(pid32 pid)
{
    intmask mask = disable();
    proctab[pid].park_flag = FALSE;
    proctab[pid].prstate = PR_READY;
    proctab[pid].al_lock_acquired_pid = NULL;
    insert(pid, readylist, proctab[pid].prprio);

    restore(mask);
    return OK;
}

syscall deadlock_detection(pid32 pid)
{
    pid32 deadlock_tree[NPROC] = {0};
    uint32 LoopIteration = 0;
    uint32 LoopIteration1 = 0;
 
    for(LoopIteration = 0; LoopIteration < NPROC; LoopIteration++)
    {
        deadlock_tree[LoopIteration] = pid;
        if (!proctab[deadlock_tree[LoopIteration]].deadlock_flag)
        {
            for(LoopIteration1 = 0; LoopIteration1 < LoopIteration; LoopIteration1++)
            {
                if (pid == deadlock_tree[LoopIteration1])
                {   
                    kprintf("deadlock_detected=");
                    array_sorting_and_printing(deadlock_tree, LoopIteration);
                    /*for(LoopIteration1 = 0; LoopIteration1 < LoopIteration; LoopIteration1++)
                        {
                            kprintf("P%d-", deadlock_tree[LoopIteration1]);
                            proctab[deadlock_tree[LoopIteration1]].deadlock_flag = TRUE;
                        }*/
                    return OK;
                }
            }
        }
        //kprintf("procta1_%d--%d\n", pid, LoopIteration );
        //kprintf("proctab[%d].al_lock_acquired_pid = %d\n", pid, proctab[pid].al_lock_acquired_pid);
        //kprintf("procta2_\n");
        if (proctab[pid].al_lock_acquired_pid == NULL)
            return OK;
        pid = proctab[pid].al_lock_acquired_pid;
    }   
    return OK;
}

void array_sorting_and_printing(uint32 deadlock_tree[], uint32 ArraySize)
{
    uint32 Iteration1, Iteration2, temp;
    for (Iteration1 = 0; Iteration1 < (ArraySize - 1); Iteration1++)
    {
        for (Iteration2 = 0; Iteration2< (ArraySize - Iteration1 - 1); Iteration2++)
        {
            if (deadlock_tree[Iteration2] > deadlock_tree[Iteration2 + 1])
            {
                temp = deadlock_tree[Iteration2 + 1];
                deadlock_tree[Iteration2 + 1] = deadlock_tree[Iteration2];
                deadlock_tree[Iteration2] = temp;
            }
        }
    }
    for(Iteration1 = 0; Iteration1 < ArraySize; Iteration1++)
    {
        if (Iteration1 == ArraySize -1)
            kprintf("P%d\n", deadlock_tree[Iteration1]);
        else
            kprintf("P%d-", deadlock_tree[Iteration1]);
        proctab[deadlock_tree[Iteration1]].deadlock_flag = TRUE;
    }
}

