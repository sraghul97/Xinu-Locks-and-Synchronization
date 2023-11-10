#include <xinu.h>
static uint32 pi_lock_t_count = 0;

syscall pi_initlock(pi_lock_t *l)
{
    if (pi_lock_t_count >= NALOCKS) 
        return SYSERR;
    else
    {
        l->flag = FALSE;
        l->guard = FALSE;
        l->q = newqueue(); 
        l->pid = NULL;
        pi_lock_t_count += 1;
        return OK;
    }
}

syscall pi_lock(pi_lock_t *l)
{
    while (test_and_set(&l->guard, TRUE))
        sleepms(QUANTUM);
   
    if (!(l->flag))
    {
        l->flag = TRUE;
        l->pid = currpid;
        l->guard = FALSE;
    }
    else
    {
        enqueue(currpid, l->q);
        pi_setpark(currpid);
        l->guard = FALSE;
        pi_park(currpid, l);
    }
    return OK;
}

syscall pi_unlock(pi_lock_t *l)
{
    while (test_and_set(&l->guard, TRUE))
        sleepms(QUANTUM);
   
    if (isempty(l->q))
    {
        l->flag = FALSE;
        l->guard = FALSE;
    }   
    else
    {
        l->pid = dequeue(l->q);
        pi_unpark(l->pid, l);
    }

    //l->guard = FALSE;
    return OK;
}

syscall pi_setpark(pid32 pid)
{
    intmask mask = disable();
    proctab[pid].park_flag = TRUE;

    restore(mask);
    return OK;
}

syscall pi_park(pid32 park_pid, pi_lock_t *l)
{
    //int32 i;
    intmask mask = disable();
    if (proctab[park_pid].park_flag)
    {
        proctab[park_pid].prstate = PR_PARK;
        proctab[park_pid].pi_lock_acquired_pid = l->pid;
        //for (i = (NPILOCKS-1); i >0; i--) 
        //    proctab[park_pid].pi_lock[i] = proctab[park_pid].pi_lock[i-1];
        proctab[park_pid].pi_lock = l;
        priority_upgrade(park_pid, l->pid);
        proctab[park_pid].park_flag = FALSE;
    }

    restore(mask);
    return OK;
}

syscall pi_unpark(pid32 acquired_pid, pi_lock_t *l)
{
    //int32 i,j;
    intmask mask = disable();
    uint32 LoopIteration = 0;
    proctab[acquired_pid].park_flag = FALSE;
    proctab[acquired_pid].prstate = PR_READY;
    proctab[acquired_pid].pi_lock_acquired_pid = NULL;//////////////////
    proctab[acquired_pid].pi_lock = NULL;
    priority_downgrade(acquired_pid, l);
    
    /*for (i = 0; i < (NPILOCKS-1); i++) 
    {
        if (proctab[acquired_pid].pi_lock[i] == l)
        {
            j = i;
            break;
        }
    }
    for (i = j; i < (NPILOCKS-1); i++) 
        proctab[acquired_pid].pi_lock[i] = proctab[acquired_pid].pi_lock[i+1];
    proctab[acquired_pid].pi_lock[NPILOCKS-1] = NULL;*/
   
    for (LoopIteration = 0; LoopIteration < NPROC; LoopIteration++)
    {
        if(proctab[LoopIteration].pi_lock_acquired_pid == currpid)
             proctab[LoopIteration].pi_lock_acquired_pid = acquired_pid;
    }
    insert(acquired_pid, readylist, proctab[acquired_pid].prprio);
    l->guard = FALSE;
    restore(mask);
    //l->guard = FALSE;
   return OK;
}

syscall priority_upgrade(pid32 lock_park_pid, pid32 lock_acquired_pid)
{
    if (proctab[lock_park_pid].prprio > proctab[lock_acquired_pid].prprio)
    {
        while(!(lock_acquired_pid == NULL)) 
        {
            if (proctab[lock_park_pid].prprio > proctab[lock_acquired_pid].prprio)
            {
                kprintf("priority_change=P%d::%d-%d\n", lock_acquired_pid, proctab[lock_acquired_pid].prprio, proctab[lock_park_pid].prprio);       
                proctab[lock_acquired_pid].prprio = proctab[lock_park_pid].prprio;
                
                //getitem(lock_acquired_pid);
                //insert(lock_acquired_pid, readylist, proctab[lock_acquired_pid].prprio);
                readylist_modify(lock_acquired_pid);
            }
            lock_park_pid = lock_acquired_pid; //////needed? (anyways priority is equal)
            lock_acquired_pid = proctab[lock_acquired_pid].pi_lock_acquired_pid;
        }
        resched();  //to run other process
    }
    return OK;
}

syscall priority_downgrade(pid32 lock_acquired_pid, pi_lock_t *l)
{
    uint32 LoopIteration = 0;
    pri16 priority_before_downgrade = proctab[currpid].prprio;
    pri16 goto_priority = 0;
    //pid32 pid_array[NPROC] = {0};
    //uint32 pid_array_index = 0;

    if (proctab[currpid].prprio > proctab[currpid].original_priority)
    {
        //proctab[currpid].prprio = proctab[currpid].original_priority;
        for (LoopIteration = 0; LoopIteration < NPROC; LoopIteration++)
        {
           if (LoopIteration != currpid)
           {
                //kprintf("Process%d::%d, %d, %d, %d, %d, %d, %d,\n",LoopIteration, proctab[LoopIteration].pi_lock_acquired_pid, currpid, proctab[LoopIteration].pi_lock, l, proctab[LoopIteration].prprio, proctab[currpid].original_priority, proctab[currpid].prprio);
                if ((proctab[LoopIteration].pi_lock_acquired_pid == currpid) && (proctab[LoopIteration].pi_lock != l))
                {
                    //kprintf("&&&&&&Process%d::%d, %d, %d, %d, %d, %d, %d,\n",LoopIteration, proctab[LoopIteration].pi_lock_acquired_pid, currpid, proctab[LoopIteration].pi_lock, l, proctab[LoopIteration].prprio, proctab[currpid].original_priority, proctab[currpid].prprio);
                    //if (proctab[LoopIteration].prprio > proctab[currpid].prprio)
                    //    proctab[currpid].prprio = proctab[LoopIteration].prprio;
                    if (proctab[LoopIteration].prprio > proctab[currpid].original_priority)
                    {
                        if (proctab[LoopIteration].prprio < proctab[currpid].prprio)
                        {
                            //kprintf("#####Process%d::%d, %d, %d, %d, %d, %d, %d, %d\n",LoopIteration, proctab[LoopIteration].pi_lock_acquired_pid, currpid, proctab[LoopIteration].pi_lock, l, proctab[LoopIteration].prprio, proctab[currpid].original_priority, proctab[currpid].prprio);
                            //pid_array[pid_array_index] = proctab[LoopIteration].prprio;
                            //pid_array_index += 1;
                            if (proctab[LoopIteration].prprio > goto_priority)
                            {
                                    goto_priority = proctab[LoopIteration].prprio;
                                    //kprintf("*****Process%d::%d, %d, %d, %d, %d, %d, %d, %d %d\n",LoopIteration, proctab[LoopIteration].pi_lock_acquired_pid, currpid, proctab[LoopIteration].pi_lock, l, proctab[LoopIteration].prprio, proctab[currpid].original_priority, proctab[currpid].prprio, goto_priority);
                            }
                        }
                    }
                }
           }     
        }
        if (goto_priority == 0)
            proctab[currpid].prprio = proctab[currpid].original_priority;
        else
            proctab[currpid].prprio = goto_priority;
        kprintf("priority_change=P%d::%d-%d\n", currpid, priority_before_downgrade, proctab[currpid].prprio);
        readylist_modify(currpid);/////////////////////////////////////////////////////////////
    }
    //kprintf("prio\n");
    if (priority_before_downgrade > proctab[lock_acquired_pid].prprio)
    {
        kprintf("priority_change=P%d::%d-%d\n", lock_acquired_pid, proctab[lock_acquired_pid].prprio, priority_before_downgrade);
        proctab[lock_acquired_pid].prprio = priority_before_downgrade;
    }
    //resched();  //to run other process
    return OK;
}

syscall readylist_modify(pid32 pid) 
{
    qid16	next = firstid(readylist);
	qid16	tail = queuetail(readylist);

    while(next != tail) 
    {
        if (next == pid) 
        {
            getitem(pid);
            insert(pid, readylist, proctab[pid].prprio);
            return OK;
        }
		next = queuetab[next].qnext;
	}
    return OK;
}


