#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ext_alloc.h"
#include "mcp_set_struct.h"
//#include "memwatch.h"

/*
  1	成功
  0	失败
*/
int initializeContext(struct extctx *ctx)
{
    int i, ret;

    memset(ctx, 0, sizeof(struct extctx));

    //drv主备状态初始化
    for(i = 0; i < EXT_GRP_NUM; i++)
    {
        ctx->drv[0].drvPR[i] = 1;
    }

    for(i = 0; i < EXT_GRP_NUM; i++)
    {
        ctx->drv[1].drvPR[i] = 2;
    }

    //mgr主备状态初始化
    for(i = 0; i < EXT_MGR_NUM; i += 2)
    {
        ctx->mgr[i].mgrPR = 1;
        ctx->mgr[i + 1].mgrPR = 2;
    }

    for(i = 0; i < EXT_GRP_NUM; i++)
    {
        memcpy(ctx->extBid[i], "OOOOOOOOOOOOOO", 14);
    }

    for(i = 0; i < EXT_DRV_NUM; i++)
    {
        ctx->drv[i].drvAlm = '@';
    }

    for(i = 0; i < EXT_MGR_NUM; i++)
    {
        memcpy(ctx->mgr[i].mgrAlm, "@@@@@@@", 7);
    }

    for(i = 0; i < EXT_OUT_NUM; i++)
    {
        memcpy(ctx->out[i].outAlm, "@@@@@@", 6);
    }

    ret = ext_read_flash(&(ctx->save));

    printf("return=%d\n", ret);
    printf("p300-1 flashmark=%c\n", ctx->save.flashMark);


    if( (1 == ret) && ('F' == ctx->save.flashMark) )
    {
        printf("%s\n", "read config from flash");

        for(i = 0; i < EXT_OUT_NUM; i++)
        {
            memcpy(ctx->out[i].outSsm, ctx->save.outSsm[i], 2);
            memcpy(ctx->out[i].outSignal, ctx->save.outSignal[i], 16);
            memcpy(ctx->out[i].outPR, ctx->save.outPR[i], 3);
        }
    }
    else
    {
        for(i = 0; i < EXT_OUT_NUM; i++)
        {
            memcpy(ctx->out[i].outSsm, "04", 2);
            memcpy(ctx->out[i].outSignal, "1111111111111111", 16);
            memcpy(ctx->out[i].outPR, "0,0", 3);
        }

        memset(&ctx->save, 0, sizeof(struct extsave));
        for(i = 0; i < EXT_OUT_NUM; i++)
        {
            memcpy(ctx->save.outSsm[i], ctx->out[i].outSsm, 2);
            memcpy(ctx->save.outSignal[i], ctx->out[i].outSignal, 16);
            memcpy(ctx->save.outPR[i], ctx->out[i].outPR, 3);
        }

        printf("%s\n", "write config to flash");
        ctx->save.flashMark = 'F';
        if(0 == ext_write_flash(&(ctx->save)))
        {
            return 0;
        }
    }

    if( (ctx->save.onlineSta & 0xf0) != 0x40)
    {
        ctx->save.onlineSta = 0x4f;
    }

    printf("sizeof(struct extsave) = %d\n", (int)sizeof(struct extsave));
    printf("sizeof(struct extctx) = %d\n", (int)sizeof(struct extctx));

    return 1;
}







/*
  1	成功
  0	失败
*/
int ext_read_flash(struct extsave *psave)
{
    FILE *pf;
    u8_t cmd[128];

    memset(cmd, 0, 128);
    sprintf((char *)cmd, "dd if=/dev/rom1 of=%s bs=65536 count=1 skip=31", EXT_SAVE_PATH);
    if(-1 == system(cmd))
    {
        return 0;
    }
    sync();
    printf("%s SUCCESS\n", cmd);

    pf = fopen(EXT_SAVE_PATH, "rb+");
    if(NULL == pf)
    {
        return 0;
    }

    if(0 != fseek(pf, sizeof(file_content), SEEK_SET))
    {
        fclose(pf);
        return 0;
    }

    memset(psave, 0, sizeof(struct extsave));
    if(1 != fread(psave, sizeof(struct extsave), 1, pf))
    {
        fclose(pf);
        return 0;
    }

    fclose(pf);
    return 1;
}


/*
  1	成功
  0	失败
*/
int ext_write_flash(struct extsave *psave)
{
    FILE *pf;
    u8_t cmd[128];

    pf = fopen(EXT_SAVE_PATH, "rb+");
    if(NULL == pf)
    {
        return 0;
    }

    if(0 != fseek(pf, sizeof(file_content), SEEK_SET))
    {
        fclose(pf);
        return 0;
    }

    if(1 != fwrite(psave, sizeof(struct extsave), 1, pf))
    {
        fclose(pf);
        return 0;
    }

    fclose(pf);
	sync();
    memset(cmd, 0, 128);
    sprintf((char *)cmd, "fw -ul -f %s -o 0x%x /dev/rom1", EXT_SAVE_PATH, EXT_SAVE_ADDR);
    if(-1 == system(cmd))
    {
        return 0;
    }
    sync();
    printf("%s SUCCESS\n", cmd);
    return 1;
}







void ext_drv2_mgr6_pr(struct extctx *ctx, u8_t bid13, u8_t bid14)
{
    //drv13
    if(EXT_DRV != bid13)
    {
        ctx->drv[0].drvPR[0] = 4;
        ctx->drv[0].drvPR[1] = 4;
        ctx->drv[0].drvPR[2] = 4;
    }
    else
    {
        //连接状态
        if( (ctx->drv[0].drvAlm)&BIT(0) )
        {
            ctx->drv[0].drvPR[0] = 4;
        }
        else
        {
            //通信告警
            if(1 == ctx->extMcpDrvAlm[0])
            {
                ctx->drv[0].drvPR[0] = 3;
            }
            else
            {
                ctx->drv[0].drvPR[0] = 1;
            }
        }

        //连接状态
        if( (ctx->drv[0].drvAlm)&BIT(1) )
        {
            ctx->drv[0].drvPR[1] = 4;
        }
        else
        {
            //通信告警
            if(1 == ctx->extMcpDrvAlm[0])
            {
                ctx->drv[0].drvPR[1] = 3;
            }
            else
            {
                ctx->drv[0].drvPR[1] = 1;
            }
        }

        //连接状态
        if( (ctx->drv[0].drvAlm)&BIT(2) )
        {
            ctx->drv[0].drvPR[2] = 4;
        }
        else
        {
            //通信告警
            if(1 == ctx->extMcpDrvAlm[0])
            {
                ctx->drv[0].drvPR[2] = 3;
            }
            else
            {
                ctx->drv[0].drvPR[2] = 1;
            }
        }
    }

    //drv14
    if(EXT_DRV != bid14)
    {
        ctx->drv[1].drvPR[0] = 4;
        ctx->drv[1].drvPR[1] = 4;
        ctx->drv[1].drvPR[2] = 4;
    }
    else
    {
        //连接状态
        if( (ctx->drv[1].drvAlm)&BIT(0) )
        {
            ctx->drv[1].drvPR[0] = 4;
        }
        else
        {
            //通信告警
            if(1 == ctx->extMcpDrvAlm[1])
            {
                ctx->drv[1].drvPR[0] = 3;
            }
            else
            {
                if(1 != ctx->drv[0].drvPR[0])
                {
                    ctx->drv[1].drvPR[0] = 1;
                }
                else
                {
                    ctx->drv[1].drvPR[0] = 2;
                }
            }
        }

        //连接状态
        if( (ctx->drv[1].drvAlm)&BIT(1) )
        {
            ctx->drv[1].drvPR[1] = 4;
        }
        else
        {
            //通信告警
            if(1 == ctx->extMcpDrvAlm[1])
            {
                ctx->drv[1].drvPR[1] = 3;
            }
            else
            {
                if(1 != ctx->drv[0].drvPR[1])
                {
                    ctx->drv[1].drvPR[1] = 1;
                }
                else
                {
                    ctx->drv[1].drvPR[1] = 2;
                }
            }
        }

        //连接状态
        if( (ctx->drv[1].drvAlm)&BIT(2) )
        {
            ctx->drv[1].drvPR[2] = 4;
        }
        else
        {
            //通信告警
            if(1 == ctx->extMcpDrvAlm[1])
            {
                ctx->drv[1].drvPR[2] = 3;
            }
            else
            {
                if(1 != ctx->drv[0].drvPR[2])
                {
                    ctx->drv[1].drvPR[2] = 1;
                }
                else
                {
                    ctx->drv[1].drvPR[2] = 2;
                }
            }
        }
    }

    if((4 == ctx->drv[0].drvPR[0]) && (4 == ctx->drv[1].drvPR[0]))
    {
        memcpy(ctx->extBid[0], "OOOOOOOOOOOOOO", 14);
    }
    if((4 == ctx->drv[0].drvPR[1]) && (4 == ctx->drv[1].drvPR[1]))
    {
        memcpy(ctx->extBid[1], "OOOOOOOOOOOOOO", 14);
    }
    if((4 == ctx->drv[0].drvPR[2]) && (4 == ctx->drv[1].drvPR[2]))
    {
        memcpy(ctx->extBid[2], "OOOOOOOOOOOOOO", 14);
    }
}







