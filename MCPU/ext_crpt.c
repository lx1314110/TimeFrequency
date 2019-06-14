#include <stdio.h>
#include <string.h>

#include "ext_ctx.h"
#include "ext_crpt.h"
//#include "memwatch.h"



#define MGR_RALM_NUM 4


/*
  1	成功
  0	失败
*/
u8_t ext_out_ssm_crpt(u8_t save_flag, u8_t ext_sid, u8_t *ext_data, struct extctx *ctx)
{
    int _sid;

    if((ext_sid >= EXT_1_OUT_LOWER) && (ext_sid <= EXT_1_OUT_UPPER))
    {
        _sid = ext_sid - EXT_1_OUT_LOWER;
        if( EXT_OUT16 == ctx->extBid[0][_sid] ||
                EXT_OUT32 == ctx->extBid[0][_sid] ||
                EXT_OUT16S == ctx->extBid[0][_sid] ||
                EXT_OUT32S == ctx->extBid[0][_sid] )
        {
            memcpy(ctx->out[ext_sid - EXT_1_OUT_OFFSET].outSsm, ext_data, 2);

            if(save_flag)
            {
                memcpy(ctx->save.outSsm[ext_sid - EXT_1_OUT_OFFSET], ext_data, 2);
            }
        }
    }
    else if((ext_sid >= EXT_2_OUT_LOWER) && (ext_sid <= EXT_2_OUT_UPPER))
    {
        _sid = ext_sid - EXT_2_OUT_LOWER;
        if( EXT_OUT16 == ctx->extBid[1][_sid] ||
                EXT_OUT32 == ctx->extBid[1][_sid] ||
                EXT_OUT16S == ctx->extBid[1][_sid] ||
                EXT_OUT32S == ctx->extBid[1][_sid] )
        {
            memcpy(ctx->out[ext_sid - EXT_2_OUT_OFFSET].outSsm, ext_data, 2);

            if(save_flag)
            {
                memcpy(ctx->save.outSsm[ext_sid - EXT_2_OUT_OFFSET], ext_data, 2);
            }
        }
    }
    else if((ext_sid >= EXT_3_OUT_LOWER) && (ext_sid <= EXT_3_OUT_UPPER))
    {
        if(ext_sid < 58)//except ':;'
        {
            _sid = ext_sid - EXT_3_OUT_LOWER;
            if( EXT_OUT16 == ctx->extBid[2][_sid] ||
                    EXT_OUT32 == ctx->extBid[2][_sid] ||
                    EXT_OUT16S == ctx->extBid[2][_sid] ||
                    EXT_OUT32S == ctx->extBid[2][_sid])
            {
                memcpy(ctx->out[ext_sid - EXT_3_OUT_OFFSET].outSsm, ext_data, 2);

                if(save_flag)
                {
                    memcpy(ctx->save.outSsm[ext_sid - EXT_3_OUT_OFFSET], ext_data, 2);
                }
            }
        }
        else
        {
            _sid = ext_sid - EXT_3_OUT_LOWER - 2;
            if( EXT_OUT16 == ctx->extBid[2][_sid] ||
                    EXT_OUT32 == ctx->extBid[2][_sid] ||
                    EXT_OUT16S == ctx->extBid[2][_sid] ||
                    EXT_OUT32S == ctx->extBid[2][_sid] )
            {
                memcpy(ctx->out[ext_sid - EXT_3_OUT_OFFSET - 2].outSsm, ext_data, 2);

                if(save_flag)
                {
                    memcpy(ctx->save.outSsm[ext_sid - EXT_3_OUT_OFFSET - 2], ext_data, 2);
                }
            }
        }
    }
    else
    {
        return 0;
    }

    return 1;
}





/*
  1	成功
  0	失败
*/
u8_t ext_out_signal_crpt(u8_t save_flag, u8_t ext_sid, u8_t *ext_data, struct extctx *ctx)
{
    int _sid;

    if((ext_sid >= EXT_1_OUT_LOWER) && (ext_sid <= EXT_1_OUT_UPPER))
    {
        _sid = ext_sid - EXT_1_OUT_LOWER;
        if( EXT_OUT16 == ctx->extBid[0][_sid] ||
                EXT_OUT32 == ctx->extBid[0][_sid] ||
                EXT_OUT16S == ctx->extBid[0][_sid] ||
                EXT_OUT32S == ctx->extBid[0][_sid])
        {
            memcpy(ctx->out[ext_sid - EXT_1_OUT_OFFSET].outSignal, ext_data, 16);

            if(save_flag)
            {
                memcpy(ctx->save.outSignal[ext_sid - EXT_1_OUT_OFFSET], ext_data, 16);
            }
        }
    }
    else if((ext_sid >= EXT_2_OUT_LOWER) && (ext_sid <= EXT_2_OUT_UPPER))
    {
        _sid = ext_sid - EXT_2_OUT_LOWER;
        if( EXT_OUT16 == ctx->extBid[1][_sid] ||
                EXT_OUT32 == ctx->extBid[1][_sid] ||
                EXT_OUT16S == ctx->extBid[1][_sid] ||
                EXT_OUT32S == ctx->extBid[1][_sid] )
        {
            memcpy(ctx->out[ext_sid - EXT_2_OUT_OFFSET].outSignal, ext_data, 16);

            if(save_flag)
            {
                memcpy(ctx->save.outSignal[ext_sid - EXT_2_OUT_OFFSET], ext_data, 16);
            }
        }
    }
    else if((ext_sid >= EXT_3_OUT_LOWER) && (ext_sid <= EXT_3_OUT_UPPER))
    {
        if(ext_sid < 58)//except ':;'
        {
            _sid = ext_sid - EXT_3_OUT_LOWER;
            if( EXT_OUT16 == ctx->extBid[2][_sid] ||
                    EXT_OUT32 == ctx->extBid[2][_sid] ||
                    EXT_OUT16S == ctx->extBid[2][_sid] ||
                    EXT_OUT32S == ctx->extBid[2][_sid] )
            {
                memcpy(ctx->out[ext_sid - EXT_3_OUT_OFFSET].outSignal, ext_data, 16);

                if(save_flag)
                {
                    memcpy(ctx->save.outSignal[ext_sid - EXT_3_OUT_OFFSET], ext_data, 16);
                }
            }
        }
        else
        {
            _sid = ext_sid - EXT_3_OUT_LOWER - 2;
            if( EXT_OUT16 == ctx->extBid[2][_sid] ||
                    EXT_OUT32 == ctx->extBid[2][_sid] ||
                    EXT_OUT16S == ctx->extBid[2][_sid] ||
                    EXT_OUT32S == ctx->extBid[2][_sid] )
            {
                memcpy(ctx->out[ext_sid - EXT_3_OUT_OFFSET - 2].outSignal, ext_data, 16);

                if(save_flag)
                {
                    memcpy(ctx->save.outSignal[ext_sid - EXT_3_OUT_OFFSET - 2], ext_data, 16);
                }
            }
        }
    }
    else
    {
        return 0;
    }

    return 1;
}







/*
  1	成功
  0	失败
*/
u8_t ext_out_mode_crpt(u8_t save_flag, u8_t ext_sid, u8_t *ext_data, struct extctx *ctx)
{
    int _sid;
    if((ext_sid >= EXT_1_OUT_LOWER) && (ext_sid <= EXT_1_OUT_UPPER))
    {
        _sid = ext_sid - EXT_1_OUT_LOWER;
        if( EXT_OUT16 == ctx->extBid[0][_sid] ||
                EXT_OUT32 == ctx->extBid[0][_sid] ||
                EXT_OUT16S == ctx->extBid[0][_sid] ||
                EXT_OUT32S == ctx->extBid[0][_sid] )
        {
            memcpy(ctx->out[ext_sid - EXT_1_OUT_OFFSET].outPR, ext_data, 3);
            if(save_flag)
            {
                memcpy(ctx->save.outPR[ext_sid - EXT_1_OUT_OFFSET], ext_data, 3);
            }
        }
    }
    else if((ext_sid >= EXT_2_OUT_LOWER) && (ext_sid <= EXT_2_OUT_UPPER))
    {
        _sid = ext_sid - EXT_2_OUT_LOWER;
        if( EXT_OUT16 == ctx->extBid[1][_sid] ||
                EXT_OUT32 == ctx->extBid[1][_sid] ||
                EXT_OUT16S == ctx->extBid[1][_sid] ||
                EXT_OUT32S == ctx->extBid[1][_sid] )
        {
            memcpy(ctx->out[ext_sid - EXT_2_OUT_OFFSET].outPR, ext_data, 3);

            if(save_flag)
            {
                memcpy(ctx->save.outPR[ext_sid - EXT_2_OUT_OFFSET], ext_data, 3);
            }
        }
    }
    else if((ext_sid >= EXT_3_OUT_LOWER) && (ext_sid <= EXT_3_OUT_UPPER))
    {
        if(ext_sid < 58)//except ':;'
        {
            _sid = ext_sid - EXT_3_OUT_LOWER;
            if( EXT_OUT16 == ctx->extBid[2][_sid] ||
                    EXT_OUT32 == ctx->extBid[2][_sid] ||
                    EXT_OUT16S == ctx->extBid[2][_sid] ||
                    EXT_OUT32S == ctx->extBid[2][_sid] )
            {
                memcpy(ctx->out[ext_sid - EXT_3_OUT_OFFSET].outPR, ext_data, 3);

                if(save_flag)
                {
                    memcpy(ctx->save.outPR[ext_sid - EXT_3_OUT_OFFSET], ext_data, 3);
                }
            }
        }
        else
        {
            _sid = ext_sid - EXT_3_OUT_LOWER - 2;
            if( EXT_OUT16 == ctx->extBid[2][_sid] ||
                    EXT_OUT32 == ctx->extBid[2][_sid] ||
                    EXT_OUT16S == ctx->extBid[2][_sid] ||
                    EXT_OUT32S == ctx->extBid[2][_sid] )
            {
                memcpy(ctx->out[ext_sid - EXT_3_OUT_OFFSET - 2].outPR, ext_data, 3);

                if(save_flag)
                {
                    memcpy(ctx->save.outPR[ext_sid - EXT_3_OUT_OFFSET - 2], ext_data, 3);
                }
            }
        }
    }
    else
    {
        return 0;
    }

    return 1;
}







/*
  1	成功
  0	失败
*/
u8_t ext_out_prr_crpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx)
{
    return 1;
}







/*
  1	成功
  0	失败
*/
u8_t ext_out_alm_crpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx)
{
    if((ext_sid >= EXT_1_OUT_LOWER) && (ext_sid <= EXT_1_OUT_UPPER))
    {
        memcpy(ctx->out[ext_sid - EXT_1_OUT_OFFSET].outAlm, ext_data, 6);
    }
    else if((ext_sid >= EXT_2_OUT_LOWER) && (ext_sid <= EXT_2_OUT_UPPER))
    {
        memcpy(ctx->out[ext_sid - EXT_2_OUT_OFFSET].outAlm, ext_data, 6);
    }
    else if((ext_sid >= EXT_3_OUT_LOWER) && (ext_sid <= EXT_3_OUT_UPPER))
    {
        if(ext_sid < 58)//except ':;'
        {
            memcpy(ctx->out[ext_sid - EXT_3_OUT_OFFSET].outAlm, ext_data, 6);
        }
        else
        {
            memcpy(ctx->out[ext_sid - EXT_3_OUT_OFFSET - 2].outAlm, ext_data, 6);
        }
    }
    else if(EXT_1_MGR_PRI == ext_sid)
    {
        memcpy(ctx->mgr[0].mgrAlm, ext_data, MGR_RALM_NUM);
    }
    else if(EXT_1_MGR_RSV == ext_sid)
    {
        memcpy(ctx->mgr[1].mgrAlm, ext_data, MGR_RALM_NUM);
    }
    else if(EXT_2_MGR_PRI == ext_sid)
    {
        memcpy(ctx->mgr[2].mgrAlm, ext_data, MGR_RALM_NUM);
    }
    else if(EXT_2_MGR_RSV == ext_sid)
    {
        memcpy(ctx->mgr[3].mgrAlm, ext_data, MGR_RALM_NUM);
    }
    else if(EXT_3_MGR_PRI == ext_sid)
    {
        memcpy(ctx->mgr[4].mgrAlm, ext_data, MGR_RALM_NUM);
    }
    else if(EXT_3_MGR_RSV == ext_sid)
    {
        memcpy(ctx->mgr[5].mgrAlm, ext_data, MGR_RALM_NUM);
    }
    else
    {
        return 0;
    }

    return 1;
}






