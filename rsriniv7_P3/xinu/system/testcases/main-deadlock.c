#include <xinu.h>
#include <stdlib.h>

syscall sync_printf(char *fmt, ...)
{
        intmask mask = disable();
        void *arg = __builtin_apply_args();
        __builtin_apply((void*)kprintf, arg, 100);
        restore(mask);
        return OK;
}

void run_for_ms(uint32 time)
{
	uint32 start = proctab[currpid].runtime;
	while ((proctab[currpid].runtime-start) < time);
}

process tc1_Deadlock(uint32 *SharedVariable, al_lock_t *l1, al_lock_t *l2)
{
	al_lock(l1);
	run_for_ms(100);
	al_lock(l2);

	(*SharedVariable) +=1;
    sync_printf("Hi from tc1_Deadlock(). I am P%d. SharedValue::%d\n",currpid, *SharedVariable);
    
	al_unlock(l2);
    run_for_ms(100);
	al_unlock(l1);		
	return OK;
}

process tc2_HoldAndWait(uint32 *SharedVariable, al_lock_t *mainmutex, al_lock_t *l1, al_lock_t *l2)
{
	al_lock(mainmutex);
    al_lock(l1);
	al_lock(l2);
    run_for_ms(100);
	
	(*SharedVariable) +=1;
    sync_printf("Hi from tc2_HoldAndWait(). I am P%d. SharedValue::%d\n",currpid, *SharedVariable);
    
	run_for_ms(100);
    al_unlock(l2);
    al_unlock(l1);		
	al_unlock(mainmutex);
    return OK;
}

process tc3_Preemption(uint32 *SharedVariable, al_lock_t *l1, al_lock_t *l2)
{
	while (TRUE) 
    {
		if (!(al_trylock(l1)))
			sleepms(50);
		else
        {
            run_for_ms(100);           
            if (!(al_trylock(l2))) 
            {
                run_for_ms(100);
                al_unlock(l1);
                sleepms(50);
            }
            else
            {
                (*SharedVariable) +=1;
                sync_printf("Hi from tc3_Preemption(). I am P%d. SharedValue::%d\n",currpid, *SharedVariable);
    
                al_unlock(l2);
                run_for_ms(100);
                al_unlock(l1);		
                return OK;
            }
        }
	}
}

process tc4_CircularWait(uint32 *SharedVariable, al_lock_t *l1, al_lock_t *l2)
{
    if (l1 > l2)
    {
        al_lock(l1);
        run_for_ms(100);
        al_lock(l2);

        (*SharedVariable) +=1;
        sync_printf("Hi from tc4_CircularWait_1(). I am P%d. SharedValue::%d\n",currpid, *SharedVariable);
        
        al_unlock(l2);
        run_for_ms(100);
        al_unlock(l1);		
        return OK;
    }
    else
    {
        al_lock(l2);
        run_for_ms(100);
        al_lock(l1);

        (*SharedVariable) +=1;
        sync_printf("Hi from tc4_CircularWait_2(). I am P%d. SharedValue::%d\n",currpid, *SharedVariable);
        
        al_unlock(l1);
        run_for_ms(100);
        al_unlock(l2);		
        return OK;       
    }
}


process	main(void)
{	
    uint32 SharedVariable = 0;
	pid32 pid[12]; 
	al_lock_t mutex[12];
	al_lock_t mainmutex;

	kprintf("\n\n=========== TEST 1: Deadlock detection for 3 threads  ===========\n\n");
    al_initlock(&mutex[0]);
    al_initlock(&mutex[1]);
    al_initlock(&mutex[2]);

    pid[0] = create((void *)tc1_Deadlock, INITSTK, 2, "tc1_Deadlock", 3, &SharedVariable, &mutex[0], &mutex[1]);
    pid[1] = create((void *)tc1_Deadlock, INITSTK, 2, "tc1_Deadlock", 3, &SharedVariable, &mutex[1], &mutex[2]);
    pid[2] = create((void *)tc1_Deadlock, INITSTK, 2, "tc1_Deadlock", 3, &SharedVariable, &mutex[2], &mutex[0]);

    kprintf("** 3 Processes created **\n");

    resume(pid[0]);
    sleepms(25);
    resume(pid[1]);
    sleepms(25);
    resume(pid[2]);
    sleepms(25);

    while ((!proctab[pid[0]].deadlock_flag) || (!proctab[pid[1]].deadlock_flag) || (!proctab[pid[2]].deadlock_flag)) 
        sleepms(250);

    kprintf("TEST PASSED\n");

	kprintf("\n\n=========== TEST 2: Deadlock prevention by avoiding hold-and-wait  ===========\n\n");
    al_initlock(&mainmutex);
    al_initlock(&mutex[3]);
    al_initlock(&mutex[4]);
    al_initlock(&mutex[5]);
    
    pid[3] = create((void *)tc2_HoldAndWait, INITSTK, 2, "tc2_HoldAndWait", 4, &SharedVariable, &mainmutex, &mutex[3], &mutex[4]);
    pid[4] = create((void *)tc2_HoldAndWait, INITSTK, 2, "tc2_HoldAndWait", 4, &SharedVariable, &mainmutex, &mutex[4], &mutex[5]);
    pid[5] = create((void *)tc2_HoldAndWait, INITSTK, 2, "tc2_HoldAndWait", 4, &SharedVariable, &mainmutex, &mutex[5], &mutex[3]);

    kprintf("** 3 Processes created **\n");

    resume(pid[3]);
    sleepms(25);
    resume(pid[4]);
    sleepms(25);
    resume(pid[5]);
    sleepms(25);

    receive();
    receive();
    receive();

    kprintf("Deadlock Avoided::P%d-P%d-P%d\n",pid[3], pid[4], pid[5]);
    kprintf("SharedValue -> %d\n",SharedVariable);
    if (SharedVariable == 3)
        kprintf("TEST PASSED\n");
    else
        kprintf("TEST FAILED\n");

    kprintf("\n\n=========== TEST 3: Deadlock prevention by preemption of the thread owning the lock.  ===========\n\n");
    al_initlock(&mutex[6]);
    al_initlock(&mutex[7]);
    al_initlock(&mutex[8]);
    
    pid[6] = create((void *)tc3_Preemption, INITSTK, 2, "tc3_Preemption", 3, &SharedVariable, &mutex[6], &mutex[7]);
    pid[7] = create((void *)tc3_Preemption, INITSTK, 2, "tc3_Preemption", 3, &SharedVariable, &mutex[7], &mutex[8]);
    pid[8] = create((void *)tc3_Preemption, INITSTK, 2, "tc3_Preemption", 3, &SharedVariable, &mutex[8], &mutex[6]);

    kprintf("** 3 Processes created **\n");

    resume(pid[6]);
    sleepms(25);
    resume(pid[7]);
    sleepms(25);
    resume(pid[8]);
    sleepms(25);

    receive();
    receive();
    receive();

    kprintf("Deadlock Avoided::P%d-P%d-P%d\n",pid[6], pid[7], pid[8]);
    kprintf("SharedValue -> %d\n",SharedVariable);
    if (SharedVariable == 6)
        kprintf("TEST PASSED\n");
    else
        kprintf("TEST FAILED\n");
    
    kprintf("\n\n=========== TEST 4: Deadlock prevention by avoiding circular wait  ===========\n\n");
    al_initlock(&mutex[9]);
    al_initlock(&mutex[10]);
    al_initlock(&mutex[11]);

    pid[9] = create((void *)tc4_CircularWait, INITSTK, 2, "tc4_CircularWait", 3, &SharedVariable, &mutex[9], &mutex[10]);
    pid[10] = create((void *)tc4_CircularWait, INITSTK, 2, "tc4_CircularWait", 3, &SharedVariable, &mutex[10], &mutex[11]);
    pid[11] = create((void *)tc4_CircularWait, INITSTK, 2, "tc4_CircularWait", 3, &SharedVariable, &mutex[11], &mutex[9]);

    kprintf("** 3 Processes created **\n");
    
    resume(pid[9]);
    sleepms(25);
    resume(pid[10]);
    sleepms(25);
    resume(pid[11]);
    sleepms(25);

    receive();
    receive();
    receive();

    kprintf("Deadlock Avoided::P%d-P%d-P%d\n",pid[9], pid[10], pid[11]);
	kprintf("SharedValue -> %d\n",SharedVariable);
    if (SharedVariable == 9)
        kprintf("TEST PASSED\n");
    else
        kprintf("TEST FAILED\n");
    
    return OK;
}
