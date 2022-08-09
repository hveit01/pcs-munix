
/* Header file          t c p _ t o o l s . h           */
/* Version 1.0  12/06/86   j&j                          */

/* this file contains extern declarations for all the tcp tool functions */
/* implemented in the files:                                             */
/*      tcp_desc.c      Decision functions frequently used               */
/*      tcp_help.c      tcb management routines (insert/release etc.)    */
/*      tcp_act.c       Action procedures and Policy decision functions  */

/* All decision functions from 'tcp_desc.c' */
/*      these are mostly boolean or enum-type functions */

extern      boolean         ack_on();
extern      boolean         rst_on();
extern      boolean         fin_on();
extern      boolean         syn_on();
extern enum tcp_status_type ack_1_status_test();
extern enum tcp_status_type ack_2_status_test();
extern      boolean         fin_acked();
extern      boolean         fin_seen();
extern enum tcp_ge_type     svprec_vs_segprec();
extern      boolean         res_suff_send();
extern      boolean         sec_match();
extern      boolean         sec_prec_allowed();
extern      boolean         sec_prec_match();
extern enum tcp_status_type seqno_status();
extern      boolean         syn_in_window();


/* All TCB management functions from 'tcp_help.c' */
/*      these are (struct tcb *) functions */

extern    struct tcb        *search_state_port_tcb();
extern    struct tcb        *search_tcb();
extern    struct tcb        *alloc_tcb();
extern    boolean           tcp_port_used();


/* All action and policy functions from 'tcp_act.c'  */
/*      all types of procedures including 'void' are here */

extern    void              Accept();
extern    int               Accept_policy();
extern    boolean           Ack_policy(); /* ??? */
extern    void              Check_urg();
extern    void              Clear_queue();
extern    void              Conn_open();
extern    void              Deliver();
extern    boolean           Dispatch();
extern    void              Dm_add_to_send();
extern    void              Dm_add_to_recv();
extern    void              Dm_copy_from_send();
extern    void              Dm_rm_from_recv();
extern    void              Dm_rm_from_send();
extern    void              Error(); /* ??? */
extern    void              Format_net_params();
extern    Ushort            Gen_id();
extern    Ulong             Gen_isn();
extern    void              Gen_syn();
extern    void              Load_security(); /* ??? */
extern    void              New_allocation();
extern    void              Open();
extern    void              Openfail();
extern    void              Part_reset();
extern    void              Recompute_window_size(); /* ??? */
extern    void              Record_syn();
extern    void              Report_timeout(); /* ??? */
extern    void              Requeue_oldest(); /* ??? */
extern    void              Reset();
extern    void              Reset_self();
extern    void              Restart_time_wait(); /* ??? */
extern    void              Retransmit(); /* ??? */
extern    void              Save_fin();
extern    boolean           Save_send_data(); /* ??? */
extern    void              Send_ack();
extern    void              Send_fin();
extern    void              Send_new_data();
extern    short             Send_policy();
extern    void              Set_fin();
extern    void              Start_time_wait(); /* ??? */
extern    void              Update();

