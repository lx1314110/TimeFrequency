#ifndef	__EXT_CRPT__
#define	__EXT_CRPT__







#include "ext_ctx.h"






u8_t ext_out_ssm_crpt(u8_t save_flag, u8_t ext_sid, u8_t *ext_data, struct extctx *ctx);
u8_t ext_out_signal_crpt(u8_t save_flag, u8_t ext_sid, u8_t *ext_data, struct extctx *ctx);
u8_t ext_out_mode_crpt(u8_t save_flag, u8_t ext_sid, u8_t *ext_data, struct extctx *ctx);
u8_t ext_out_prr_crpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx);

u8_t ext_out_alm_crpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx);










#endif//__EXT_CRPT__


