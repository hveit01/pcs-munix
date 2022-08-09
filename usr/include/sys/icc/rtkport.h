typedef struct _message
{	struct _message *link;
	SEMAPHORE reply;
#ifdef MESSAGE_HEADER
	MESSAGE_HEADER
#else
	ADDR data;
#endif
} MESSAGE;

typedef struct
{	MESSAGE *first, *last;
	SEMAPHORE notempty;
} PORT;

#define NOWAIT	0L
#define WAIT	-1L

#define Init_message(msg)	(msg)->link = NULL
#define Reply(msg)		Signal(&(msg)->reply)

extern MESSAGE *Receive();
