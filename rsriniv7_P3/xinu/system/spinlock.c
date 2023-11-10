#include <xinu.h>
static uint32 sl_lock_t_count = 0;

syscall sl_initlock(sl_lock_t *l)
{  
    if (sl_lock_t_count >= NSPINLOCKS) 
        return SYSERR;
    else
    {
        l->flag = FALSE;
        sl_lock_t_count += 1;
        //kprintf("\n\nsl_lock_t_count = %d\n\n",sl_lock_t_count);
        return OK;
    }
}

syscall sl_lock(sl_lock_t *l)
{
    while(test_and_set(&l->flag, TRUE));
    return OK;
}

syscall sl_unlock(sl_lock_t *l)
{
    l->flag = FALSE;
    return OK;
}