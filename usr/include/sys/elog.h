struct iotime {
	long    io_cnt;         /* number of read/writes */
	long	io_bcnt;	/* total blocks transferred */
	time_t	io_resp;	/* total block response time */
	time_t	io_act;		/* total drive active time (cumulative utilization) */
};
