/**********sel.h********/
/* commands for driver dependent subroutines                            */
#define SELECT    1
#define POLL      0
#define UNSELECT (-1)

/* results of driver dependent subroutine (to be "ored")                */
#define SEL_IN    1
#define SEL_OUT   2
#define SEL_EX    4

extern int nselect;
