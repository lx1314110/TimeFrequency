#ifndef	__EXT_ALLOC__
#define	__EXT_ALLOC__







#include "ext_ctx.h"








int initializeContext(struct extctx *ctx);

int ext_read_flash(struct extsave *psave);
int ext_write_flash(struct extsave *psave);

void ext_drv2_mgr6_pr(struct extctx *ctx, u8_t bid13, u8_t bid14);












#endif//__EXT_ALLOC__


