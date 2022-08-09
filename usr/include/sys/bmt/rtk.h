/*
 * Addresses
 */
typedef char *ADDR;
#define NOADDR	(ADDR)-1
#ifndef NULL
#define NULL	(ADDR)0
#endif
#ifdef lint
#define void int
#endif

/*
 * Process descriptor
 */
typedef struct
{       unsigned short cnt;
	struct _proc_des *proc;
} SEMAPHORE;
typedef struct _proc_des
{	void 		(*p_addr)();	/* process start address */
	unsigned short  p_pri;          /* process priority */
	ADDR 		p_env;		/* pointer to environment -
					   see `struct env' in Init_process */
	long		p_time;		/* time interval -
					   offset 10 used by Tick */
	unsigned char	p_flag;		/* flags - currently not used */
	SEMAPHORE	*p_sem;		/* process is waiting there */ 
	SEMAPHORE	*t_sem;		/* Time_wait semaphore */
	SEMAPHORE	s_sem;		/* Sleep semaphore */
	struct _proc_des *s_link;	/* semaphore link */
	struct _proc_des *t_link;	/* timer link */
	struct _proc_des *p_link;	/* process link */
	ADDR		p_mem;		/* begin of process memory */
	ADDR		p_maxstack;	/* for debugger:
					   maximal observed stack top */
} PROCESSDES;

typedef PROCESSDES *PROCESS;
typedef SEMAPHORE RESOURCE;

#define Request(res)	Wait(res)
#define	Release(res)	Signal(res)
#define Time_request(res,time)	Time_wait(res,time)
#define Init_res(res)	{(res).cnt=1;(res).proc=(PROCESS)0;}
#define Ini_res		{1,(PROCESS)0}
#define	Init_sem(sem)	{(sem).cnt=0;(sem).proc=(PROCESS)0;}
#define Ini_sem		{0,(PROCESS)0}
#define	Waiting(sem)	(sem).proc

extern PROCESS Init_process(), Kill_proc(), Move_proc();
extern PROCESS Curr_p, First_p, Main_pid, Idle_pid;
extern SEMAPHORE Ready_queue;

/*
 * Interrupts
 */
typedef unsigned short INT_LEVEL;

#define ENABLE		0x2000
#define DISABLE		0x2700
#define INT_LEV1	0x2100
#define INT_LEV2	0x2200
#define INT_LEV3	0x2300
#define INT_LEV4	0x2400
#define INT_LEV5	0x2500
#define INT_LEV6	0x2600

extern ADDR Serv_interr(), Set_interr();

/*
 * Priorities
 */
extern unsigned short Currprio;

#define SUPER		1	/* The highest process priority */

#define Get_prio(proc)	(proc)->p_pri

#define DECLARE_SUPER   register unsigned short oldprio
#define BEGIN_SUPER	{ oldprio = Currprio; Currprio = SUPER; }
#define END_SUPER	{ Currprio = oldprio; Reschedule(); }

/*
 * Memory management
 */
extern ADDR lmalloc(), lrealloc(), calloc();

#define malloc(size)		lmalloc((long)(size))
#define realloc(ptr,size)	lrealloc(ptr,(long)(size))
#define cfree(ptr,nelem,size)	free(ptr)

/*
 * time and date
 */
typedef unsigned long TIME;
typedef struct { unsigned short day, hour, min, sec, msec } DATE;

extern TIME Ctime;

#define Sleep(t)	Time_wait(&Curr_p->s_sem,t)
#define Get_time()	Ctime
#define Set_time(t)	(Ctime = t)

#define Sec_to_time(x)	((TIME)(x) * 1000L)
#define Min_to_time(x)	((TIME)(x) * (60L*1000L))
#define Hour_to_time(x)	((TIME)(x) * (60L*60L*1000L))
#define Day_to_time(x)	((TIME)(x) * (24L*60L*60L*1000L))
#define Date_to_time(day,hour,min,sec,msec) (((((TIME)(day)*24L+(hour))*60L+(min))*60L+(sec))*1000L+(msec))

#define Rel_time(x)	((TIME)(x) - Ctime)
