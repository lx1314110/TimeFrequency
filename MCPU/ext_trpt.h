#ifndef	__EXT_TRPT__
#define	__EXT_TRPT__






#include "ext_ctx.h"






u8_t ext_out_ssm_trpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx);
u8_t ext_out_signal_trpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx);
u8_t ext_out_mode_trpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx);
u8_t ext_out_bid_trpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx);
u8_t ext_out_alm_trpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx);

u8_t ext_out_ver_trpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx);
u8_t ext_mgr_pr_trpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx);












#endif//__EXT_TRPT__


