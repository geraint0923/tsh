#ifndef __SIG_HANDLER_H__
#define __SIG_HANDLER_H__

extern void init_block_set();

extern void block_signals();

extern void unblock_signals();

extern void int_handler(int no);

extern void stp_handler(int no);

extern void chld_handler(int no);

#endif // __SIG_HANDLER_H__
