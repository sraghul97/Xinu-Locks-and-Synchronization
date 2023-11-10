
extern uint32 test_and_set(uint32 *, uint32);

//*********************SpinLock**********************************
#define NSPINLOCKS 20

typedef struct 
{
    uint32 flag;
}sl_lock_t;

extern syscall sl_initlock(sl_lock_t *l);
extern syscall sl_lock(sl_lock_t *l);
extern syscall sl_unlock(sl_lock_t *l);


//*********************Lock with guard/queue**********************************
#define NLOCKS 20

typedef struct 
{
    uint32 flag;
    uint32 guard;
    qid16 q;
}lock_t;

extern syscall initlock(lock_t *l);
extern syscall lock(lock_t *l);
extern syscall unlock(lock_t *l);
//extern syscall setpark(pid32 pid);
//extern syscall park(pid32 pid);
//extern syscall unpark(pid32 pid);

//*********************Lock with deadlock detection**********************************
#define NALOCKS 20

typedef struct
{
    uint32 flag;
    uint32 guard;
    qid16 q;
    uint32 pid;
}al_lock_t;

extern syscall al_initlock(al_lock_t *l);
extern syscall al_lock(al_lock_t *l);
extern syscall al_unlock(al_lock_t *l);
extern bool8 al_trylock(al_lock_t *l);
//extern syscall al_setpark(pid32 pid);
//extern syscall al_park(pid32 pid, al_lock_t *l);
//extern syscall al_unpark(pid32 pid);

//*********************Lock with Priority inversion**********************************
#define NPILOCKS 20

typedef struct 
{
    uint32 flag;
    uint32 guard;
    qid16 q;
    uint32 pid;
}pi_lock_t;

extern syscall pi_initlock(pi_lock_t *l);
extern syscall pi_lock(pi_lock_t *l);
extern syscall pi_unlock(pi_lock_t *l);
//extern syscall pi_setpark(pid32 pid);
//extern syscall pi_park(pid32 park_pid, pid32 acquired_pid);
//extern syscall pi_unpark(pid32 acquired_pid, pi_lock_t *l);