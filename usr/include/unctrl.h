#ifndef unctrl
extern char	*_unctrl[];

# define	unctrl(ch)	(_unctrl[(unsigned) ch])
#endif
