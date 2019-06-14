#include <stdio.h>
#include <string.h>

#include "ext_ctx.h"
#include "ext_trpt.h"
#include "ext_cmn.h"
#include "mcp_process.h"
//#include "memwatch.h"





/*
  1	成功
  0	失败
*/
u8_t ext_out_ssm_trpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx)
{
    int i, j;
    int slotid;
    u8_t sendbuf[128];

    //printf("--%s--\n", ext_data);

    if((EXT_1_MGR_PRI == ext_sid) || (EXT_1_MGR_RSV == ext_sid))
    {
        for(i = 0, j = 0; (i < 20) && (j < 10); i += 2, j++)
        {
            if( EXT_OUT16 == ctx->extBid[0][j] ||
                    EXT_OUT32 == ctx->extBid[0][j] ||
                    EXT_OUT16S == ctx->extBid[0][j] ||
                    EXT_OUT32S == ctx->extBid[0][j] )
            {
                if(' ' != ext_data[i] && ' ' != ext_data[i + 1])
                {
                    memcpy(ctx->out[j].outSsm, &ext_data[i], 2);
                    //memcpy(ctx->save.outSsm[j], &ext_data[i], 2);
                    if(0 != memcmp(ctx->out[j].outSsm, ctx->save.outSsm[j], 2))
                    {
                        //lev
                        if(2 == strlen(ctx->save.outSsm[j]))
                        {
                            memcpy(ctx->out[j].outSsm, ctx->save.outSsm[j], 2);
                            memset(sendbuf, 0, 128);
                            sprintf((char *)sendbuf, "SET-OUT-LEV:%c:%s::%s;",
                                    j + EXT_1_OUT_LOWER,
                                    "000000",
                                    ctx->save.outSsm[j]);
                            ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, j + EXT_1_OUT_LOWER, "000000");
                        }
                    }
                }
            }
        }
    }
    else if((EXT_2_MGR_PRI == ext_sid) || (EXT_2_MGR_RSV == ext_sid))
    {
        for(i = 0, j = 0; (i < 20) && (j < 10); i += 2, j++)
        {
            if( EXT_OUT16 == ctx->extBid[1][j] ||
                    EXT_OUT32 == ctx->extBid[1][j] ||
                    EXT_OUT16S == ctx->extBid[1][j] ||
                    EXT_OUT32S == ctx->extBid[1][j] )
            {
                if(' ' != ext_data[i] && ' ' != ext_data[i + 1])
                {
                    memcpy(ctx->out[10 + j].outSsm, &ext_data[i], 2);
                    //memcpy(ctx->save.outSsm[10+j], &ext_data[i], 2);
                    if(0 != memcmp(ctx->out[10 + j].outSsm, ctx->save.outSsm[10 + j], 2))
                    {
                        //lev
                        if(2 == strlen(ctx->save.outSsm[10 + j]))
                        {
                            memcpy(ctx->out[10 + j].outSsm, ctx->save.outSsm[10 + j], 2);
                            memset(sendbuf, 0, 128);
                            sprintf((char *)sendbuf, "SET-OUT-LEV:%c:%s::%s;",
                                    j + EXT_2_OUT_LOWER,
                                    "000000",
                                    ctx->save.outSsm[10 + j]);
                            ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, j + EXT_2_OUT_LOWER, "000000");
                        }
                    }
                }
            }
        }
    }
    else if((EXT_3_MGR_PRI == ext_sid) || (EXT_3_MGR_RSV == ext_sid))
    {
        for(i = 0, j = 0; (i < 20) && (j < 10); i += 2, j++)
        {
            if( EXT_OUT16 == ctx->extBid[2][j] ||
                    EXT_OUT32 == ctx->extBid[2][j] ||
                    EXT_OUT16S == ctx->extBid[2][j] ||
                    EXT_OUT32S == ctx->extBid[2][j] )
            {
                if(' ' != ext_data[i] && ' ' != ext_data[i + 1])
                {
                    memcpy(ctx->out[20 + j].outSsm, &ext_data[i], 2);
                    //memcpy(ctx->save.outSsm[20+j], &ext_data[i], 2);
                    if(0 != memcmp(ctx->out[20 + j].outSsm, ctx->save.outSsm[20 + j], 2))
                    {
                        if((j + EXT_3_OUT_LOWER) >= ':')//except ':;'
                        {
                            slotid = j + EXT_3_OUT_LOWER + 2;
                        }
                        else
                        {
                            slotid = j + EXT_3_OUT_LOWER;
                        }

                        //lev
                        if(2 == strlen(ctx->save.outSsm[20 + j]))
                        {
                            memcpy(ctx->out[20 + j].outSsm, ctx->save.outSsm[20 + j], 2);
                            memset(sendbuf, 0, 128);
                            sprintf((char *)sendbuf, "SET-OUT-LEV:%c:%s::%s;",
                                    slotid,
                                    "000000",
                                    ctx->save.outSsm[20 + j]);
                            ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, slotid, "000000");
                        }
                    }
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
u8_t ext_out_signal_trpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx)
{
    int i;
    u8_t bid;
    int slotid;
    u8_t sendbuf[128];

    /*
      对于OUT16，分为两组（1-8  9-16），
          bit0-bit3表示1-8路，bit4-bit7表示9-16路。
      对于OUT32，分为四组（1-4  5-8  9-12  13-16），
          bit0-bit1表示1-4路，bit2-bit3表示5-8路，bit4-bit5表示9-12路，bit6-bit7表示13-16路。
      0：2.048Mhz
      1：2.048Mbit/s
      2：无输出
    */
    if((EXT_1_MGR_PRI == ext_sid) || (EXT_1_MGR_RSV == ext_sid))
    {
        for(i = 0; i < 10; i++)
        {
            bid = ctx->extBid[0][i];
            if(EXT_OUT16 == bid || EXT_OUT16S == bid)
            {
                if(0 == (ext_data[i] & 0x0F))
                {
                    memcpy(ctx->out[i].outSignal, "00000000", 8);
                }
                else if(1 == (ext_data[i] & 0x0F))
                {
                    memcpy(ctx->out[i].outSignal, "11111111", 8);
                }
                else if(2 == (ext_data[i] & 0x0F))
                {
                    memcpy(ctx->out[i].outSignal, "22222222", 8);
                }
                else
                {
                    return 0;
                }

                if(0 == ((ext_data[i] >> 4) & 0x0F))
                {
                    memcpy(&(ctx->out[i].outSignal[8]), "00000000", 8);
                }
                else if(1 == ((ext_data[i] >> 4) & 0x0F))
                {
                    memcpy(&(ctx->out[i].outSignal[8]), "11111111", 8);
                }
                else if(2 == ((ext_data[i] >> 4) & 0x0F))
                {
                    memcpy(&(ctx->out[i].outSignal[8]), "22222222", 8);
                }
                else
                {
                    return 0;
                }

                //memcpy(ctx->save.outSignal[i], ctx->out[i].outSignal, 16);
                if(0 != memcmp(ctx->out[i].outSignal, ctx->save.outSignal[i], 16))
                {
                    //signal
                    if(16 == strlen(ctx->save.outSignal[i]))
                    {
                        memcpy(ctx->out[i].outSignal, ctx->save.outSignal[i], 16);
                        memset(sendbuf, 0, 128);
                        sprintf((char *)sendbuf, "SET-OUT-TYP:%c:%s::%s;",
                                i + EXT_1_OUT_LOWER,
                                "000000",
                                ctx->save.outSignal[i]);
                        ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, i + EXT_1_OUT_LOWER, "000000");
                    }
                }
            }
            else if(EXT_OUT32 == bid || EXT_OUT32S == bid)
            {
                if(0 == (ext_data[i] & 0x03))
                {
                    memcpy(ctx->out[i].outSignal, "0000", 4);
                }
                else if(1 == (ext_data[i] & 0x03))
                {
                    memcpy(ctx->out[i].outSignal, "1111", 4);
                }
                else if(2 == (ext_data[i] & 0x03))
                {
                    memcpy(ctx->out[i].outSignal, "2222", 4);
                }
                else
                {
                    return 0;
                }

                if(0 == ((ext_data[i] >> 2) & 0x03))
                {
                    memcpy(&(ctx->out[i].outSignal[4]), "0000", 4);
                }
                else if(1 == ((ext_data[i] >> 2) & 0x03))
                {
                    memcpy(&(ctx->out[i].outSignal[4]), "1111", 4);
                }
                else if(2 == ((ext_data[i] >> 2) & 0x03))
                {
                    memcpy(&(ctx->out[i].outSignal[4]), "2222", 4);
                }
                else
                {
                    return 0;
                }

                if(0 == ((ext_data[i] >> 4) & 0x03))
                {
                    memcpy(&(ctx->out[i].outSignal[8]), "0000", 4);
                }
                else if(1 == ((ext_data[i] >> 4) & 0x03))
                {
                    memcpy(&(ctx->out[i].outSignal[8]), "1111", 4);
                }
                else if(2 == ((ext_data[i] >> 4) & 0x03))
                {
                    memcpy(&(ctx->out[i].outSignal[8]), "2222", 4);
                }
                else
                {
                    return 0;
                }

                if(0 == ((ext_data[i] >> 6) & 0x03))
                {
                    memcpy(&(ctx->out[i].outSignal[12]), "0000", 4);
                }
                else if(1 == ((ext_data[i] >> 6) & 0x03))
                {
                    memcpy(&(ctx->out[i].outSignal[12]), "1111", 4);
                }
                else if(2 == ((ext_data[i] >> 6) & 0x03))
                {
                    memcpy(&(ctx->out[i].outSignal[12]), "2222", 4);
                }
                else
                {
                    return 0;
                }

                //memcpy(ctx->save.outSignal[i], ctx->out[i].outSignal, 16);
                if(0 != memcmp(ctx->out[i].outSignal, ctx->save.outSignal[i], 16))
                {
                    //signal
                    if(16 == strlen(ctx->save.outSignal[i]))
                    {
                        memcpy(ctx->out[i].outSignal, ctx->save.outSignal[i], 16);
                        memset(sendbuf, 0, 128);
                        sprintf((char *)sendbuf, "SET-OUT-TYP:%c:%s::%s;",
                                i + EXT_1_OUT_LOWER,
                                "000000",
                                ctx->save.outSignal[i]);
                        ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, i + EXT_1_OUT_LOWER, "000000");
                    }
                }
            }
            else
            {
                //
            }
        }
    }
    else if((EXT_2_MGR_PRI == ext_sid) || (EXT_2_MGR_RSV == ext_sid))
    {
        for(i = 0; i < 10; i++)
        {
            bid = ctx->extBid[1][i];
            if(EXT_OUT16 == bid || EXT_OUT16S == bid)
            {
                if(0 == (ext_data[i] & 0x0F))
                {
                    memcpy(ctx->out[10 + i].outSignal, "00000000", 8);
                }
                else if(1 == (ext_data[i] & 0x0F))
                {
                    memcpy(ctx->out[10 + i].outSignal, "11111111", 8);
                }
                else if(2 == (ext_data[i] & 0x0F))
                {
                    memcpy(ctx->out[10 + i].outSignal, "22222222", 8);
                }
                else
                {
                    return 0;
                }

                if(0 == ((ext_data[i] >> 4) & 0x0F))
                {
                    memcpy(&(ctx->out[10 + i].outSignal[8]), "00000000", 8);
                }
                else if(1 == ((ext_data[i] >> 4) & 0x0F))
                {
                    memcpy(&(ctx->out[10 + i].outSignal[8]), "11111111", 8);
                }
                else if(2 == ((ext_data[i] >> 4) & 0x0F))
                {
                    memcpy(&(ctx->out[10 + i].outSignal[8]), "22222222", 8);
                }
                else
                {
                    return 0;
                }

                //memcpy(ctx->save.outSignal[10+i], ctx->out[10+i].outSignal, 16);
                if(0 != memcmp(ctx->out[10 + i].outSignal, ctx->save.outSignal[10 + i], 16))
                {
                    //signal
                    if(16 == strlen(ctx->save.outSignal[10 + i]))
                    {
                        memcpy(ctx->out[10 + i].outSignal, ctx->save.outSignal[10 + i], 16);
                        memset(sendbuf, 0, 128);
                        sprintf((char *)sendbuf, "SET-OUT-TYP:%c:%s::%s;",
                                i + EXT_2_OUT_LOWER,
                                "000000",
                                ctx->save.outSignal[10 + i]);
                        ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, i + EXT_2_OUT_LOWER, "000000");
                    }
                }
            }
            else if(EXT_OUT32 == bid || EXT_OUT32S == bid)
            {
                if(0 == (ext_data[i] & 0x03))
                {
                    memcpy(ctx->out[10 + i].outSignal, "0000", 4);
                }
                else if(1 == (ext_data[i] & 0x03))
                {
                    memcpy(ctx->out[10 + i].outSignal, "1111", 4);
                }
                else if(2 == (ext_data[i] & 0x03))
                {
                    memcpy(ctx->out[10 + i].outSignal, "2222", 4);
                }
                else
                {
                    return 0;
                }

                if(0 == ((ext_data[i] >> 2) & 0x03))
                {
                    memcpy(&(ctx->out[10 + i].outSignal[4]), "0000", 4);
                }
                else if(1 == ((ext_data[i] >> 2) & 0x03))
                {
                    memcpy(&(ctx->out[10 + i].outSignal[4]), "1111", 4);
                }
                else if(2 == ((ext_data[i] >> 2) & 0x03))
                {
                    memcpy(&(ctx->out[10 + i].outSignal[4]), "2222", 4);
                }
                else
                {
                    return 0;
                }

                if(0 == ((ext_data[i] >> 4) & 0x03))
                {
                    memcpy(&(ctx->out[10 + i].outSignal[8]), "0000", 4);
                }
                else if(1 == ((ext_data[i] >> 4) & 0x03))
                {
                    memcpy(&(ctx->out[10 + i].outSignal[8]), "1111", 4);
                }
                else if(2 == ((ext_data[i] >> 4) & 0x03))
                {
                    memcpy(&(ctx->out[10 + i].outSignal[8]), "2222", 4);
                }
                else
                {
                    return 0;
                }

                if(0 == ((ext_data[i] >> 6) & 0x03))
                {
                    memcpy(&(ctx->out[10 + i].outSignal[12]), "0000", 4);
                }
                else if(1 == ((ext_data[i] >> 6) & 0x03))
                {
                    memcpy(&(ctx->out[10 + i].outSignal[12]), "1111", 4);
                }
                else if(2 == ((ext_data[i] >> 6) & 0x03))
                {
                    memcpy(&(ctx->out[10 + i].outSignal[12]), "2222", 4);
                }
                else
                {
                    return 0;
                }

                //memcpy(ctx->save.outSignal[10+i], ctx->out[10+i].outSignal, 16);
                if(0 != memcmp(ctx->out[10 + i].outSignal, ctx->save.outSignal[10 + i], 16))
                {
                    //signal
                    printf("2+%d+%s\n", i + 1, ctx->save.outSignal[10 + i]);
                    if(16 == strlen(ctx->save.outSignal[10 + i]))
                    {
                        memcpy(ctx->out[10 + i].outSignal, ctx->save.outSignal[10 + i], 16);
                        memset(sendbuf, 0, 128);
                        sprintf((char *)sendbuf, "SET-OUT-TYP:%c:%s::%s;",
                                i + EXT_2_OUT_LOWER,
                                "000000",
                                ctx->save.outSignal[10 + i]);
                        ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, i + EXT_2_OUT_LOWER, "000000");
                    }
                }
            }
            else
            {
                //
            }
        }
    }
    else if((EXT_3_MGR_PRI == ext_sid) || (EXT_3_MGR_RSV == ext_sid))
    {
        for(i = 0; i < 10; i++)
        {
            bid = ctx->extBid[2][i];
            if(EXT_OUT16 == bid || EXT_OUT16S == bid)
            {
                if(0 == (ext_data[i] & 0x0F))
                {
                    memcpy(ctx->out[20 + i].outSignal, "00000000", 8);
                }
                else if(1 == (ext_data[i] & 0x0F))
                {
                    memcpy(ctx->out[20 + i].outSignal, "11111111", 8);
                }
                else if(2 == (ext_data[i] & 0x0F))
                {
                    memcpy(ctx->out[20 + i].outSignal, "22222222", 8);
                }
                else
                {
                    return 0;
                }

                if(0 == ((ext_data[i] >> 4) & 0x0F))
                {
                    memcpy(&(ctx->out[20 + i].outSignal[8]), "00000000", 8);
                }
                else if(1 == ((ext_data[i] >> 4) & 0x0F))
                {
                    memcpy(&(ctx->out[20 + i].outSignal[8]), "11111111", 8);
                }
                else if(2 == ((ext_data[i] >> 4) & 0x0F))
                {
                    memcpy(&(ctx->out[20 + i].outSignal[8]), "22222222", 8);
                }
                else
                {
                    return 0;
                }

                //memcpy(ctx->save.outSignal[20+i], ctx->out[20+i].outSignal, 16);
                if(0 != memcmp(ctx->out[20 + i].outSignal, ctx->save.outSignal[20 + i], 16))
                {
                    if((i + EXT_3_OUT_LOWER) >= ':')//except ':;'
                    {
                        slotid = i + EXT_3_OUT_LOWER + 2;
                    }
                    else
                    {
                        slotid = i + EXT_3_OUT_LOWER;
                    }

                    //signal
                    if(16 == strlen(ctx->save.outSignal[20 + i]))
                    {
                        memcpy(ctx->out[20 + i].outSignal, ctx->save.outSignal[20 + i], 16);
                        printf("3+%d+%s\n", i + 1, ctx->save.outSignal[20 + i]);
                        memset(sendbuf, 0, 128);
                        sprintf((char *)sendbuf, "SET-OUT-TYP:%c:%s::%s;",
                                slotid,
                                "000000",
                                ctx->save.outSignal[20 + i]);
                        ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, slotid, "000000");
                    }
                }
            }
            else if(EXT_OUT32 == bid || EXT_OUT32S == bid)
            {
                if(0 == (ext_data[i] & 0x03))
                {
                    memcpy(ctx->out[20 + i].outSignal, "0000", 4);
                }
                else if(1 == (ext_data[i] & 0x03))
                {
                    memcpy(ctx->out[20 + i].outSignal, "1111", 4);
                }
                else if(2 == (ext_data[i] & 0x03))
                {
                    memcpy(ctx->out[20 + i].outSignal, "2222", 4);
                }
                else
                {
                    return 0;
                }

                if(0 == ((ext_data[i] >> 2) & 0x03))
                {
                    memcpy(&(ctx->out[20 + i].outSignal[4]), "0000", 4);
                }
                else if(1 == ((ext_data[i] >> 2) & 0x03))
                {
                    memcpy(&(ctx->out[20 + i].outSignal[4]), "1111", 4);
                }
                else if(2 == ((ext_data[i] >> 2) & 0x03))
                {
                    memcpy(&(ctx->out[20 + i].outSignal[4]), "2222", 4);
                }
                else
                {
                    return 0;
                }

                if(0 == ((ext_data[i] >> 4) & 0x03))
                {
                    memcpy(&(ctx->out[20 + i].outSignal[8]), "0000", 4);
                }
                else if(1 == ((ext_data[i] >> 4) & 0x03))
                {
                    memcpy(&(ctx->out[20 + i].outSignal[8]), "1111", 4);
                }
                else if(2 == ((ext_data[i] >> 4) & 0x03))
                {
                    memcpy(&(ctx->out[20 + i].outSignal[8]), "2222", 4);
                }
                else
                {
                    return 0;
                }

                if(0 == ((ext_data[i] >> 6) & 0x03))
                {
                    memcpy(&(ctx->out[20 + i].outSignal[12]), "0000", 4);
                }
                else if(1 == ((ext_data[i] >> 6) & 0x03))
                {
                    memcpy(&(ctx->out[20 + i].outSignal[12]), "1111", 4);
                }
                else if(2 == ((ext_data[i] >> 6) & 0x03))
                {
                    memcpy(&(ctx->out[20 + i].outSignal[12]), "2222", 4);
                }
                else
                {
                    return 0;
                }

                //memcpy(ctx->save.outSignal[20+i], ctx->out[20+i].outSignal, 16);
                if(0 != memcmp(ctx->out[20 + i].outSignal, ctx->save.outSignal[20 + i], 16))
                {
                    if((i + EXT_3_OUT_LOWER) >= ':')//except ':;'
                    {
                        slotid = i + EXT_3_OUT_LOWER + 2;
                    }
                    else
                    {
                        slotid = i + EXT_3_OUT_LOWER;
                    }

                    //signal
                    if(16 == strlen(ctx->save.outSignal[20 + i]))
                    {
                        memcpy(ctx->out[20 + i].outSignal, ctx->save.outSignal[20 + i], 16);
                        printf("3+%d+%s\n", i + 1, ctx->save.outSignal[20 + i]);
                        memset(sendbuf, 0, 128);
                        sprintf((char *)sendbuf, "SET-OUT-TYP:%c:%s::%s;",
                                slotid,
                                "000000",
                                ctx->save.outSignal[20 + i]);
                        ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, slotid, "000000");
                    }
                }
            }
            else
            {
                //
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
u8_t ext_out_mode_trpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx)
{
    int i, j;
    int slotid;
    u8_t sendbuf[128];

    if((EXT_1_MGR_PRI == ext_sid) || (EXT_1_MGR_RSV == ext_sid))
    {
        for(i = 0, j = 0; i < 20 && j < 10; i += 2, j++)
        {
            if( EXT_OUT16 == ctx->extBid[0][j] ||
                    EXT_OUT32 == ctx->extBid[0][j] ||
                    EXT_OUT16S == ctx->extBid[0][j] ||
                    EXT_OUT32S == ctx->extBid[0][j] )
            {
                if(' ' != ext_data[i] && ' ' != ext_data[i + 1])
                {
                    ctx->out[j].outPR[0] = ext_data[i];
                    ctx->out[j].outPR[1] = ',';
                    ctx->out[j].outPR[2] = ext_data[i + 1];
                }

                /*ctx->save.outPR[j][0] = ext_data[i];
                ctx->save.outPR[j][1] = ',';
                ctx->save.outPR[j][2] = ext_data[i+1];*/
                if(0 != memcmp(ctx->out[j].outPR, ctx->save.outPR[j], 3))
                {
                    //mod
                    printf("1+%d+%s\n", j + 1, ctx->save.outPR[j]);
                    if('0' == ctx->save.outPR[j][0] || '1' == ctx->save.outPR[j][0])
                    {
                        memcpy(ctx->out[j].outPR, ctx->save.outPR[j], 1);

                        memset(sendbuf, 0, 128);
                        sprintf((char *)sendbuf, "SET-OUT-MOD:%c:%s::%c;",
                                j + EXT_1_OUT_LOWER,
                                "000000",
                                ctx->save.outPR[j][0]);
                        ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, j + EXT_1_OUT_LOWER, "000000");
                    }
                }
            }
        }
    }
    else if((EXT_2_MGR_PRI == ext_sid) || (EXT_2_MGR_RSV == ext_sid))
    {
        for(i = 0, j = 0; i < 20 && j < 10; i += 2, j++)
        {
            if( EXT_OUT16 == ctx->extBid[1][j] ||
                    EXT_OUT32 == ctx->extBid[1][j] ||
                    EXT_OUT16S == ctx->extBid[1][j] ||
                    EXT_OUT32S == ctx->extBid[1][j] )
            {
                if(' ' != ext_data[i] && ' ' != ext_data[i + 1])
                {
                    ctx->out[10 + j].outPR[0] = ext_data[i];
                    ctx->out[10 + j].outPR[1] = ',';
                    ctx->out[10 + j].outPR[2] = ext_data[i + 1];
                }

                /*ctx->save.outPR[10+j][0] = ext_data[i];
                ctx->save.outPR[10+j][1] = ',';
                ctx->save.outPR[10+j][2] = ext_data[i+1];*/
                if(0 != memcmp(ctx->out[10 + j].outPR, ctx->save.outPR[10 + j], 3))
                {
                    //mod
                    printf("2+%d+%s\n", j + 1, ctx->save.outPR[10 + j]);
                    if('0' == ctx->save.outPR[10 + j][0] || '1' == ctx->save.outPR[10 + j][0])
                    {
                        memcpy(ctx->out[10 + j].outPR, ctx->save.outPR[10 + j], 1);
                        memset(sendbuf, 0, 128);
                        sprintf((char *)sendbuf, "SET-OUT-MOD:%c:%s::%c;",
                                j + EXT_2_OUT_LOWER,
                                "000000",
                                ctx->save.outPR[10 + j][0]);
                        ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, j + EXT_2_OUT_LOWER, "000000");
                    }
                }
            }
        }
    }
    else if((EXT_3_MGR_PRI == ext_sid) || (EXT_3_MGR_RSV == ext_sid))
    {
        for(i = 0, j = 0; i < 20 && j < 10; i += 2, j++)
        {
            if( EXT_OUT16 == ctx->extBid[2][j] ||
                    EXT_OUT32 == ctx->extBid[2][j] ||
                    EXT_OUT16S == ctx->extBid[2][j] ||
                    EXT_OUT32S == ctx->extBid[2][j] )
            {
                if(' ' != ext_data[i] && ' ' != ext_data[i + 1])
                {
                    ctx->out[20 + j].outPR[0] = ext_data[i];
                    ctx->out[20 + j].outPR[1] = ',';
                    ctx->out[20 + j].outPR[2] = ext_data[i + 1];
                }

                /*ctx->save.outPR[20+j][0] = ext_data[i];
                ctx->save.outPR[20+j][1] = ',';
                ctx->save.outPR[20+j][2] = ext_data[i+1];*/
                if(0 != memcmp(ctx->out[20 + j].outPR, ctx->save.outPR[20 + j], 3))
                {
                    if((j + EXT_3_OUT_LOWER) >= ':')//except ':;'
                    {
                        slotid = j + EXT_3_OUT_LOWER + 2;
                    }
                    else
                    {
                        slotid = j + EXT_3_OUT_LOWER;
                    }

                    //mod
                    printf("3+%d+%s\n", j + 1, ctx->save.outPR[20 + j]);
                    if('0' == ctx->save.outPR[20 + j][0] || '1' == ctx->save.outPR[20 + j][0])
                    {
                        memcpy(ctx->out[20 + j].outPR, ctx->save.outPR[20 + j], 1);
                        memset(sendbuf, 0, 128);
                        sprintf((char *)sendbuf, "SET-OUT-MOD:%c:%s::%c;",
                                slotid,
                                "000000",
                                ctx->save.outPR[20 + j][0]);
                        ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, slotid, "000000");
                    }
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
u8_t ext_out_bid_trpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx)
{
    if((EXT_1_MGR_PRI == ext_sid) || (EXT_1_MGR_RSV == ext_sid))
    {
        memcpy(ctx->extBid[0], ext_data, 14);
    }
    else if((EXT_2_MGR_PRI == ext_sid) || (EXT_2_MGR_RSV == ext_sid))
    {
        memcpy(ctx->extBid[1], ext_data, 14);
    }
    else if((EXT_3_MGR_PRI == ext_sid) || (EXT_3_MGR_RSV == ext_sid))
    {
        //printf("<ext_out_bid_trpt>:%s\n",ext_data);
        memcpy(ctx->extBid[2], ext_data, 14);
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
u8_t ext_mgr_pr_trpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx)
{
    if((EXT_1_MGR_PRI == ext_sid) || (EXT_1_MGR_RSV == ext_sid))
    {
        ctx->mgr[0].mgrPR = ext_data[0] - '0';
        ctx->mgr[1].mgrPR = ext_data[1] - '0';
    }
    else if((EXT_2_MGR_PRI == ext_sid) || (EXT_2_MGR_RSV == ext_sid))
    {
        ctx->mgr[2].mgrPR = ext_data[0] - '0';
        ctx->mgr[3].mgrPR = ext_data[1] - '0';
    }
    else if((EXT_3_MGR_PRI == ext_sid) || (EXT_3_MGR_RSV == ext_sid))
    {
        ctx->mgr[4].mgrPR = ext_data[0] - '0';
        ctx->mgr[5].mgrPR = ext_data[1] - '0';
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
u8_t ext_out_alm_trpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx)
{
    int i, j;
    u8_t tmp[7];

    if((EXT_1_MGR_PRI == ext_sid) || (EXT_1_MGR_RSV == ext_sid))
    {
        for(i = 0, j = 0; i < 30 && j < 10; i += 4, j++)
        {
            memset(tmp, 0, 7);
            tmp[0] = 0x40 | (ext_data[i + 0] & 0x0F);
            tmp[1] = 0x40 | ((ext_data[i + 1] & 0x03) << 2) | ((ext_data[i + 0] >> 4) & 0x03);
			
            tmp[2] = 0x40 | ((ext_data[i + 1] >> 2) & 0x0F);
			
            tmp[3] = 0x40 | (ext_data[i + 2] & 0x0F);
			
            tmp[4] = 0x40 | ((ext_data[i + 3] & 0x03) << 2) | ((ext_data[i + 2] >> 4) & 0x03);
			
            tmp[5] = 0x40 |((ext_data[i + 3] >> 2) & 0x0F);
            memcpy(ctx->out[j].outAlm, tmp, 6);
        }
    }
    else if((EXT_2_MGR_PRI == ext_sid) || (EXT_2_MGR_RSV == ext_sid))
    {
        for(i = 0, j = 0; i < 30 && j < 10; i += 4, j++)
        {
            memset(tmp, 0, 7);
            tmp[0] = 0x40 | (ext_data[i + 0] & 0x0F);
            tmp[1] = 0x40 | ((ext_data[i + 1] & 0x03) << 2) | ((ext_data[i + 0] >> 4) & 0x03);
			
            tmp[2] = 0x40 | ((ext_data[i + 1] >> 2) & 0x0F);
			
            tmp[3] = 0x40 | (ext_data[i + 2] & 0x0F);
			
            tmp[4] = 0x40 | ((ext_data[i + 3] & 0x03) << 2) | ((ext_data[i + 2] >> 4) & 0x03);
			
            tmp[5] = 0x40 |((ext_data[i + 3] >> 2) & 0x0F);
            memcpy(ctx->out[10 + j].outAlm, tmp, 6);
        }
    }
    else if((EXT_3_MGR_PRI == ext_sid) || (EXT_3_MGR_RSV == ext_sid))
    {
        for(i = 0, j = 0; i < 30 && j < 10; i += 4, j++)
        {
            memset(tmp, 0, 7);
            tmp[0] = 0x40 | (ext_data[i + 0] & 0x0F);
            tmp[1] = 0x40 | ((ext_data[i + 1] & 0x03) << 2) | ((ext_data[i + 0] >> 4) & 0x03);
            tmp[2] = 0x40 | ((ext_data[i + 1] >> 2) & 0x0F);
            tmp[3] = 0x40 | (ext_data[i + 2] & 0x0F);
            tmp[4] = 0x40 | ((ext_data[i + 3] & 0x03) << 2) | ((ext_data[i + 2] >> 4) & 0x03);
            tmp[5] = 0x40 |((ext_data[i + 3] >> 2) & 0x0F);
            memcpy(ctx->out[20 + j].outAlm, tmp, 6);
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
u8_t ext_out_ver_trpt(u8_t ext_sid, u8_t *ext_data, struct extctx *ctx)
{
    //drv
    if('m' == ext_sid)
    {
        memcpy(ctx->drv[0].drvVer, ext_data, 22);
        printf("%s\n", ctx->drv[0].drvVer);
    }
    else if('n' == ext_sid)
    {
        memcpy(ctx->drv[1].drvVer, ext_data, 22);
    }

    //mgr
    else if(EXT_1_MGR_PRI == ext_sid)
    {
        memcpy(ctx->mgr[0].mgrVer, ext_data, 22);
    }
    else if(EXT_1_MGR_RSV == ext_sid)
    {
        memcpy(ctx->mgr[1].mgrVer, ext_data, 22);
    }

    else if(EXT_2_MGR_PRI == ext_sid)
    {
        memcpy(ctx->mgr[2].mgrVer, ext_data, 22);
    }
    else if(EXT_2_MGR_RSV == ext_sid)
    {
        memcpy(ctx->mgr[3].mgrVer, ext_data, 22);
    }

    else if(EXT_3_MGR_PRI == ext_sid)
    {
        memcpy(ctx->mgr[4].mgrVer, ext_data, 22);
    }
    else if(EXT_3_MGR_RSV == ext_sid)
    {
        memcpy(ctx->mgr[5].mgrVer, ext_data, 22);
    }

    //out
    else if((ext_sid >= EXT_1_OUT_LOWER) && (ext_sid <= EXT_1_OUT_UPPER))
    {
        memcpy(ctx->out[ext_sid - EXT_1_OUT_OFFSET].outVer, ext_data, 22);
    }
    else if((ext_sid >= EXT_2_OUT_LOWER) && (ext_sid <= EXT_2_OUT_UPPER))
    {
        if(',' == ext_sid)
        {
            memcpy(ctx->out[ext_sid - EXT_2_OUT_OFFSET].outVer, ext_data + 1, 22);
        }
        else
        {
            memcpy(ctx->out[ext_sid - EXT_2_OUT_OFFSET].outVer, ext_data, 22);
        }
    }
    else if((ext_sid >= EXT_3_OUT_LOWER) && (ext_sid <= EXT_3_OUT_UPPER))
    {
        if(ext_sid < 58)//except ':;'
        {
            memcpy(ctx->out[ext_sid - EXT_3_OUT_OFFSET].outVer, ext_data, 22);
        }
        else
        {
            memcpy(ctx->out[ext_sid - EXT_3_OUT_OFFSET - 2].outVer, ext_data, 22);
        }
    }

    else
    {
        return 0;
    }

    return 1;
}








