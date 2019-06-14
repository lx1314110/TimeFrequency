#include "ext_crpt.h"
#include "ext_global.h"
#include "ext_ctx.h"
#include "mcp_main.h"
#include "mcp_process.h"

#include "ext_alloc.h"
#include "ext_cmn.h"
#include "mcp_def.h"
#include "mcp_set_struct.h"
#include "mcp_save_struct.h"
//#include "memwatch.h"




//#include <sys/ipc.h>
//#include <sys/msg.h>

/* fpga device file descriptor */
static int fpga_fd = -1;


/**
 * @brief use linux3.0.x
 * @details [long description]
 *
 */
typedef struct jiffy_counts_t
{
    unsigned long long usr, nic, sys, idle;
    unsigned long long iowait, irq, softirq, steal;
    unsigned long long total;
    unsigned long long busy;
} jiffy_counts_t;

#define CPU_INFO_PATH 	"/proc/stat"
#define LINE_BUF_NO 	500

/**
 * @brief 	cpu_utilization
 * @details [long description]
 * @return 	CPU utilization percentage example:return 1 -> 1%
 */
int cpu_utilization(void)
{
    static const char fmt[] = "cpu %llu %llu %llu %llu %llu %llu %llu %llu";
    char line_buf[LINE_BUF_NO];
    jiffy_counts_t jiffy;
    jiffy_counts_t jiffyold;

    int cpu_util;
    int ret;
    unsigned long long total_old;
    unsigned long long total;
    unsigned long long busy_old;
    unsigned long long busy;

    FILE *fp = fopen(CPU_INFO_PATH, "r");

    if (!fgets(line_buf, LINE_BUF_NO, fp))
    {
        printf("cpu utilization fgets err \n");
    }
    ret = sscanf(line_buf, fmt,
                 &jiffyold.usr, &jiffyold.nic, &jiffyold.sys, &jiffyold.idle,
                 &jiffyold.iowait, &jiffyold.irq, &jiffyold.softirq,
                 &jiffyold.steal);
    busy_old = jiffyold.usr + jiffyold.nic + jiffyold.sys;
    total_old = jiffyold.usr + jiffyold.nic + jiffyold.sys + jiffyold.idle;

    fclose(fp);

    sleep(1);

    fp = fopen(CPU_INFO_PATH, "r");

    if (!fgets(line_buf, LINE_BUF_NO, fp))
    {
        printf("cpu utilization fgets err \n");
    }
    ret = sscanf(line_buf, fmt,
                 &jiffy.usr, &jiffy.nic, &jiffy.sys, &jiffy.idle,
                 &jiffy.iowait, &jiffy.irq, &jiffy.softirq,
                 &jiffy.steal);


    busy = jiffy.usr + jiffy.nic + jiffy.sys;
    total = jiffy.usr + jiffy.nic + jiffy.sys + jiffy.idle;
    cpu_util = (busy - busy_old) * 100.0 / (total - total_old);
#if CPU_UTILIZATION_DEBUG
    printf("busy is %llu  busy_old is %llu total is %llu  total_old is %llu\n", busy, busy_old, total, total_old);
    printf("usr is %llu usr_old is %llu \n", jiffyold.usr, jiffy.usr);
    printf("cpu is %d \n", cpu_util);
#endif
    fclose(fp);

    return cpu_util;
}

int Open_FpgaDev(void)
{
    if ( ( fpga_fd = open( FPGA_DEV, O_RDWR) ) < 0 )
    {
        return 0;
    }
    else
    {
        return 1;
    }
}


/* FPGA 读写函数, 读写FPGA 某一地址的值 */
int FpgaRead( const long paddr, unsigned short *DstAddr)
{
    long addr;
    int retval;

    addr = paddr << 16;

    retval = ioctl( fpga_fd, FPGA_READ, &addr );
    if ( 0 == retval )
    {
        *DstAddr = (unsigned short)( addr & 0xFFFF );
        return 1;
    }
    else
    {
        return 0;
    }
}


int FpgaWrite( const long paddr, unsigned short value)
{
    long addr;
    int retval;

    addr = (paddr << 16) + value;

    retval = ioctl( fpga_fd, FPGA_WRITE, &addr );
    if ( 0 == retval )
    {
        return 0;
    }
    else
    {
        return 1;
    }
}


void Close_FpgaDev(void)
{
    close(fpga_fd);
}




int cmd_set_tid(_MSG_NODE *msg, int flag)
{
    int len = 0;
    len = strlen(msg->data);
    if(len != 16)
    {
        return 1;
    }
    if(strncasecmp(msg->data, "LT", 2) == 0)
    {
        memset(conf_content.slot_u.tid, '\0', 17);
        memcpy(conf_content.slot_u.tid, msg->data, 16);
        respond_success(current_client, msg->ctag);

        save_config();
    }
    else
    {
        return 1;
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:set_tid:%s\n", NET_INFO, msg->data);
        printf("now,tid:%s\n", conf_content.slot_u.tid);
    }
#endif
    return 0;
}
int cmd_hand_shake(_MSG_NODE *msg, int flag )
{
    shake_hand_count++;
    //printf("<cmd_hand_shake>\n");
    respond_success(current_client, msg->ctag);

    timeout_cnt[client_num]++;
    if(print_switch == 0)
    {
        printf("<cmd_hand_shake>client<%d>++\n", client_num);
    }

    return 0;
}
int cmd_set_main(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    //int len_send;
    memset(sendbuf, '\0', SENDBUFSIZE);
    if(msg->solt == MCP_SLOT)
    {
        memset(conf_content.slot_u.msmode, '\0', 5);
        memcpy(conf_content.slot_u.msmode, msg->data, 4);
        save_config();
    }
    else if(msg->solt == RB1_SLOT)
    {
        //if(strcmp(rpt_content.slot_o.rb_msmode,msg->data)!=0)
        {
            memset(conf_content.slot_o.msmode, '\0', 5);
            memcpy(conf_content.slot_o.msmode, msg->data, 4);
            /*下发设置*/
            /* SET-MAIN:<ctag>::<msmode>;*/
            sprintf((char *)sendbuf, "SET-MAIN:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else if(msg->solt == RB2_SLOT)
    {
        //if(strcmp(rpt_content.slot_p.rb_msmode,msg->data)!=0)
        {
            memset(conf_content.slot_p.msmode, '\0', 5);
            memcpy(conf_content.slot_p.msmode, msg->data, 4);

            sprintf((char *)sendbuf, "SET-MAIN:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:set_main:%s\n", NET_INFO, msg->data);
        printf("now,msmode:%s\n", conf_content.slot_u.msmode);
    }
#endif
    return 0;
}
int cmd_set_dat(_MSG_NODE *msg, int flag)
{
    /*计算时间差值*/
    unsigned char year_s[5], year_rb[5];
    unsigned char month_s[3], month_rb[3];
    unsigned char day_s[3], day_rb[3];
    unsigned char hour_s[3], hour_rb[3];
    unsigned char minute_s[3], minute_rb[3];
    unsigned char second_s[3], second_rb[3];
    int len;
    int year, month, day, hour, minute, second;
    int year_r, month_r, day_r, hour_r, minute_r, second_r;
    if(conf_content.slot_u.time_source == '1')
    {
        len = strlen(msg->data);
        //printf("TEST<1>!\n");
        if(len == 21)
            /*	YYYY-MM-DD HH:mm:SS:f*/
        {
            memcpy(year_s, msg->data, 4);
            memcpy(month_s, &msg->data[5], 2);
            memcpy(day_s, &msg->data[8], 2);
            memcpy(hour_s, &msg->data[11], 2);
            memcpy(minute_s, &msg->data[14], 2);
            memcpy(second_s, &msg->data[17], 2);

            year = atoi(year_s);
            month = atoi(month_s);
            day = atoi(day_s);
            hour = atoi(hour_s);
            minute = atoi(minute_s);
            second = atoi(second_s);

            if(((hour < 0) || (hour > 23)) || ((minute < 0) || (minute > 60)) || ((second < 0) || (second > 60)))
            {
                return 1;
            }

            memcpy(year_rb, mcp_date, 4);
            memcpy(month_rb, &mcp_date[5], 2);
            memcpy(day_rb, &mcp_date[8], 2);
            memcpy(hour_rb, mcp_time, 2);
            memcpy(minute_rb, &mcp_time[3], 2);
            memcpy(second_rb, &mcp_time[6], 2);

            year_r = atoi(year_rb);
            month_r = atoi(month_rb);
            day_r = atoi(day_rb);
            hour_r = atoi(hour_rb);
            minute_r = atoi(minute_rb);
            second_r = atoi(second_rb);
            //	printf("TEST<2>!\n");
            if((year == year_r) && (month == month_r) && (day == day_r))
            {
                time_temp = (hour - hour_r) * 3600;
                time_temp += (minute - minute_r) * 60;
                time_temp += (second - second_r);
                printf("time_temp=%d\n", time_temp);
                //
            }
        }
        else
        {
            //	printf("TEST<3>!\n");
            return 1;
        }
    }
    else
    {
        //	respond_fail(current_client,msg->ctag,0);
        return 1;
    }
    //	printf("TEST<4>\n");
    respond_success(current_client, msg->ctag);
    //save_config();
    return 0;
}
int cmd_set_dat_s(_MSG_NODE *msg, int flag)
{
    if((msg->data[0] != '1') && (msg->data[0] != '0'))
    {
        //printf("error dat\n");
        return 1;
    }
    conf_content.slot_u.time_source = msg->data[0];
    /*处理*/
    respond_success(current_client, msg->ctag);
    save_config();
    return 0;
}


//设置输出信号SSM 打开,闭塞
int cmd_set_out_ssm_en(_MSG_NODE *msg, int flag)
{
    int ret = 1;

    printf("SSM enable = %c\n", msg->data[0]);
    if(MCP_SLOT == msg->solt)
    {
        if(('0' == msg->data[0]) || ('1' == msg->data[0]))
        {
            conf_content.slot_u.out_ssm_en = msg->data[0];
            respond_success(current_client, msg->ctag);
            save_config();
            ret = 0;
        }
    }

    return ret;
}

//
//设置输出信号SSM 门限
//
int cmd_set_out_ssm_oth(_MSG_NODE *msg, int flag)
{
    int ret = 1;
    unsigned char sendbuf[SENDBUFSIZE];
    unsigned char temp = msg->data[0];

    printf("SSM threshold = %c\n", msg->data[0]);
    if(MCP_SLOT == msg->solt)
    {
        if((msg->data[0] >= 'a') && (msg->data[0] <= 'f'))
        {
            switch(msg->data[0])
            {
            case 'a':
                conf_content.slot_u.out_ssm_oth = 0x02;
                break;
            case 'b':
                conf_content.slot_u.out_ssm_oth = 0x04;
                break;
            case 'c':
                conf_content.slot_u.out_ssm_oth = 0x08;
                break;
            case 'd':
                conf_content.slot_u.out_ssm_oth = 0x0b;
                break;
            case 'e':
                conf_content.slot_u.out_ssm_oth = 0x0f;
                break;
            case 'f':
                conf_content.slot_u.out_ssm_oth = 0x00;
                break;
            }
            respond_success(current_client, msg->ctag);
            if('S' == slot_type[0])
            {
                memset(sendbuf, 0, sizeof(sendbuf));
                sprintf((char *)sendbuf, "SET-SSM-OTH:%s::%c;", "000000", temp);
                sendtodown(sendbuf, 'a');
            }
            if('S' == slot_type[1])
            {
                memset(sendbuf, 0, sizeof(sendbuf));
                sprintf((char *)sendbuf, "SET-SSM-OTH:%s::%c;", "000000", temp);
                sendtodown(sendbuf, 'b');
            }
            save_config();
            ret = 0;
        }
    }

    return ret;
}


int cmd_set_net(_MSG_NODE *msg, int flag)
{
    int len, i, j, i1, i2, i3, i4, j1, j2, j3, len_tmp;
    int ret;
    unsigned char  tmp[50];
    memset(tmp, '\0', 50);
	//
    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
    j1 = 0;
    j2 = 0;
    j3 = 0;

    if(msg->solt == MCP_SLOT)
    {
        len = strlen(msg->data);
        for(i = 0; i < len; i++)
        {
            if(msg->data[i] == ',')
            {
                j++;
                if(j == 1)
                {
                    i1 = i + 1;
                }
                if(j == 2)
                {
                    i2 = i + 1;
                }
                if(j == 3)
                {
                    i3 = i + 1;
                }
                if(j == 4)
                {
                    i4 = i + 1;
                }
            }
        }
        if(i4 == 0)
        {
            return 1;
        }
        //printf("#%s#,%d,%d,%d,%d\n",msg->data,i1,i2,i3,i4);
        if(msg->data[0] != ',')
        {
            memset(tmp, '\0', 50);
            len_tmp = i1 - 1;
            memcpy(tmp, msg->data, len_tmp);

            if(if_a_string_is_a_valid_ipv4_address(tmp) == 0)
            {
                //if(strcmp(tmp,conf_content.slot_u.ip )!=0)
                {
                    ret = API_Set_McpIp(tmp);
                    if(ret == 0)
                    {
                        memset(conf_content.slot_u.ip, '\0', 16);
                        memcpy(conf_content.slot_u.ip, tmp, len_tmp);
                    }
                    else
                    {
                        return 1;
                    }
                }
            }
            else
            {
                return 1;
            }
        }
        if(msg->data[i2] != ',')
        {
            memset(tmp, '\0', 50);
            len_tmp = i3 - i2 - 1;
            memcpy(tmp, &msg->data[i2], len_tmp);

            if(if_a_string_is_a_valid_ipv4_address(tmp) == 0)
            {
                //if(strcmp(tmp,conf_content.slot_u.mask )!=0)
                {
                    ret = API_Set_McpMask(tmp);
                    if(ret == 0)
                    {
                        memset(conf_content.slot_u.mask, '\0', 16);
                        memcpy(conf_content.slot_u.mask, tmp, len_tmp);
                    }
                    else
                    {
                        return 1;
                    }
                }
            }
            else
            {
                return 1;
            }
        }
        if(msg->data[i1] != ',')
        {
            memset(tmp, '\0', 50);
            j = 0;
            len_tmp = i2 - i1 - 1;
            memcpy(tmp, &msg->data[i1], len_tmp);
            //printf("gggggggggate=%s\n",tmp);
            if(if_a_string_is_a_valid_ipv4_address(tmp) == 0)
            {

                for(i = 0; i < len_tmp; i++)
                {
                    if(tmp[i] == '.')
                    {
                        j++;
                        if(j == 1)
                        {
                            j1 = i;
                        }
                        if(j == 2)
                        {
                            j2 = i;
                        }
                        if(j == 3)
                        {
                            j3 = i;
                        }
                    }
                }


                ret = API_Set_McpGateWay(tmp);
                if(ret == 0)
                {
                    memset(conf_content.slot_u.gate, '\0', 16);
                    memcpy(conf_content.slot_u.gate, tmp, len_tmp);
                }
                else
                {
                    return 1;
                }


            }
            else
            {
                return 1;
            }
        }
        if(msg->data[i3] != ',')
        {
            memset(tmp, '\0', 50);
            len_tmp = i4 - i3 - 1;
            memcpy(tmp, &msg->data[i3], len_tmp);

            if(if_a_string_is_a_valid_ipv4_address(tmp) == 0)
            {
                memset(conf_content.slot_u.dns1, '\0', 16);
                memcpy(conf_content.slot_u.dns1, tmp, len_tmp);
            }
            else
            {
                return 1;
            }
        }
        if(msg->data[i4] != '\0')
        {
            memset(tmp, '\0', 50);
            len_tmp = len - i4;
            memcpy(tmp, &msg->data[i4], len_tmp);

            if(if_a_string_is_a_valid_ipv4_address(tmp) == 0)
            {
                memset(conf_content.slot_u.dns2, '\0', 16);
                memcpy(conf_content.slot_u.dns2, tmp, len_tmp);
            }
            else
            {
                return 1;
            }
        }
    }
    else
    {
        return 1;
    }

    respond_success(current_client, msg->ctag);

    save_config();
#if DEBUG_NET_INFO
    printf("%sip:%s\n", NET_INFO, conf_content.slot_u.ip);
    printf("%smask:%s\n", NET_INFO, conf_content.slot_u.mask);
    printf("%sgate:%s\n", NET_INFO, conf_content.slot_u.gate);
    printf("%sdns1:%s\n", NET_INFO, conf_content.slot_u.dns1);
    printf("%sdns2:%s\n", NET_INFO, conf_content.slot_u.dns2);
#endif

    return 0;
}
int cmd_set_mac(_MSG_NODE *msg, int flag)
{
    int ret;
    if(msg->solt == MCP_SLOT)
    {
        //	if(strcmp(msg->data,conf_content.slot_u.mac )!=0)
        {
            ret = API_Set_McpMacAddr(msg->data);
            if(ret == 0)
            {
                memset(conf_content.slot_u.mac, '\0', 16);
                memcpy(conf_content.slot_u.mac, msg->data, 17);
            }
            else
            {
                return 1;
            }

            respond_success(current_client, msg->ctag);
            save_config();
        }
    }
    else
    {
        return 1;
    }
    return 0;
}
int cmd_set_password(_MSG_NODE *msg, int flag)
{
    memset(password, '\0', 9);
    memcpy(password, msg->data, 8);

    respond_success(current_client, msg->ctag);
    save_config();
#if DEBUG_NET_INFO
    printf("%s:set_password:%s\n", NET_INFO, msg->data);
    printf("now,password:%s\n", password);
#endif
    return 0;
}
int cmd_set_mode(_MSG_NODE *msg, int flag)
{
    int len;
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    len = strlen(msg->data);
    if(msg->solt == GBTP1_SLOT)
    {
        //	if(strcmp(rpt_content.slot_q.gb_acmode,msg->data)!=0)
        //		{
        memset(conf_content.slot_q.mode, '\0', 4);
        memcpy(conf_content.slot_q.mode, msg->data, len);
        /*下发处理*/
        /*SET-MODE:<ctag>::<acmode>;*/
        sprintf((char *)sendbuf, "SET-MODE:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
        //		}
    }
    else if(msg->solt == GBTP2_SLOT)
    {
        //	if(strcmp(rpt_content.slot_r.gb_acmode,msg->data)!=0)
        //		{
        memset(conf_content.slot_r.mode, '\0', 4);
        memcpy(conf_content.slot_r.mode, msg->data, len);

        sprintf((char *)sendbuf, "SET-MODE:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
        //		}
    }
    else
    {
        return 1;
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
int cmd_set_pos(_MSG_NODE *msg, int flag)
{
    int len;
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    len = strlen(msg->data);
    if(msg->solt == GBTP1_SLOT)
    {
        //if(strcmp(rpt_content.slot_q.pos,msg->data)!=0)
        //	{
        memset(conf_content.slot_q.pos, '\0', 35);
        memcpy(conf_content.slot_q.pos, msg->data, len);
        /*下发*//* SET-POS:<ctag>::<postion>;*/
        sprintf((char *)sendbuf, "SET-POS:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
        //}
    }
    else if(msg->solt == GBTP2_SLOT)
    {
        //if(strcmp(conf_content.slot_r.pos,msg->data)!=0)
        //{
        memset(conf_content.slot_r.pos, '\0', 35);
        memcpy(conf_content.slot_r.pos, msg->data, len);

        sprintf((char *)sendbuf, "SET-POS:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
        //}
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
int cmd_mask_gbtp(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    if((msg->data[0] != '0') && (msg->data[0] != '1'))
    {
        return 1;
    }
    if((msg->data[1] != '0') && (msg->data[1] != '1'))
    {
        return 1;
    }
    if(msg->solt == GBTP1_SLOT)
    {
        //	if(strcmp(rpt_content.slot_q.gb_mask,msg->data)!=0)
        {
            memset(conf_content.slot_q.mask, '\0', 3);
            memcpy(conf_content.slot_q.mask, msg->data, 2);
            /*下发处理*//* SET-MASK:<ctag>::<mask>;*/
            sprintf((char *)sendbuf, "SET-MASK:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else if(msg->solt == GBTP2_SLOT)
    {
        //	if(strcmp(rpt_content.slot_r.gb_mask,msg->data)!=0)
        {
            memset(conf_content.slot_r.mask, '\0', 3);
            memcpy(conf_content.slot_r.mask, msg->data, 2);

            sprintf((char *)sendbuf, "SET-MASK:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}

int cmd_bdzb(_MSG_NODE *msg, int flag)
{
    int len = 0;
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    if((strncmp1(msg->data, "BJ54", 4) != 0) && (strncmp1(msg->data, "CGS2000", 7) != 0))
    {
        return 1;
    }
    len = strlen(msg->data);
    if(msg->solt == GBTP1_SLOT)
    {
        //	if(strcmp(rpt_content.slot_q.gb_mask,msg->data)!=0)
        {
            memset(conf_content.slot_q.bdzb, '\0', 8);
            memcpy(conf_content.slot_q.bdzb, msg->data, len);
            sprintf((char *)sendbuf, "SET-BDZB:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else if(msg->solt == GBTP2_SLOT)
    {
        //	if(strcmp(rpt_content.slot_r.gb_mask,msg->data)!=0)
        {
            memset(conf_content.slot_r.bdzb, '\0', 8);
            memcpy(conf_content.slot_r.bdzb, msg->data, len);

            sprintf((char *)sendbuf, "SET-BDZB:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}


int cmd_sys_ref(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
	
	//
	//!Set sys ref: '0' Auto Select Sys Ref; '1' --- '8' Manual Select Sys Ref; 'F' -----NO Sys Ref;  
	//
    if((*msg->data < 0x30) || ((*msg->data > 0x38) && (*msg->data != 0x46)))
    {
        return 1;
    }
    if(msg->solt == RB1_SLOT)
    {
        //
        //!Main Slot Set Sys Ref.
        //
        conf_content.slot_o.sys_ref = *msg->data;
        //
        //!Send down Command。
        //
        sprintf((char *)sendbuf, "SET-SYS-REF:%s::INP-%c;", msg->ctag, *msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
    
    }
    else if(msg->solt == RB2_SLOT)
    {
        //
        //!Slave Slot Set Sys Ref。
        //
        conf_content.slot_p.sys_ref = *msg->data;
        //
        //!Send down Command。
        //
        sprintf((char *)sendbuf, "SET-SYS-REF:%s::INP-%c;", msg->ctag, *msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
    }
    else
    {
        return 1;
    }
	printf("%s:%s\n", "Set System Ref", sendbuf);
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
int cmd_set_dely(_MSG_NODE *msg, int flag)
{
    int len, num;
    int temp;
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    len = strlen(msg->data);
    if((msg->data[0] < 0x31) || (msg->data[0] > 0x38))
    {
        return 1;
    }
    temp = atoi(&msg->data[2]);
    if((temp < 0) || (temp > 99999999))
    {
        return 1;
    }
    if(msg->solt == RB1_SLOT)
    {
        num = (int)(msg->data[0] - 0x31);
        if((num >= 0x00) && (num < 4))
        {
            //if(strcmp(conf_content.slot_o.dely[num], &msg->data[2])!=0)
            //	{
            memset(conf_content.slot_o.dely[num], '\0', 9);
            memcpy(conf_content.slot_o.dely[num], &msg->data[2], len - 2);

            sprintf((char *)sendbuf, "SET-DELY:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
            //	}
        }
    }
    else if(msg->solt == RB2_SLOT)
    {
        num = (int)(msg->data[0] - 0x31);
        if((num >= 0x00) && (num < 4))
        {
            //if(strcmp(rpt_content.slot_p., &msg->data[2])!=0)
            //	{
            memset(conf_content.slot_p.dely[num], '\0', 9);
            memcpy(conf_content.slot_p.dely[num], &msg->data[2], len - 2);

            sprintf((char *)sendbuf, "SET-DELY:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
            //	}
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
int cmd_set_prio_inp(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    int i;
    memset(sendbuf, '\0', SENDBUFSIZE);
    for(i = 0; i < 8; i++)
    {
        if((msg->data[i] > '0') && (msg->data[i] < '9'))
        {
        }
        else
        {
            return 1;
        }
    }
    if(msg->solt == RB1_SLOT)
    {
        //	if(strcmp(rpt_content.slot_o.rb_prio, msg->data)!=0)
        {
            memset(conf_content.slot_o.priority, '\0', 9);
            memcpy(conf_content.slot_o.priority, msg->data, 8);
            /*下发*//*SET-PRIO-INP:<ctag>::<priority>;*/
            sprintf((char *)sendbuf, "SET-PRIO-INP:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else if(msg->solt == RB2_SLOT)
    {
        //	if(strcmp(rpt_content.slot_p.rb_prio, msg->data)!=0)
        {
            memset(conf_content.slot_p.priority, '\0', 9);
            memcpy(conf_content.slot_p.priority, msg->data, 8);
            /*下发*/
            sprintf((char *)sendbuf, "SET-PRIO-INP:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
int cmd_set_mask_e1(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    int i;
    memset(sendbuf, '\0', SENDBUFSIZE);
    for(i = 0; i < 8; i++)
    {
        if((msg->data[i] == '0') || (msg->data[i] == '1'))
        {}
        else
        {
            return 1;
        }
    }
    if(msg->solt == RB1_SLOT)
    {
        //	if(strcmp(rpt_content.slot_o.rb_mask, msg->data)!=0)
        {
            memset(conf_content.slot_o.mask, '\0', 9);
            memcpy(conf_content.slot_o.mask, msg->data, 8);
            /*下发*//*SET-MASK-INP:<ctag>::<mask>;*/
            sprintf((char *)sendbuf, "SET-MASK-INP:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else if(msg->solt == RB2_SLOT)
    {
        //	if(strcmp(rpt_content.slot_p.rb_mask, msg->data)!=0)
        {
            memset(conf_content.slot_p.mask, '\0', 9);
            memcpy(conf_content.slot_p.mask, msg->data, 8);
            /*下发*/
            sprintf((char *)sendbuf, "SET-MASK-INP:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
int cmd_set_leap_mask(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    int i;
    memset(sendbuf, '\0', SENDBUFSIZE);
    for(i = 0; i < 5; i++)
    {
        if((msg->data[i] == '0') || (msg->data[i] == '1'))
        {}
        else
        {
            return 1;
        }
    }
    if(msg->solt == RB1_SLOT)
    {
        //	if(strcmp(rpt_content.slot_o.rb_leapmask, msg->data)!=0)
        {
            memset(conf_content.slot_o.leapmask, '\0', 6);
            memcpy(conf_content.slot_o.leapmask, msg->data, 5);
            /*下发*//*SET-LMAK-INP:<ctag>::<leapmask>;*/
            sprintf((char *)sendbuf, "SET-LMAK-INP:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else if(msg->solt == RB2_SLOT)
    {
        //	if(strcmp(rpt_content.slot_p.rb_leapmask, msg->data)!=0)
        {
            memset(conf_content.slot_p.leapmask, '\0', 6);
            memcpy(conf_content.slot_p.leapmask, msg->data, 5);
            /*下发*/
            sprintf((char *)sendbuf, "SET-LMAK-INP:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
int cmd_set_tzo(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    if((msg->data[0] != '0') && (msg->data[0] != '1'))
    {
        return 1;
    }
    if((msg->data[2] < 0x40) || (msg->data[2] > 0x4c))
    {
        return 1;
    }
    if(msg->solt == RB1_SLOT)
    {
        //	if(strcmp(rpt_content.slot_o.rf_tzo,msg->data)!=0)
        {
            memset(conf_content.slot_o.tzo, '\0', 4);
            memcpy(conf_content.slot_o.tzo, msg->data, 3);
            sprintf((char *)sendbuf, "SET-TZO:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }

    }
    else if(msg->solt == RB2_SLOT)
    {
        //	if(strcmp(rpt_content.slot_p.rf_tzo,msg->data)!=0)
        {
            memset(conf_content.slot_p.tzo, '\0', 4);
            memcpy(conf_content.slot_p.tzo, msg->data, 3);
            sprintf((char *)sendbuf, "SET-TZO:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
int cmd_set_ptp_net(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    int num;
    int port;
    int i, j, i1, i2, i3, i4, i5, i6;
    int len = strlen(msg->data);
    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
    i5 = 0;
    i6 = 0;
    memset(sendbuf, '\0', SENDBUFSIZE);
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        if((msg->data[2] > '0') && (msg->data[2] < '5'))
        {
            //	memset(conf_content.slot[num].tp16_en,'\0',17);
            num = (int)(msg->solt - 0x61);
            port = (int)(msg->data[2] - 0x31);
            //	if(strcmp(conf_content.slot[num].ptp_net[port],msg->data)!=0)
            //		{
            //memset(conf_content.slot[num].ptp_net[port],'\0',99);
            //memcpy(conf_content.slot[num].ptp_net[port],msg->data,len);
            /*SET-PTP-NET:<type>,<response message>;*/

            for(i = 0; i < len; i++)
            {
                if(msg->data[i] == ',')
                {
                    j++;
                    if(j == 2)
                    {
                        i1 = i + 1;
                    }
                    if(j == 3)
                    {
                        i2 = i + 1;
                    }
                    if(j == 4)
                    {
                        i3 = i + 1;
                    }
                    if(j == 5)
                    {
                        i4 = i + 1;
                    }
                    if(j == 6)
                    {
                        i5 = i + 1;
                    }
                    if(j == 7)
                    {
                        i6 = i + 1;
                    }
                }
            }
            if(i6 == 0)
            {
                return 1;
            }
            conf_content.slot[num].ptp_type = msg->data[0];
            if(msg->data[i1] != ',')
            {
                memcpy(conf_content.slot[num].ptp_ip[port], &msg->data[i1], i2 - i1 - 1);
            }
            if(msg->data[i2] != ',')
            {
                memcpy(conf_content.slot[num].ptp_mac[port], &msg->data[i2], i3 - i2 - 1);
            }
            if(msg->data[i3] != ',')
            {
                memcpy(conf_content.slot[num].ptp_gate[port], &msg->data[i3], i4 - i3 - 1);
            }
            if(msg->data[i4] != ',')
            {
                memcpy(conf_content.slot[num].ptp_mask[port], &msg->data[i4], i5 - i4 - 1);
            }
            if(msg->data[i5] != ',')
            {
                memcpy(conf_content.slot[num].ptp_dns1[port], &msg->data[i5], i6 - i5 - 1);
            }
            if(msg->data[i6] != '\0')
            {
                memcpy(conf_content.slot[num].ptp_dns2[port], &msg->data[i6], len - i6);
            }

            sprintf((char *)sendbuf, "SET-PTP-NET:%s;", msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
            //		}
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
int cmd_set_ptp_mod(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    int num;
    int port;
    int i, j, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13;
    int len = strlen(msg->data);
    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
    i5 = 0;
    i6 = 0;
    i7 = 0;
    i8 = 0;
    i9 = 0;
    i10 = 0;
    i11 = 0;
    i12 = 0;
    i13 = 0;
    memset(sendbuf, '\0', SENDBUFSIZE);
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        if((msg->data[2] > '0') && (msg->data[2] < '5'))
        {
            //memset(conf_content.slot[num].tp16_en,'\0',17);
            num = (int)(msg->solt - 0x61);
            port = (int)(msg->data[2] - 0x31);
            //if(strcmp(conf_content.slot[num].ptp_mod[port],msg->data)!=0)
            //	{
            //		memset(conf_content.slot[num].ptp_mod[port],'\0',34);
            //		memcpy(conf_content.slot[num].ptp_mod[port],msg->data,len);
            /*SET-PTP-MOD:<type>,<response message>;*/
            for(i = 0; i < len; i++)
            {
                if(msg->data[i] == ',')
                {
                    j++;
                    if(j == 2)
                    {
                        i1 = i + 1;
                    }
                    if(j == 3)
                    {
                        i2 = i + 1;
                    }
                    if(j == 4)
                    {
                        i3 = i + 1;
                    }
                    if(j == 5)
                    {
                        i4 = i + 1;
                    }
                    if(j == 6)
                    {
                        i5 = i + 1;
                    }
                    if(j == 7)
                    {
                        i6 = i + 1;
                    }
                    if(j == 8)
                    {
                        i7 = i + 1;
                    }
                    if(j == 9)
                    {
                        i8 = i + 1;
                    }
                    if(j == 10)
                    {
                        i9 = i + 1;
                    }
                    if(j == 11)
                    {
                        i10 = i + 1;
                    }
                    if(j == 12)
                    {
                        i11 = i + 1;
                    }
                    if(j == 13)
                    {
                        i12 = i + 1;
                    }
                    if(j == 14)
                    {
                        i13 = i + 1;    //2014.12.2
                    }
                }
            }
            if(i12 == 0)
            {
                return 1;
            }
            conf_content.slot[num].ptp_type = msg->data[0];
            if((msg->data[i1] == '0') || (msg->data[i1] == '1'))
            {
                conf_content.slot[num].ptp_en[port] = msg->data[i1];
            }
            if((msg->data[i2] == '0') || (msg->data[i2] == '1'))
            {
                conf_content.slot[num].ptp_delaytype[port] = msg->data[i2];
            }
            if((msg->data[i3] == '0') || (msg->data[i3] == '1'))
            {
                //printf("ptp set %c\n",msg->data[i3]);
                conf_content.slot[num].ptp_multicast[port] = msg->data[i3];
            }
            if((msg->data[i4] == '0') || (msg->data[i4] == '1'))
            {
                conf_content.slot[num].ptp_enp[port] = msg->data[i4];
            }
            if((msg->data[i5] == '0') || (msg->data[i5] == '1'))
            {
                conf_content.slot[num].ptp_step[port] = msg->data[i5];
            }
            if(msg->data[i6] != ',')
            {
                memcpy(conf_content.slot[num].ptp_sync[port], &msg->data[i6], i7 - i6 - 1);
            }
            if(msg->data[i7] != ',')
            {
                memcpy(conf_content.slot[num].ptp_announce[port], &msg->data[i7], i8 - i7 - 1);
            }
            if(msg->data[i8] != ',')
            {
                memcpy(conf_content.slot[num].ptp_delayreq[port], &msg->data[i8], i9 - i8 - 1);
            }
            if(msg->data[i9] != ',')
            {
                memcpy(conf_content.slot[num].ptp_pdelayreq[port], &msg->data[i9], i10 - i9 - 1);
            }
            if(msg->data[i10] != ',')
            {
                memset(conf_content.slot[num].ptp_delaycom[port],'\0',9);//ADD PTP4
                memcpy(conf_content.slot[num].ptp_delaycom[port], &msg->data[i10], i11 - i10 - 1);
            }
            if((msg->data[i11] == '0') || (msg->data[i11] == '1'))
            {
                conf_content.slot[num].ptp_linkmode[port] = msg->data[i11];
            }
            if((msg->data[i12] == '0') || (msg->data[i12] == '1'))
            {
                conf_content.slot[num].ptp_out[port] = msg->data[i12];
            }
            //2014.12.2
            if(i13 != 0 && msg->data[i13] != '\0')
            {
                if(msg->data[i13] == 'A')
                {
                    memset(conf_content3.ptp_prio1[num][port], 0, 5);
                    memcpy(conf_content3.ptp_prio1[num][port], (msg->data) + i13, len - i13);
                    //prio = 0;
                }
                else if(msg->data[i13] == 'B')
                {
                    memset(conf_content3.ptp_prio2[num][port], 0, 5);
                    memcpy(conf_content3.ptp_prio2[num][port], (msg->data) + i13, len - i13);
                }
                else
                {
                    return 1;
                }
            }

            //printf("\n\nLUOHAO~~~~~~~~~2,conf_content.slot[%d].ptp_multicast[%d] = %d\n\n",num,port,conf_content.slot[num].ptp_multicast[port]);

            sprintf((char *)sendbuf, "SET-PTP-MOD:%s;", msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
            //	}
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}


int cmd_set_ptp_mtc(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    int num,i;
    int port;
	int ip_len;
	int max_ip_len = sizeof( conf_content3.ptp_mtc1_ip[0][0]);
	
	ip_len = strlen(msg->data)-4;
	//
    //!if no ip set,return;one ip address 8 byte;
    //
	if(ip_len < 8 || ip_len > max_ip_len)
	return 1;

	//
	//!check ip array is valid.
	//
	for(i = 9; i < ip_len; )
	{
       if(msg->data[i+3] != '.')
	   	 return 1;
	   else
	   	 i += 9;
	}
	
    memset(sendbuf, '\0', SENDBUFSIZE);

    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        if((msg->data[2] > '0') && (msg->data[2] < '5'))
        {
            num = (int)(msg->solt - 0x61);
            port = (int)(msg->data[2] - 0x31);
            if(msg->data[4] != '\0')
            {
                memset(conf_content3.ptp_mtc1_ip[num][port], '\0', max_ip_len);
                memcpy(conf_content3.ptp_mtc1_ip[num][port], &msg->data[4], ip_len);
				sprintf((char *)sendbuf, "SET-PTP-MTC:%s;", msg->data);
				sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            return 0;

        }
		return 1;
    }
    else
    {
        return 1;
    }
}
int cmd_set_ptp_dom(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    int num;
    int port;
	int i;
	
    memset(sendbuf, '\0', SENDBUFSIZE);

    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        
        if((msg->data[2] > '0') && (msg->data[2] < '5'))
        {
            num = (int)(msg->solt - 0x61);
            port = (int)(msg->data[2] - 0x31);
            if(msg->data[4] != ';')
            {
                //printf("set ptp domA:%s \n",msg->data);
                for(i = 4; msg->data[i] != '\0' ; i++ )
                {
                    if(i >= 7)
						return 1;
                }
				//printf("set ptp domB:%s \n",msg->data);
                memset(conf_content3.ptp_dom[num][port], 0, 4);
                memcpy(conf_content3.ptp_dom[num][port], &msg->data[4], i - 4);
                sprintf((char *)sendbuf, "SET-PTP-DOM:%s;", msg->data);

                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            return 0;

        }
		return 1;
    }
    else
    {
        return 1;
    }
}

/*
int cmd_set_ptp_out(_MSG_NODE * msg,int flag)
{
	unsigned char sendbuf[SENDBUFSIZE];
	int num;
	int port;
	memset(sendbuf,'\0',SENDBUFSIZE);
	if(((msg->solt)>'`')&&((msg->solt)<'o'))
		{
			if((msg->data[2]>'0')&&(msg->data[2]<'5'))
				{
					num=(int)(msg->solt-0x61);
					port=(int)(msg->data[2]-0x31);

					if((msg->data[4]=='1' )||(msg->data[4]=='0'))
						{
							conf_content.slot[num].ptp_out[port]=msg->data[4];
						}
					else
						return 1;


					sprintf((char *)sendbuf,"SET-PTP-OUT:%s;",msg->data);
					if(flag)
						{
							sendtodown_cli(sendbuf,msg->solt,msg->ctag);
							save_config();
						}
					else
						sendtodown(sendbuf, msg->solt);

				}
			else
				return 1;
		}
	else
		return 1;

	return 0;
}
*/






int cmd_set_out_lev(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    unsigned char tmp_ctag[7];
    int num;

    memset(sendbuf, '\0', SENDBUFSIZE);
    memset(tmp_ctag, 0, 7);
    memcpy(tmp_ctag, "000000", 6);
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        num = (int)(msg->solt - 0x61);
        memset(conf_content.slot[num].out_lev, '\0', 3);
        memcpy(conf_content.slot[num].out_lev, msg->data, 2);
        //SET-OUT-LEV:<ctag>::<level>;
        sprintf((char *)sendbuf, "SET-OUT-LEV:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
    }
    //ext
    else if((msg->solt >= EXT_1_OUT_LOWER && msg->solt <= EXT_1_OUT_UPPER) ||
            (msg->solt >= EXT_2_OUT_LOWER && msg->solt <= EXT_2_OUT_UPPER) ||
            (msg->solt >= EXT_3_OUT_LOWER && msg->solt <= EXT_3_OUT_UPPER && msg->solt != ':' && msg->solt != ';') )
    {
        if(msg->solt >= EXT_1_OUT_LOWER && msg->solt <= EXT_1_OUT_UPPER)
        {
            memcpy(gExtCtx.out[msg->solt - EXT_1_OUT_OFFSET].outSsm, msg->data, 2);
            memcpy(gExtCtx.save.outSsm[msg->solt - EXT_1_OUT_OFFSET], msg->data, 2);
        }
        else if(msg->solt >= EXT_2_OUT_LOWER && msg->solt <= EXT_2_OUT_UPPER)
        {
            memcpy(gExtCtx.out[msg->solt - EXT_2_OUT_OFFSET].outSsm, msg->data, 2);
            memcpy(gExtCtx.save.outSsm[msg->solt - EXT_2_OUT_OFFSET], msg->data, 2);
        }
        else
        {
            if(msg->solt < ':')
            {
                memcpy(gExtCtx.out[msg->solt - EXT_3_OUT_OFFSET].outSsm, msg->data, 2);
                memcpy(gExtCtx.save.outSsm[msg->solt - EXT_3_OUT_OFFSET], msg->data, 2);
            }
            else
            {
                memcpy(gExtCtx.out[msg->solt - EXT_3_OUT_OFFSET - 2].outSsm, msg->data, 2);
                memcpy(gExtCtx.save.outSsm[msg->solt - EXT_3_OUT_OFFSET - 2], msg->data, 2);
            }
        }

        if(flag)
        {
            sprintf((char *)sendbuf, "SET-OUT-LEV:%c:%s::%s;", msg->solt, msg->ctag, msg->data);
            if(0 == ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, msg->solt, msg->ctag))
            {
                return 1;
            }
        }
        else
        {
            sprintf((char *)sendbuf, "SET-OUT-LEV:%c:%s::%s;", msg->solt, tmp_ctag, msg->data);
            if(0 == ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, msg->solt, tmp_ctag))
            {
                return 1;
            }
        }

        if(0 == ext_write_flash(&(gExtCtx.save)))
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif

    return 0;
}








int cmd_set_out_ppr(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    int num;
    unsigned char tmp_ctag[7];

    memset(sendbuf, '\0', SENDBUFSIZE);
    memset(tmp_ctag, 0, 7);
    memcpy(tmp_ctag, "000000", 6);
    if( ((msg->solt) > '`') && ((msg->solt) < 'o') )
    {
        num = (int)(msg->solt - 0x61);
        sprintf((char *)sendbuf, "SET-OUT-PRR:%s::;", msg->ctag);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
    }
    //ext
    else if((msg->solt >= EXT_1_OUT_LOWER && msg->solt <= EXT_1_OUT_UPPER) ||
            (msg->solt >= EXT_2_OUT_LOWER && msg->solt <= EXT_2_OUT_UPPER) ||
            (msg->solt >= EXT_3_OUT_LOWER && msg->solt <= EXT_3_OUT_UPPER && msg->solt != ':' && msg->solt != ';') )
    {
        if(flag)
        {
            sprintf((char *)sendbuf, "SET-OUT-PRR:%c:%s::;", msg->solt, msg->ctag);
            if(0 == ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, msg->solt, msg->ctag))
            {
                return 1;
            }
        }
        else
        {
            sprintf((char *)sendbuf, "SET-OUT-PRR:%c:%s::;", msg->solt, tmp_ctag);
            if(0 == ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, msg->solt, tmp_ctag))
            {
                return 1;
            }
        }
    }
    else
    {
        return 1;
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif

    return 0;
}



int cmd_set_out_typ(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    int num, i;
    unsigned char tmp_ctag[7];

    memset(tmp_ctag, 0, 7);
    memcpy(tmp_ctag, "000000", 6);
    memset(sendbuf, '\0', SENDBUFSIZE);
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        num = (int)(msg->solt - 0x61);
        for(i = 0; i < 16; i++)
        {
            if((msg->data[i] < '0') || (msg->data[i] > '2'))
            {
                return 1;
            }
        }

        memset(conf_content.slot[num].out_typ, '\0', 17);
        memcpy(conf_content.slot[num].out_typ, msg->data, 16);
        /*SET-OUT-TYP:<ctag>::<level>;*/
        sprintf((char *)sendbuf, "SET-OUT-TYP:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
    }
    else if((msg->solt >= EXT_1_OUT_LOWER && msg->solt <= EXT_1_OUT_UPPER) ||
            (msg->solt >= EXT_2_OUT_LOWER && msg->solt <= EXT_2_OUT_UPPER) ||
            (msg->solt >= EXT_3_OUT_LOWER && msg->solt <= EXT_3_OUT_UPPER && msg->solt != ':' && msg->solt != ';') )
    {
        for(i = 0; i < 16; i++)
        {
            if( (msg->data[i] < '0') || (msg->data[i] > '2') )
            {
                return 1;
            }
        }

        if(0 == ext_out_signal_crpt(1, msg->solt, msg->data, &gExtCtx))
        {
            return 1;
        }

        if(flag)
        {
            sprintf((char *)sendbuf, "SET-OUT-TYP:%c:%s::%s;", msg->solt, msg->ctag, msg->data);
            if(0 == ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, msg->solt, msg->ctag))
            {
                return 1;
            }
        }
        else
        {
            sprintf((char *)sendbuf, "SET-OUT-TYP:%c:%s::%s;", msg->solt, tmp_ctag, msg->data);
            if(0 == ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, msg->solt, tmp_ctag))
            {
                return 1;
            }
        }

        if(0 == ext_write_flash(&(gExtCtx.save)))
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif

    return 0;
}





int cmd_set_out_mod(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    int num;
    unsigned char tmp_ctag[7];

    memset(sendbuf, '\0', SENDBUFSIZE);
    memset(tmp_ctag, 0, 7);
    memcpy(tmp_ctag, "000000", 6);

    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        if((msg->data[0] == '1') || (msg->data[0] == '0'))
        {
            num = (msg->solt) - 'a';
            conf_content.slot[num].out_mod = msg->data[0];
            sprintf((char *)sendbuf, "SET-OUT-MOD:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
        else
        {
            return 1;
        }
    }
    //ext
    else if((msg->solt >= EXT_1_OUT_LOWER && msg->solt <= EXT_1_OUT_UPPER) ||
            (msg->solt >= EXT_2_OUT_LOWER && msg->solt <= EXT_2_OUT_UPPER) ||
            (msg->solt >= EXT_3_OUT_LOWER && msg->solt <= EXT_3_OUT_UPPER && msg->solt != ':' && msg->solt != ';') )
    {
        if(msg->solt >= EXT_1_OUT_LOWER && msg->solt <= EXT_1_OUT_UPPER)
        {
            gExtCtx.out[msg->solt - EXT_1_OUT_OFFSET].outPR[0] = msg->data[0];
            gExtCtx.save.outPR[msg->solt - EXT_1_OUT_OFFSET][0] = msg->data[0];
        }
        else if(msg->solt >= EXT_2_OUT_LOWER && msg->solt <= EXT_2_OUT_UPPER)
        {
            gExtCtx.out[msg->solt - EXT_2_OUT_OFFSET].outPR[0] = msg->data[0];
            gExtCtx.save.outPR[msg->solt - EXT_2_OUT_OFFSET][0] = msg->data[0];
        }
        else
        {
            if(msg->solt < ':')//except : ;
            {
                gExtCtx.out[msg->solt - EXT_3_OUT_OFFSET].outPR[0] = msg->data[0];
                gExtCtx.save.outPR[msg->solt - EXT_3_OUT_OFFSET][0] = msg->data[0];
            }
            else
            {
                gExtCtx.out[msg->solt - EXT_3_OUT_OFFSET - 2].outPR[0] = msg->data[0];
                gExtCtx.save.outPR[msg->solt - EXT_3_OUT_OFFSET - 2][0] = msg->data[0];
            }
        }

        if(flag)
        {
            sprintf((char *)sendbuf, "SET-OUT-MOD:%c:%s::%s;", msg->solt, msg->ctag, msg->data);
            if(0 == ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, msg->solt, msg->ctag))
            {
                return 1;
            }
        }
        else
        {
            sprintf((char *)sendbuf, "SET-OUT-MOD:%c:%s::%s;", msg->solt, tmp_ctag, msg->data);
            if(0 == ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, msg->solt, tmp_ctag))
            {
                return 1;
            }
        }

        if(0 == ext_write_flash(&(gExtCtx.save)))
        {
            return 1;
        }
    }
    else
    {
        return 1;
    }

    return 0;
}





int cmd_set_reset(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);

    if((msg->solt == ':') || (msg->solt == 'u'))
    {
        /****reboot*/
        memcpy(sendbuf, "RESET", 5);
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        respond_success(current_client, msg->ctag);

    }
    else if((msg->solt > '`') && (msg->solt < 's'))
    {
        sprintf((char *)sendbuf, "SET-RESET:%s::;", msg->ctag);
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
    }
    else if(msg->solt == 'v')
    {}
    else if((msg->solt == 's') || ((msg->solt == 't')))
    {}
    else
    {
        return 1;
    }
    /*SET-RESET:<ctag>::;*/
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
int cmd_set_leap_tag(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    if(msg->solt == RB1_SLOT)
    {
        //	if(rpt_content.slot_o.rb_leaptag!=*msg->data)
        {
            conf_content.slot_o.leaptag = *msg->data;
            /*SET-LEAP:<ctag>::<leaptag>;*/
            sprintf((char *)sendbuf, "SET-LEAP:%s::%c;", msg->ctag, *msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else if(msg->solt == RB2_SLOT)
    {
        //	if(rpt_content.slot_p.rb_leaptag!=*msg->data)
        {
            conf_content.slot_p.leaptag = *msg->data;

            sprintf((char *)sendbuf, "SET-LEAP:%s::%c;", msg->ctag, *msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
int cmd_set_tp16_en(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    int num = 0, i;
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);
    //	memset(tmp,'\0',sizeof(unsigned char) * 17);
    for(i = 0; i < 16; i++)
    {
        if((msg->data[i] != '0') && (msg->data[i] != '1'))
        {
            return 1;
        }
    }
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        //memset(&conf_content.slot[num],'\0',sizeof(out_content));
        num = (int)(msg->solt - 0x61);
        //	if(strcmp(rpt_content.slot[num].tp16_en, msg->data)!=0)
        //		{
        //memset(conf_content.slot[num].tp16_en, '\0', 17);
        //			for(i=0;i<16;i++)
        //				tmp[i]=msg->data[15-i];
        memcpy(conf_content.slot[num].tp16_en, msg->data, 16);
        /*SET-TP16-EN::<ctag>::<en>;*/
        sprintf((char *)sendbuf, "SET-TP16-EN:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
        //		}
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    //	free(tmp);
    return 0;
}
int cmd_set_leap_num(_MSG_NODE *msg, int flag)
{
    int num, i;
    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);

    num = atoi(msg->data);
    if(strncmp1(msg->data, "00", 2) == 0)
    {}
    else if((num < 1) || (num > 99))
    {
        return 1;
    }
    if((msg->solt == ':') || (msg->solt == 'u'))
    {
        memset(conf_content.slot_u.leap_num, '\0', 3);
        memcpy(conf_content.slot_u.leap_num, msg->data, 2);

        sprintf((char *)sendbuf, "SET-LEAP-NUM:%s::%s;", msg->ctag, msg->data);
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        save_config();

        for(i = 0; i < 14; i++)
        {
            if(slot_type[i] == 'I')
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-TP16-LEAP:%s::%s;", msg->ctag, msg->data);
                sendtodown(sendbuf, i + 0x61);
            }
            else if((slot_type[i] > '@') && (slot_type[i] < 'I'))
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-PTP-LEAP:%s::%s;", msg->ctag, msg->data);
                sendtodown(sendbuf, i + 0x61);
            }
			else if(slot_type[i] == 'j')
			{
			    memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-PTP-LEAP:%s::%s;", msg->ctag, msg->data);
                sendtodown(sendbuf, i + 0x61);
			}
            else if(slot_type[i] == 'T' || slot_type[i] == 'X')
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-RS-TZO:%s::,%s;", msg->ctag, msg->data);
                sendtodown(sendbuf, i + 0x61);
            }
            else if(slot_type[i] == 'V')
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-TOD-PRO:%s::,,%s;", msg->ctag, msg->data);
                sendtodown(sendbuf, i + 0x61);
            }
            else if(slot_type[i] == 'W')
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-IGB-LEAP:%s::%s;", msg->ctag, msg->data);
                sendtodown(sendbuf, i + 0x61);
            }
            else if(slot_type[i] == 'Z')
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-NTP-LEAP:%s::%s;", msg->ctag, msg->data);
                sendtodown(sendbuf, i + 0x61);
            }
        }
		//if the online pan type at the 15 slot, it need to send the leap value to the 15 slot.the 16 slot as the up.
		//
		if((slot_type[14] == 'R') | (slot_type[14] == 'K'))
		{
		    memset(sendbuf, '\0', SENDBUFSIZE);
            sprintf((char *)sendbuf, "SET-RB-LEAP:%s::%s;", msg->ctag, msg->data);
            sendtodown(sendbuf, 14 + 0x61);
		}
		if((slot_type[15] == 'R') | (slot_type[15] == 'K'))
		{
		    memset(sendbuf, '\0', SENDBUFSIZE);
            sprintf((char *)sendbuf, "SET-RB-LEAP:%s::%s;", msg->ctag, msg->data);
            sendtodown(sendbuf, 15 + 0x61);
		}
    }
    else
    {
        return 1;
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, conf_content.slot_u.leap_num);
    }
#endif
    return 0;
}



int cmd_rtrv_alrms(int client, _MSG_NODE *msg)
{
    int send_ok, num;
    unsigned char sendbuf[1024];

    memset(sendbuf, '\0', sizeof(unsigned char) * 1024);
    num = (int)(msg->solt - 'a');
    get_format(sendbuf, msg->ctag);
    get_alm(msg->solt, sendbuf);
    //printf("<%s>:%s\n",__FUNCTION__,sendbuf);
    send_ok = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(send_ok == -1 && errno == EAGAIN)
    {
        printf("timeout_<3>!\n");
    }

    return 0;
}




/*
  0	成功
  1	失败
*/
int cmd_ext_alm(int client, _MSG_NODE *msg)
{
    int ret;
    u8_t sendbuf[1024];

    memset(sendbuf, 0, 1024);
    get_format(sendbuf, msg->ctag);
    get_ext_alm(msg->solt, sendbuf);
    ret = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(ret == -1 && errno != EAGAIN)
    {
        return 1;
    }

    return 0;
}




/*
  0	成功
  1	失败
*/
int cmd_ext1_alm(int client, _MSG_NODE *msg)
{
    int ret;
    u8_t sendbuf[1024];

    memset(sendbuf, 0, 1024);
    get_format(sendbuf, msg->ctag);
    get_ext1_alm(msg->solt, sendbuf);
    ret = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(ret == -1 && errno != EAGAIN)
    {
        return 1;
    }

    return 0;
}





/*
  0	成功
  1	失败
*/
int cmd_ext2_alm(int client, _MSG_NODE *msg)
{
    int ret;
    u8_t sendbuf[1024];

    memset(sendbuf, 0, 1024);
    get_format(sendbuf, msg->ctag);
    get_ext2_alm(msg->solt, sendbuf);
    ret = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(ret == -1 && errno != EAGAIN)
    {
        return 1;
    }

    return 0;
}





/*
  0	成功
  1	失败
*/
int cmd_ext3_alm(int client, _MSG_NODE *msg)
{
    int ret;
    u8_t sendbuf[1024];

    memset(sendbuf, 0, 1024);
    get_format(sendbuf, msg->ctag);
    get_ext3_alm(msg->solt, sendbuf);
    ret = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(ret == -1 && errno != EAGAIN)
    {
        return 1;
    }

    return 0;
}







/*
  0	成功
  1	失败
*/
int cmd_ext1_prs(int client, _MSG_NODE *msg)
{
    u8_t sendbuf[1024];
    int i, ret;

    memset(sendbuf, 0, 1024);
    get_format(sendbuf, msg->ctag);

    sprintf((char *)sendbuf, "%s%c%c%c\"", sendbuf, SP, SP, SP);

    for(i = 0; i < 10; i++)
    {
        if( (EXT_OUT16 == gExtCtx.extBid[0][i]) ||
                (EXT_OUT32 == gExtCtx.extBid[0][i]) ||
                (EXT_OUT16S == gExtCtx.extBid[0][i]) ||
                (EXT_OUT32S == gExtCtx.extBid[0][i]) )
        {
            sprintf((char *)sendbuf, "%s%c,%s",
                    sendbuf,
                    gExtCtx.extBid[0][i],
                    ((0 == strlen(gExtCtx.out[i].outPR)) ? (char *)"," : (char *)(gExtCtx.out[i].outPR)));
        }
        else
        {
            sprintf((char *)sendbuf, "%s%c,%s",
                    sendbuf,
                    gExtCtx.extBid[0][i],
                    ",");
        }

        if(i < 9)
        {
            strcat(sendbuf, ":");
        }
    }

    strcat(sendbuf, "\"\r\n;");
    ret = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(ret == -1 && errno != EAGAIN)
    {
        return 1;
    }

    return 0;
}







/*
  0	成功
  1	失败
*/
int cmd_ext2_prs(int client, _MSG_NODE *msg)
{
    u8_t sendbuf[1024];
    int i, ret;

    memset(sendbuf, 0, 1024);
    get_format(sendbuf, msg->ctag);

    sprintf((char *)sendbuf, "%s%c%c%c\"", sendbuf, SP, SP, SP);

    for(i = 0; i < 10; i++)
    {
        if( (EXT_OUT16 == gExtCtx.extBid[1][i]) ||
                (EXT_OUT32 == gExtCtx.extBid[1][i]) ||
                (EXT_OUT16S == gExtCtx.extBid[1][i]) ||
                (EXT_OUT32S == gExtCtx.extBid[1][i]) )
        {
            sprintf((char *)sendbuf, "%s%c,%s",
                    sendbuf,
                    gExtCtx.extBid[1][i],
                    ((0 == strlen(gExtCtx.out[10 + i].outPR)) ? (char *)"," : (char *)(gExtCtx.out[10 + i].outPR)));
        }
        else
        {
            sprintf((char *)sendbuf, "%s%c,%s",
                    sendbuf,
                    gExtCtx.extBid[1][i],
                    ",");
        }

        if(i < 9)
        {
            strcat(sendbuf, ":");
        }
    }

    strcat(sendbuf, "\"\r\n;");
    ret = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(ret == -1 && errno != EAGAIN)
    {
        return 1;
    }

    return 0;
}







/*
  0	成功
  1	失败
*/
int cmd_ext3_prs(int client, _MSG_NODE *msg)
{
    u8_t sendbuf[1024];
    int i, ret;

    memset(sendbuf, 0, 1024);
    get_format(sendbuf, msg->ctag);

    sprintf((char *)sendbuf, "%s%c%c%c\"", sendbuf, SP, SP, SP);

    for(i = 0; i < 10; i++)
    {
        if( (EXT_OUT16 == gExtCtx.extBid[2][i]) ||
                (EXT_OUT32 == gExtCtx.extBid[2][i]) ||
                (EXT_OUT16S == gExtCtx.extBid[2][i]) ||
                (EXT_OUT32S == gExtCtx.extBid[2][i]) )
        {
            sprintf((char *)sendbuf, "%s%c,%s",
                    sendbuf,
                    gExtCtx.extBid[2][i],
                    ((0 == strlen(gExtCtx.out[20 + i].outPR)) ? (char *)"," : (char *)(gExtCtx.out[20 + i].outPR)));
        }
        else
        {
            sprintf((char *)sendbuf, "%s%c,%s",
                    sendbuf,
                    gExtCtx.extBid[2][i],
                    ",");
        }

        if(i < 9)
        {
            strcat(sendbuf, ":");
        }
    }

    strcat(sendbuf, "\"\r\n;");
    ret = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(ret == -1 && errno != EAGAIN)
    {
        return 1;
    }

    return 0;
}








/*
  0	成功
  1	失败
*/
int cmd_drv_mgr(int client, _MSG_NODE *msg)
{
    u8_t sendbuf[1024];
    int ret;
    unsigned char drvPr[2][3];
    unsigned char mgrPr[6];

    memset(drvPr, 0, sizeof(drvPr));
    memset(mgrPr, 0, sizeof(mgrPr));
    memset(sendbuf, 0, 1024);

    drvPr[0][0] = gExtCtx.drv[0].drvPR[0];
    drvPr[1][0] = gExtCtx.drv[1].drvPR[0];
    drvPr[0][1] = gExtCtx.drv[0].drvPR[1];
    drvPr[1][1] = gExtCtx.drv[1].drvPR[1];
    drvPr[0][2] = gExtCtx.drv[0].drvPR[2];
    drvPr[1][2] = gExtCtx.drv[1].drvPR[2];

    mgrPr[0] = gExtCtx.mgr[0].mgrPR;
    mgrPr[1] = gExtCtx.mgr[1].mgrPR;
    mgrPr[2] = gExtCtx.mgr[2].mgrPR;
    mgrPr[3] = gExtCtx.mgr[3].mgrPR;
    mgrPr[4] = gExtCtx.mgr[4].mgrPR;
    mgrPr[5] = gExtCtx.mgr[5].mgrPR;

    get_format(sendbuf, msg->ctag);

    if( (((alm_sta[12][5] & 0x01) == 0x01) && (slot_type[12] == 'd') )
            || ( ((alm_sta[13][5] & 0x01) == 0x01) && (slot_type[13] == 'd') )
            || ((gExtCtx.save.onlineSta & 0x01) == 0))
    {
        drvPr[0][0] = 4;
        drvPr[1][0] = 4;
        mgrPr[0] = 4;
        mgrPr[1] = 4;
    }


    if( (((alm_sta[12][5] & 0x02) == 0x02) && (slot_type[12] == 'd'))
            || ( ((alm_sta[13][5] & 0x02) == 0x02) && (slot_type[13] == 'd') )
            || ((gExtCtx.save.onlineSta & 0x02) == 0x00))
    {
        drvPr[0][1] = 4;
        drvPr[1][1] = 4;
        mgrPr[2] = 4;
        mgrPr[3] = 4;
    }


    if( (((alm_sta[12][5] & 0x04) == 0x04) && (slot_type[12] == 'd'))
            || ( ((alm_sta[13][5] & 0x04) == 0x04) && (slot_type[13] == 'd') )
            || ((gExtCtx.save.onlineSta & 0x04) == 0x00))
    {
        drvPr[0][2] = 4;
        drvPr[1][2] = 4;
        mgrPr[4] = 4;
        mgrPr[5] = 4;
    }

    sprintf((char *)sendbuf, "%s   \"%d%d%d%d%d%d,%d%d%d%d%d%d\"\r\n;",
            sendbuf,
            drvPr[0][0], drvPr[1][0],
            drvPr[0][1], drvPr[1][1],
            drvPr[0][2], drvPr[1][2],
            mgrPr[0], mgrPr[1],
            mgrPr[2], mgrPr[3],
            mgrPr[4], mgrPr[5] );


    /*
    	sprintf((char *)sendbuf, "%s   \"%d%d%d%d%d%d,%d%d%d%d%d%d\"\r\n;",
    					 sendbuf,
    					 gExtCtx.drv[0].drvPR[0], gExtCtx.drv[1].drvPR[0],
    					 gExtCtx.drv[0].drvPR[1], gExtCtx.drv[1].drvPR[1],
    					 gExtCtx.drv[0].drvPR[2], gExtCtx.drv[1].drvPR[2],
    					 gExtCtx.mgr[0].mgrPR, gExtCtx.mgr[1].mgrPR,
    					 gExtCtx.mgr[2].mgrPR, gExtCtx.mgr[3].mgrPR,
    					 gExtCtx.mgr[4].mgrPR, gExtCtx.mgr[5].mgrPR );
    */

    ret = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(ret == -1 && errno != EAGAIN)
    {
        return 1;
    }

    return 0;
}








int cmd_rtrv_out(int client , _MSG_NODE *msg  )
{
    unsigned char sendbuf[1024];
    int send_ok, i;
    unsigned char real_rpt;

    memset(sendbuf, '\0', 1024);
    get_format(sendbuf, msg->ctag);
    sprintf((char *)sendbuf, "%s%c%c%c\"", sendbuf, SP, SP, SP);
    for(i = 0; i < 14; i++)
    {
        if( (slot_type[i] == 'J') ||
                (slot_type[i] == 'a' ) ||
                (slot_type[i] == 'b') ||
                (slot_type[i] == 'c') ||
                (slot_type[i] == 'h') ||
                (slot_type[i] == 'i') )
        {
            if('0' == rpt_content.slot[i].out_mode)
            {
                real_rpt = '2';
            }
            else
            {
                real_rpt = rpt_content.slot[i].out_sta;
            }
            sprintf((char *)sendbuf, "%s%c%c", sendbuf, slot_type[i], real_rpt);
        }
        if(i < 13)
        {
            strncatnew(sendbuf, ",", 1);
        }
    }

    sprintf((char *)sendbuf, "%s\"%c%c;", sendbuf, CR, LF);
    send_ok = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(send_ok == -1 && errno == EAGAIN)
    {
        /*	close(client);
        FD_CLR(client, &allset);
        client= -1;*/
        printf("timeout_<4>!\n");
    }

    return 0;
	
}


int cmd_set_CONFIG(_MSG_NODE *msg, int flag)
{
    unsigned char buf[15] = "SET-CONFIG:::;";
    if(file_exists(CONFIG_FILE))
    {
        printf("CONFIG wait\n");
        sendtodown_cli(buf, msg->solt, msg->ctag);
    }
    else
    {
        return 1;
    }
    return 0;
}


int cmd_rtrv_eqpt(int client , _MSG_NODE *msg  )
{
    int send_ok;
    unsigned char sendbuf[1024];

    memset(sendbuf, '\0', sizeof(unsigned char) * 1024);
    get_format(sendbuf, msg->ctag);
    get_data(msg->solt, sendbuf);

    send_ok = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    //printf("\n\n%s SEND: %s\n\n", __func__, sendbuf);
    if(send_ok == -1 && errno == EAGAIN)
    {
        /*	close(client);
        	FD_CLR(client, &allset);
        	client= -1;*/
        printf("timeout_<4>!\n");
    }
    return 0;
}





/*
  0	成功
  1	失败
*/
int cmd_ext1_eqt(int client, _MSG_NODE *msg)
{
    int ret;
    u8_t sendbuf[1024];

    memset(sendbuf, 0, 1024);

    get_format(sendbuf, msg->ctag);
    get_ext1_eqt(msg->solt, sendbuf);

    ret = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(ret == -1 && errno != EAGAIN)
    {
        return 1;
    }

    return 0;
}





/*
  0	成功
  1	失败
*/
int cmd_ext2_eqt(int client, _MSG_NODE *msg)
{
    int ret;
    u8_t sendbuf[1024];

    memset(sendbuf, 0, 1024);

    get_format(sendbuf, msg->ctag);
    get_ext2_eqt(msg->solt, sendbuf);

    ret = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(ret == -1 && errno != EAGAIN)
    {
        return 1;
    }

    return 0;
}





/*
  0	成功
  1	失败
*/
int cmd_ext3_eqt(int client, _MSG_NODE *msg)
{
    int ret;
    u8_t sendbuf[1024];

    memset(sendbuf, 0, 1024);

    get_format(sendbuf, msg->ctag);
    get_ext3_eqt(msg->solt, sendbuf);

    ret = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(ret == -1 && errno != EAGAIN)
    {
        return 1;
    }

    return 0;
}

/*
  0	成功
  1	失败
*/
int cmd_ext_eqt(int client, _MSG_NODE *msg)
{
    int ret;
    u8_t sendbuf[1024];

    memset(sendbuf, 0, 1024);

    get_format(sendbuf, msg->ctag);
	get_exta1_eqt(msg->solt, sendbuf);
	get_exta2_eqt(msg->solt, sendbuf);
    get_exta3_eqt(msg->solt, sendbuf);

    ret = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(ret == -1 && errno != EAGAIN)
    {
        return 1;
    }

    return 0;
}



int cmd_rtrv_pm_inp(int client, _MSG_NODE *msg )
{
    //unsigned char  *sendbuf=(unsigned char *) malloc ( sizeof(unsigned char) * 1536);
    unsigned char sendbuf[1536];

    int i, send_ok;
    memset(sendbuf, '\0', sizeof(unsigned char) * 1536);

    get_format(sendbuf, msg->ctag);
    /*	<cr><lf><lf>      ^^^<sid>^<date>^<time><cr><lf>
    M^^<catg>^COMPLD<cr><lf>
    [^^^<response message><cr><lf>]*
        ;*/
    sprintf((char *)sendbuf, "%s%c%c%c\"", sendbuf, SP, SP, SP);
    if(msg->solt == RB1_SLOT)
    {
        for(i = 0; i < 10; i++)
        {
            strncatnew(sendbuf, rpt_content.slot_o.rb_inp[i], strlen(rpt_content.slot_o.rb_inp[i]));
        }
    }
    else if(msg->solt == RB2_SLOT)
    {
        for(i = 0; i < 10; i++)
        {
            strncatnew(sendbuf, rpt_content.slot_p.rb_inp[i], strlen(rpt_content.slot_p.rb_inp[i]));
        }
    }
    sprintf((char *)sendbuf, "%s\"%c%c;", sendbuf, CR, LF);
    send_ok = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(send_ok == -1 && errno == EAGAIN)
    {
        /*	close(client);
        	FD_CLR(client, &allset);
        	client= -1;*/
        printf("timeout_<5>!\n");
    }
    //	free(sendbuf);
    //	sendbuf=NULL;
    return 0;
}
int cmd_rtrv_pm_msg(int client , _MSG_NODE *msg  )
{
    /*unsigned char  *sendbuf=(unsigned char *) malloc ( sizeof(unsigned char) * SENDBUFSIZE);
    memset(sendbuf,'\0',SENDBUFSIZE);
    free(sendbuf);*/
    /*"[<aid>:<type>,<date>,<time>,<almcde>,<ctfcncde>,<condtype>,<serveff>;"<cr><lf>]*"*/
    //unsigned char  *sendbuf=(unsigned char *) malloc ( sizeof(unsigned char) * 3072);
    unsigned char  sendbuf[3072];
    int i, send_ok, len;
    memset(sendbuf, '\0', sizeof(unsigned char) * 3072);

    get_format(sendbuf, msg->ctag);
    /*	<cr><lf><lf>      ^^^<sid>^<date>^<time><cr><lf>
    M^^<catg>^COMPLD<cr><lf>
    [^^^<response message><cr><lf>]*
        ;*/
    sprintf((char *)sendbuf, "%s%c%c%c\"", sendbuf, SP, SP, SP);
    for(i = 0; i < SAVE_ALM_NUM; i++)
    {
        len = strlen(saved_alms[i]);
        if(len > 0)
        {
            strncatnew(sendbuf, saved_alms[i], len);
        }
    }

    sprintf((char *)sendbuf, "%s\"%c%c;", sendbuf, CR, LF);
    send_ok = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(send_ok == -1 && errno == EAGAIN)
    {
        /*	close(client);
        	FD_CLR(client, &allset);
        	client= -1;*/
        printf("timeout_<5>!\n");
    }
    //	free(sendbuf);
    //	sendbuf=NULL;
    return 0;
}

int cmd_set_ref_prio(_MSG_NODE *msg, int flag)
{
    int num, i, len;
    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);

    len = strlen(msg->data);
    num = (int)(msg->solt - 0x61);
    if((num < 0) || (num > 13))
    {
        return 1;
    }

    if(len == 9)
    {
        if(msg->data[0] == ',') //wu prio1
        {
            for(i = 0; i < 8; i++)
            {
                if((msg->data[i + 1] < 0x30) || (msg->data[i + 1] > 0x38))
                {
                    return 1;
                }
            }
            memcpy(conf_content.slot[num].ref_prio2, &msg->data[1], 8);

        }
        else
        {
            for(i = 0; i < 8; i++)
            {
                if((msg->data[i] < 0x30) || (msg->data[i] > 0x38))
                {
                    return 1;
                }
            }
            memcpy(conf_content.slot[num].ref_prio1, msg->data, 8);
        }

    }
    else if(len == 17)
    {
        for(i = 0; i < 8; i++)
        {
            if((msg->data[i + 9] < 0x30) || (msg->data[i + 9] > 0x38))
            {
                return 1;
            }
            if((msg->data[i] < 0x30) || (msg->data[i] > 0x38))
            {
                return 1;
            }
        }
        memcpy(conf_content.slot[num].ref_prio2, &msg->data[9], 8);
        memcpy(conf_content.slot[num].ref_prio1, msg->data, 8);
    }
    else
    {
        return 1;
    }

    sprintf((char *)sendbuf, "SET-REF-PRIO:%s::%s;", msg->ctag, msg->data);
    if(flag)
    {
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        save_config();
    }
    else
    {
        sendtodown(sendbuf, msg->solt);
    }

    return 0;
}

int cmd_set_ref_mod(_MSG_NODE *msg, int flag)
{
    int num, len;
    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);

    len = strlen(msg->data);
    num = (int)(msg->solt - 0x61);
    if((num < 0) || (num > 13))
    {
        return 1;
    }
    if(len == 5)
    {
        if(msg->data[0] == ',')
        {
            if(((msg->data[2] == '0') || (msg->data[2] == '1')) &&
                    ((msg->data[4] > 0x2f) && (msg->data[4] < 0x39)))
            {
                memcpy(conf_content.slot[num].ref_mod2, &msg->data[2], 3);
            }
            else
            {
                return 1;
            }
        }
        else
        {
            if(((msg->data[0] == '0') || (msg->data[0] == '1')) &&
                    ((msg->data[2] > 0x2f) && (msg->data[2] < 0x39)))
            {
                memcpy(conf_content.slot[num].ref_mod1, &msg->data[0], 3);
            }
            else
            {
                return 1;
            }
        }
    }
    else if(len == 7)
    {
        if(((msg->data[0] == '0') || (msg->data[0] == '1')) &&
                ((msg->data[2] > 0x2f) && (msg->data[2] < 0x39)))
        {}
        else
        {
            return 1;
        }
        if(((msg->data[4] == '0') || (msg->data[4] == '1')) &&
                ((msg->data[6] > 0x2f) && (msg->data[6] < 0x39)))
        {}
        else
        {
            return 1;
        }

        memcpy(conf_content.slot[num].ref_mod1, &msg->data[0], 3);
        memcpy(conf_content.slot[num].ref_mod2, &msg->data[4], 3);
    }
    else
    {
        return 1;
    }
    sprintf((char *)sendbuf, "SET-REF-MOD:%s::%s;", msg->ctag, msg->data);
    if(flag)
    {
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        save_config();
    }
    else
    {
        sendtodown(sendbuf, msg->solt);
    }

    return 0;
}

int cmd_set_ref_sa(_MSG_NODE *msg, int flag)
{
    int i;
    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    for(i = 0; i < 8; i++)
    {
        if((msg->data[i] < '@') && (msg->data[i] > 'E'))
        {
            return 1;
        }
    }

    sprintf((char *)sendbuf, "SET-REF-SA:%s::%s;", msg->ctag, msg->data);
    if(flag)
    {
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        save_config();
    }
    else
    {
        sendtodown(sendbuf, msg->solt);
    }

    return 0;
}
int cmd_set_ref_tlb(_MSG_NODE *msg, int flag)
{
    int i;
    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    for(i = 0; i < 8; i++)
    {
        if((msg->data[i] < 'a') || (msg->data[i] > 'g'))
        {
            return 1;
        }
    }
    sprintf((char *)sendbuf, "SET-REF-TLB:%s::%s;", msg->ctag, msg->data);
	//printf("%s\n","BBBB");
    if(flag)
    {
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        save_config();
    }
    else
    {
        sendtodown(sendbuf, msg->solt);
    }

    return 0;
}


int cmd_set_ref_tlm(_MSG_NODE *msg, int flag)
{
    int num;
    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
	num = (int)(msg->solt - 0x61);
    
    if((msg->data[0] != '0') && (msg->data[0] != '1'))
    {
            return 1;
    }
    

    sprintf((char *)sendbuf, "SET-REF-TLM:%s::%s;", msg->ctag, msg->data);
    if(flag)
    {
    
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
		conf_content.slot[num].ref_2mb_lvlm = msg->data[0];
        save_config();
    }
    else 
    {
        sendtodown(sendbuf, msg->solt);
    }

    return 0;
}

int cmd_set_ref_tl(_MSG_NODE *msg, int flag)
{
    int i;
    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    for(i = 0; i < 8; i++)
    {
        if((msg->data[i] < 'a') && (msg->data[i] > 'g'))
        {
            return 1;
        }
    }

    sprintf((char *)sendbuf, "SET-REF-TL:%s::%s;", msg->ctag, msg->data);
    if(flag)
    {
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        save_config();
    }
    else
    {
        sendtodown(sendbuf, msg->solt);
    }

    return 0;
}


int cmd_set_ref_ssm_en(_MSG_NODE *msg, int flag)
{
    int num, i;
    unsigned char sendbuf[SENDBUFSIZE];

    memset(sendbuf, '\0', SENDBUFSIZE);
    num = (int)(msg->solt - 0x61);

    for(i = 0; i < 8; i++)
    {
        if((msg->data[i] < '0') && (msg->data[i] > '2'))
        {
            return 1;
        }
    }

    sprintf((char *)sendbuf, "SET-REF-IEN:%s::%s;", msg->ctag, msg->data);
    if(flag)
    {
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        memcpy(conf_content.slot[num].ref_in_ssm_en, &msg->data[0], 8);
        save_config();
    }
    else
    {
        sendtodown(sendbuf, msg->solt);
    }

    return 0;
}


int cmd_set_ref_ph(_MSG_NODE *msg, int flag)
{
    int i;
    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    for(i = 0; i < 8; i++)
    {
        if((msg->data[i] != '0') && (msg->data[i] != '1'))
        {
            return 1;
        }
    }

    sprintf((char *)sendbuf, "SET-REF-PH:%s::%s;", msg->ctag, msg->data);
    if(flag)
    {
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        save_config();
    }
    else
    {
        sendtodown(sendbuf, msg->solt);
    }

    return 0;
}

int cmd_set_rs_en(_MSG_NODE *msg, int flag)
{
    int num, i;
    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);

    for(i = 0; i < 4; i++)
    {
        if((msg->data[i] != '0') && (msg->data[i] != '1'))
        {
            return 1;
        }
    }
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        num = (int)(msg->solt - 0x61);

        memcpy(conf_content.slot[num].rs_en, msg->data, 4);
        sprintf((char *)sendbuf, "SET-RS-EN:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
    }
    else
    {
        return 1;
    }

    return 0;
}

int cmd_set_rs_tzo(_MSG_NODE *msg, int flag)
{
    int num, i;
    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);

    for(i = 0; i < 4; i++)
    {
        if((msg->data[i] < 0x40) || (msg->data[i] > 0x57))
        {
            return 1;
        }
    }
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        num = (int)(msg->solt - 0x61);
        memcpy(conf_content.slot[num].rs_tzo, msg->data, 4);
        sprintf((char *)sendbuf, "SET-RS-TZO:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
    }
    else
    {
        return 1;
    }
    return 0;
}


int cmd_set_ppx_mod(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    int num = 0, i;
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);
    for(i = 0; i < 16; i++)
    {
        if((msg->data[i] != '1') && (msg->data[i] != '2') && (msg->data[i] != '3'))
        {
            return 1;
        }
    }
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        num = (int)(msg->solt - 0x61);
        memcpy(conf_content.slot[num].ppx_mod, msg->data, 16);
        sprintf((char *)sendbuf, "SET-PPX-MOD:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
    }
    else
    {
        return 1;
    }
    return 0;
}

int cmd_set_tod_en(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    int num = 0, i;
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);
    for(i = 0; i < 16; i++)
    {
        if((msg->data[i] != '1') && (msg->data[i] != '0'))
        {
            return 1;
        }
    }
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        num = (int)(msg->solt - 0x61);
        memcpy(conf_content.slot[num].tod_en, msg->data, 16);
        sprintf((char *)sendbuf, "SET-TOD-EN:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
    }
    else
    {
        return 1;
    }

    return 0;
}

int cmd_set_tod_pro(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    int num = 0;
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);

    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        num = (int)(msg->solt - 0x61);
        //	memcpy(conf_content.slot[num].tod_en, msg->data, 16);

        if(msg->data[0] == ',')
        {
            if((msg->data[1] < 0x40) || (msg->data[1] > 0x57))
            {
                return 1;
            }
            else
            {
                conf_content.slot[num].tod_tzo = msg->data[1];
            }
        }
        else
        {
            if((msg->data[0] < 0x30) || (msg->data[0] > 0x37))
            {
                return 1;
            }
            else
            {
                if(msg->data[1] == ',')
                {
                    if( (msg->data[2] < 0x40) || (msg->data[2] > 0x57) )
                    {
                        //return 1;
                    }
                    else
                    {
                        conf_content.slot[num].tod_tzo = msg->data[2];
                    }
                }
                else
                {
                    return 1;
                }
            }

            conf_content.slot[num].tod_br = msg->data[0];
        }

        sprintf((char *)sendbuf, "SET-TOD-PRO:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }

    }

    return 0;
}

int cmd_set_igb_en(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    int num = 0, i;
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);
    for(i = 0; i < 4; i++)
    {
        if((msg->data[i] != '1') && (msg->data[i] != '0'))
        {
            return 1;
        }
    }
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        num = (int)(msg->solt - 0x61);
        memcpy(conf_content.slot[num].igb_en, msg->data, 4);
        sprintf((char *)sendbuf, "SET-IGB-EN:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
    }
    else
    {
        return 1;
    }
    return 0;
}

int cmd_set_igb_rat(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    int num = 0, i;
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);
    for(i = 0; i < 4; i++)
    {
        if((msg->data[i] < 'a') && (msg->data[i] > 'i'))
        {
            return 1;
        }
    }
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        num = (int)(msg->solt - 0x61);
        memcpy(conf_content.slot[num].igb_rat, msg->data, 4);
        sprintf((char *)sendbuf, "SET-IGB-RAT:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
    }
    else
    {
        return 1;
    }
    return 0;
}

int cmd_set_igb_max(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    int num = 0, i;
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);
    for(i = 0; i < 4; i++)
    {
        if((msg->data[i] < 'A') && (msg->data[i] > 'E'))
        {
            return 1;
        }
    }
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        num = (int)(msg->solt - 0x61);
        memcpy(conf_content.slot[num].igb_max, msg->data, 4);
        sprintf((char *)sendbuf, "SET-IGB-MAX:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }
    }
    else
    {
        return 1;
    }
    return 0;
}

int cmd_set_ntp_net(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    int num = 0, i, len = 0;
    int j, i1, i2, i3;
    j = i1 = i2 = i3 = 0;
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);
    num = (int)(msg->solt - 0x61);
    len = strlen(msg->data);

    for(i = 0; i < len; i++)
    {
        if(msg->data[i] == ',')
        {
            j++;
            if(j == 1)
            {
                i1 = i + 1;
            }
            if(j == 2)
            {
                i2 = i + 1;
            }
            if(j == 3)
            {
                i3 = i + 1;
            }
        }
    }
    if(i3 == 0)
    {
        return 1;
    }
    if(msg->data[0] != ',')
    {
        memcpy(conf_content.slot[num].ntp_ip, &msg->data[0], i1 - 1);
    }
    if(msg->data[i1] != ',')
    {
        memcpy(conf_content.slot[num].ntp_mac, &msg->data[i1], i2 - i1 - 1);
    }
    if(msg->data[i2] != ',')
    {
        memcpy(conf_content.slot[num].ntp_gate, &msg->data[i2], i3 - i2 - 1);
    }
    if(msg->data[i3] != '\0')
    {
        memcpy(conf_content.slot[num].ntp_msk, &msg->data[i3], len - i3);
    }


    sprintf((char *)sendbuf, "SET-NTP-NET:%s::%s;", msg->ctag, msg->data);
    if(flag)
    {
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        save_config();
    }
    else
    {
        sendtodown(sendbuf, msg->solt);
    }

    return 0;
}
int cmd_set_ntp_pnet(_MSG_NODE *msg, int flag)
{
	unsigned char sendbuf[SENDBUFSIZE];
	int len = 0;
	int num;
	memset(sendbuf,0, sizeof(unsigned char) * SENDBUFSIZE);
	
	len = strlen(msg->data);
	num = (int)(msg->solt - 0x61);
	
	
	sprintf((char *)sendbuf, "SET-NTP-PNET:%s::%s;", msg->ctag, msg->data);
	if(flag)
	{
		sendtodown_cli(sendbuf, msg->solt, msg->ctag);
	}
	else
	{
		sendtodown(sendbuf, msg->solt);
	}

	return 0;
}

int cmd_set_ntp_pmac(_MSG_NODE *msg, int flag)
{
	unsigned char sendbuf[SENDBUFSIZE];
	int len = 0;
	memset(sendbuf,0, sizeof(unsigned char) * SENDBUFSIZE);
	
	len = strlen(msg->data);
	
	
	sprintf((char *)sendbuf, "SET-NTP-PMAC:%s::%s;", msg->ctag, msg->data);
	if(flag)
	{
		sendtodown_cli(sendbuf, msg->solt, msg->ctag);
	}
	else
	{
		sendtodown(sendbuf, msg->solt);
	}

	return 0;
}


int cmd_set_ntp_pen(_MSG_NODE *msg, int flag)
{
	unsigned char sendbuf[SENDBUFSIZE];
	int len = 0;
	memset(sendbuf,0, sizeof(unsigned char) * SENDBUFSIZE);
	
	len = strlen(msg->data);
	
	
	sprintf((char *)sendbuf, "SET-NTP-PEN:%s::%s;", msg->ctag, msg->data);
	if(flag)
	{
		sendtodown_cli(sendbuf, msg->solt, msg->ctag);
	}
	else
	{
		sendtodown(sendbuf, msg->solt);
	}

	return 0;
}


int cmd_set_ntp_en(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    int num = 0, i, len;
    int j, i1, i2, i3, i4;
    j = i1 = i2 = i3 = i4 = 0;
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);
    num = (int)(msg->solt - 0x61);
    len = strlen(msg->data);
    for(i = 0; i < len; i++)
    {
        if(msg->data[i] == ',')
        {
            j++;
            if(j == 1)
            {
                i1 = i + 1;
            }
            if(j == 2)
            {
                i2 = i + 1;
            }
            if(j == 3)
            {
                i3 = i + 1;
            }
            if(j == 4)
            {
                i4 = i + 1;
            }
        }
    }
#if 1
    if((msg->data[0] == '0') || (msg->data[0] == '1'))
    {
        conf_content.slot[num].ntp_bcast_en = msg->data[0];
    }
    if(msg->data[i1] != ',')
    {
        memcpy(conf_content.slot[num].ntp_interval, &msg->data[i1], i2 - i1 - 1);
    }
    if((msg->data[i2] > 0x40) && (msg->data[i2] < 0x50))
    {
        conf_content.slot[num].ntp_tpv_en = msg->data[i2];
    }
    if((msg->data[i3] == '0') || (msg->data[i3] == '1'))
    {
        conf_content.slot[num].ntp_md5_en = msg->data[i3];
    }
    /*if((msg->data[i4] == '0') || (msg->data[i4] == '1'))
    {
        conf_content.slot[num].ntp_ois_en = msg->data[i4];
    }*/
#endif
    sprintf((char *)sendbuf, "SET-NTP-EN:%s::%s;", msg->ctag, msg->data);

    if(flag)
    {
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        save_config();
    }
    else
    {
        sendtodown(sendbuf, msg->solt);
    }

    return 0;
}


int cmd_set_ntp_key(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    unsigned char keyid[5];
	unsigned char i =0;
    int num = 0;
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);
    memset(keyid, '\0', 5);
    num = (int)(msg->solt - 0x61);

    if((msg->data[0] != '0') && (msg->data[0] != '1') && (msg->data[0] != '2'))
    {
        return 1;
    }
    else if(msg->data[1] != ',')
    {
        return 1;
    }
    else if(msg->data[6] != ',')
    {
        return 1;
    }
    #if 1
    	memcpy(keyid, &msg->data[2], 4);
    	if(msg->data[0]=='0')
    	{
    		for(i=0;i<4;i++)
    			{
    				if(conf_content.slot[num].ntp_key_flag[i]!=2)
    				{
    					conf_content.slot[num].ntp_key_flag[i]=2;
    					memcpy(conf_content.slot[num].ntp_keyid[i], &msg->data[2], 4);
    					memcpy(conf_content.slot[num].ntp_key[i], &msg->data[7], 16);
    					break;
    				}
    				//else if(i==3)
    				//	return 1;
    			}
    	}
    	else if(msg->data[0]=='1')
    	{
    		for(i=0;i<4;i++)
    			{
    				if(strncmp1(keyid, conf_content.slot[num].ntp_keyid[i] , 4)==0)
    					{
    						conf_content.slot[num].ntp_key_flag[i]=1;
    						memset(conf_content.slot[num].ntp_keyid[i],'\0',5);
    						memset(conf_content.slot[num].ntp_key[i],'\0',17);
    						break;
    					}
    				//else if(i==3)
    				//	return 1;
    			}
    	}
    	else if(msg->data[0]=='2')
    	{
    		for(i=0;i<4;i++)
    			{
    				if(strncmp1(keyid, conf_content.slot[num].ntp_keyid[i] , 4)==0)
    					{
    						memset(conf_content.slot[num].ntp_key[i],'\0',17);
    						memcpy(conf_content.slot[num].ntp_key[i], &msg->data[7], 16);
    						break;
    					}
    				//else if(i==3)
    				//	return 1;
    			}
    	}
	#endif
    
    sprintf((char *)sendbuf, "SET-NTP-KEY:%s::%s;", msg->ctag, msg->data);
    if(flag)
    {
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        save_config();
    }
    else
    {
        sendtodown(sendbuf, msg->solt);
    }

    return 0;
}

int cmd_set_ntp_ms(_MSG_NODE *msg, int flag)
{

    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);
    sprintf((char *)sendbuf, "SET-NTP-MS:%s::%s;", msg->ctag, msg->data);

    if(flag)
    {
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        //save_config();
    }
    else
    {
        sendtodown(sendbuf, msg->solt);
    }

    return 0;
}

int cmd_set_mcp_fb(_MSG_NODE *msg, int flag)
{

    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);

    if((msg->data[0] != '0') && (msg->data[0] != '1'))
    {
        return 1;
    }
    if(conf_content.slot_u.fb == msg->data[0])
    {}
    else
    {
        conf_content.slot_u.fb = msg->data[0];
        sprintf((char *)sendbuf, "SET-RB-FB:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            if(slot_type[14] == 'R' || slot_type[14] == 'K')
            {
                sendtodown_cli(sendbuf, RB1_SLOT, msg->ctag);
            }
            if(slot_type[15] == 'R' || slot_type[15] == 'K')
            {
                sendtodown_cli(sendbuf, RB2_SLOT, msg->ctag);
            }
        }
        else
            //sendtodown(sendbuf, msg->solt);
        {
            if(slot_type[14] == 'R' || slot_type[14] == 'K')
            {
                sendtodown(sendbuf, RB1_SLOT);
            }
            if(slot_type[15] == 'R' || slot_type[15] == 'K')
            {
                sendtodown(sendbuf, RB2_SLOT);
            }
        }
        save_config();

    }
    respond_success(current_client, msg->ctag);
    return 0;
}

int cmd_set_rb_sa(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);

    if((msg->data[0] < '@') || (msg->data[0] > 'E'))
    {
        return 1;
    }
    sprintf((char *)sendbuf, "SET-RB-SA:%s::%s;", msg->ctag, msg->data);
    if(flag)
    {
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        //save_config();
    }
    else
    {
        sendtodown(sendbuf, msg->solt);
    }

    return 0;
}

int cmd_set_rb_tl(_MSG_NODE *msg, int flag)
{
    if((msg->data[0] < 'a') || (msg->data[0] > 'g'))
    {
        return 1;
    }
    if(msg->solt == RB1_SLOT)
    {
        rpt_content.slot_o.rb_tl_2mhz = msg->data[0];
    }
    else if(msg->solt == RB2_SLOT)
    {
        rpt_content.slot_p.rb_tl_2mhz = msg->data[0];
    }
    get_out_lev(0, 0);
    respond_success(current_client, msg->ctag);
    return 0;
}
//
//!设置钟盘1槽位REFTF输入IRIGB信号性能超限阀值
//!设置钟盘2槽位REFTF输入IRIGB信号性能超限阀值
//
int cmd_set_rb_thresh(_MSG_NODE *msg, int flag)
{
    int len, num;
    int temp;
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    len = strlen(msg->data);
	
    if((msg->data[0] < 0x31)&&(msg->data[0] > 0x32))          //参考输入IRIGB信号的槽位号{1:2}
    {
        return 1;
    }
    temp = atoi(&msg->data[2]);
    if((temp < 1) || (temp > 999999))//6字节，最大为999，999，最小为1
    {
        return 1;
    }
    if(msg->solt == RB1_SLOT)            //钟盘槽位号15+0x60        
    {
        num = (int)(msg->data[0] - 0x31);//参考输入IRIGB信号的槽位号{1:2}
        if((num >= 0x00) && (num <= 0x01))
        {
            //if(strcmp(conf_content.slot_o.dely[num], &msg->data[2])!=0)
            //	{
            memset(conf_content.slot_o.thresh[num], '\0', 7);
            memcpy(conf_content.slot_o.thresh[num], &msg->data[2], len - 2);

            sprintf((char *)sendbuf, "SET-RB-TH:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
            //}
        }
    }
    else if(msg->solt == RB2_SLOT)        //钟盘槽位号16+0x60
    {
        num = (int)(msg->data[0] - 0x31); //参考输入IRIGB信号的槽位号{1:2}
        if((num >= 0x00) && (num <= 0x01))
        {
            //if(strcmp(rpt_content.slot_p., &msg->data[2])!=0)
            //	{
            memset(conf_content.slot_p.thresh[num], '\0', 7);
            memcpy(conf_content.slot_p.thresh[num], &msg->data[2], len - 2);
			printf("set rb thresh%s,%s \n",conf_content.slot_p.thresh[0],conf_content.slot_p.thresh[1]);

            sprintf((char *)sendbuf, "SET-RB-TH:%s::%s;", msg->ctag, msg->data);
            if(flag)
            {
                
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
            //	}
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}

int cmd_set_alm_msk(_MSG_NODE *msg, int inflag)
{
    //msg->data:[<type>,<xxxxxxxxxxx>]+
    int typeflag = 0; //1:表示为msg->data的type段；0:表示为msg->data的xxxxxxxxxxx段；
    int num;
    int i, j, len;
    int msgType[128], index = 0;
    unsigned char *data;
	unsigned char errflag;
	unsigned char maxlen;
	

	// downflag[3] [0]:REFTF [1]:RB 2MHZ/2MB [2]GBTP
	//unsigned char almmsknum[3];
	//unsigned char sendbuf[SENDBUFSIZE];
	//unsigned char maskdata[17];
	//unsigned char tmpdata[7];
	errflag = 0;
	maxlen  = 0;
	//memset(tmpdata,'\0',7);
	//memset(maskdata,'\0',17);
	
    //memset(sendbuf, '\0', SENDBUFSIZE);
	//memset(almmsknum,'\0',3);

	
    printf("-----\n%s:\n", __FUNCTION__);
    if(NULL == msg)
    {
        printf("cmd_set_alm_msk is error\n");
        return -1;
    }
    else
    {
        len = strlen(msg->data);
        data = (char *)malloc(len);
        if(NULL == data)
        {
            printf("cmd_set_alm_msk Malloc is failed\n");
            return -1;
        }
        else
        {
            memset(data, 0, len);
            memcpy(data, msg->data, len);
            printf("Msk data:%s\n", data);
        }
    }
	
    for(i = 0; i < len; i++)
    {
        if('=' == data[i])
        {
            msgType[index] = i - 1; //msg->data的type段所在位置
            index++;
        }
    }
	//
	//!the tail of msgtype array mark as -1. the -1 mean terminiter.
    msgType[index] = -1;
    index = 0;
    i = 0;
    num = -1;
	//
	//! Check the MsgType is or not out of bounds.
	//
    while(-1 != msgType[index])
    {
        //
        //if the i equal to the value of the msgType,find the table of mask .
        //
        if(i == msgType[index])
        {
            typeflag = 1;
			errflag =  1;
            
        }
        else 
        {
            typeflag = 0;
        }
		
        //
        // The Header Msgtype Should Be Corrected,or not the fuc don't deal.
        //
		if(1 == errflag)
		{
			
		        if(1 == typeflag)
		        {
		            printf("The CMD RALM----slot_type:%c\n", data[i]);
					
						
		            for(j = 0; j < alm_msk_total; j++)
		            {
		            //if equal S or K, record the num of mask.
						//almmsknum[0] = (data[i] == 'S')? j : 0;
						//almmsknum[1] = (data[i] == 'K')? j : 0;
		                if(data[i] == conf_content.alm_msk[j].type)
		                {
		                    num = j;
		                    break;
		                }
		            }
		            if(-1 == num)
		            {
		                for(j = 0; j < alm_msk_total; j++)
		                {
		                    if('\0' == conf_content.alm_msk[j].type)
		                    {
		                        conf_content.alm_msk[j].type = data[i];
		                        num = j;
		                        break;
		                    }
		                }
		            }
		            i += 2;
		        }
		        else 
		        {
		            printf("num= %d\n", num);
		            index++;
					 
		            if(-1 != msgType[index])
		            {
		                //
		                //limit the max len of data to  6。
		                //
		                maxlen =  msgType[index] - i - 1;
						
						if(maxlen <= ALM_MASK_LEN)
						{
			                if((data[i] >> 4) == 0x4)
			                {
			                    memcpy(conf_content.alm_msk[num].data, &data[i], msgType[index] - i - 1);
			                    i = msgType[index];
			                }
			                else
			                {
			                    memcpy(conf_content.alm_msk[num].data + 6, &data[i], msgType[index] - i - 1);
			                    i = msgType[index];
			                }
						}
		                printf("-1 != msgType[index]---%c:%s\n", conf_content.alm_msk[num].type, conf_content.alm_msk[num].data);
		            }
		            else
		            {
		            
		               //
		               //limit the max len of data to  6。
		               //
		               maxlen =  len - i;
						
					   if(maxlen <= ALM_MASK_LEN)
					   {
			                if((data[i] >> 4) == 0x4)
			                {
			                    memcpy(conf_content.alm_msk[num].data, &data[i], len - i);
			                }
			                else
			                {
			                    memcpy(conf_content.alm_msk[num].data + 6, &data[i], len - i);
			                }
					   }
			                //printf("-1 == msgType[index]---%c  %s\n",conf_content.alm_msk[num].type,conf_content.alm_msk[num].data);
		            }
		            typeflag = 1;
		            num = -1;
		        }
			}
		    else
		    {
		        msgType[index] = -1;
		    }

    }
    free(data);
	//
	//REFTF E1 OR TIME PORT LAMP ON/OFF control;
	/*if(almmsknum[0])
	{
	   num = almmsknum[0];

       tmpdata[2] =  (conf_content.alm_msk[num].data[3] & 0x0C) >> 2;
	   tmpdata[2] |= (conf_content.alm_msk[num].data[2] & 0x03) <<2 ;

	   tmpdata[3] =  (conf_content.alm_msk[num].data[2] & 0x0C) >>2;
       tmpdata[3] |= ( conf_content.alm_msk[num].data[3] & 0x03) <<2;

	   tmpdata[0] =  conf_content.alm_msk[num].data[4] &0x0f;
       
       tmpdata[1] =  conf_content.alm_msk[num].data[5] &0x0f;
       
	   for(i = 0; i < 4;i++)
	   for(j = 0; j < 4; j++)
	   {
		    //
		    if((tmpdata[i] & (1 << j)) == 0)
						maskdata[4*i+j] = '0';
			else
			            maskdata[4*i+j] = '1';
	   }

	   for(i = 0; i <14; i++)
	   {
	   		if(slot_type[i] == 'S')
				break;
	   }
	   sprintf((char *)sendbuf, "SET-REF-LAMP:%s::%s;", msg->ctag, maskdata);
	   sendtodown(sendbuf, i + 0x61);
	}*/
	save_config();
    return 0;
}

//cmd_rtrv_ralm_mask
int cmd_rtrv_ralm_mask(int client , _MSG_NODE *msg  )
{

    unsigned char  sendbuf[3072];
    int i, send_ok, len;
    memset(sendbuf, '\0', sizeof(unsigned char) * 3072);

    get_format(sendbuf, msg->ctag);
    /*	<cr><lf><lf>      ^^^<sid>^<date>^<time><cr><lf>
    M^^<catg>^COMPLD<cr><lf>
    [^^^<response message><cr><lf>]*
        ;*/
    /*[<type>:<data>:<cr><lf>]*/
    sprintf((char *)sendbuf, "%s%c%c%c\"", sendbuf, SP, SP, SP);
    for(i = 0; i < alm_msk_total; i++)
    {
        if(conf_content.alm_msk[i].type == '\0')
        {
            len = strlen(sendbuf);

            if(',' == sendbuf[len - 1])
            {
                sendbuf[len - 1] = '\0';
            }

            break;
        }
        //out盘等输出盘不上传
        else if(conf_content.alm_msk[i].type == 'B' ||
                conf_content.alm_msk[i].type == 'D' ||
                conf_content.alm_msk[i].type == 'E' ||
                conf_content.alm_msk[i].type == 'F' ||
                conf_content.alm_msk[i].type == 'G' ||
                conf_content.alm_msk[i].type == 'H' ||
                conf_content.alm_msk[i].type == 'J' ||
                conf_content.alm_msk[i].type == 'a' ||
                conf_content.alm_msk[i].type == 'c' ||
                conf_content.alm_msk[i].type == 'b' ||
                //conf_content.alm_msk[i].type == 'f' ||
                conf_content.alm_msk[i].type == 'h' ||
                conf_content.alm_msk[i].type == 'i'
               )
        {
            continue;
        }

        if( (conf_content.alm_msk[i].data[6] >> 4) == 0x5 )
        {
            sprintf((char *)sendbuf, "%s%c%c", sendbuf, conf_content.alm_msk[i].type, '=');
            strncatnew(sendbuf, conf_content.alm_msk[i].data, 6);
            strncatnew(sendbuf, ":", 1);
            strncatnew(sendbuf, &(conf_content.alm_msk[i].data[6]), 6);

        }
        else
        {
            sprintf((char *)sendbuf, "%s%c%c%s", sendbuf, conf_content.alm_msk[i].type, '=', conf_content.alm_msk[i].data);
        }

        if(conf_content.alm_msk[i + 1].type != '\0' && i + 1 < alm_msk_total)
        {
            strncatnew(sendbuf, ",", 1);
        }
        else
        {
            //sprintf((char *)sendbuf,"%s\"%c%c;",sendbuf,CR,LF);
        }
    }
    sprintf((char *)sendbuf, "%s\"%c%c;", sendbuf, CR, LF);
    send_ok = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(send_ok == -1 && errno == EAGAIN)
    {
        printf("timeout_<5>!\n");
    }
    return 0;
}




int cmd_set_ext_online(_MSG_NODE *msg, int flag)
{
    //
    //!
    //
    unsigned char onlineSta = '\0';
    int i;
    if(msg == NULL)
    {
        return 1;
    }

    onlineSta = msg->data[0];

    for(i = 0; i < 3; i++)
    {
        if( ( (onlineSta >> i) & 0x01 ) == 0 )
        {
            memset(gExtCtx.extBid[i], 'O', 15);
        }
        else
        {
            ;
        }
    }

    gExtCtx.save.onlineSta = onlineSta;
    if(0 == ext_write_flash(&(gExtCtx.save)))
    {
        return 1;
    }
    return 0;
}


int cmd_qext_online(int client, _MSG_NODE *msg)
{

    int send_ok, num;
    unsigned char sendbuf[1024];

    memset(sendbuf, '\0', sizeof(unsigned char) * 1024);
    num = (int)(msg->solt - 'a');

    get_format(sendbuf, msg->ctag);
    sprintf((char *)sendbuf, "%s%c%c%c\"", sendbuf, SP, SP, SP);

    if( (gExtCtx.save.onlineSta & 0xF0) != 0x40 )
    {
        gExtCtx.save.onlineSta = 0x47;
        sprintf((char *)sendbuf, "%s%c", sendbuf, gExtCtx.save.onlineSta);
    }
    else
    {
        sprintf((char *)sendbuf, "%s%c", sendbuf, gExtCtx.save.onlineSta);
    }

    sprintf((char *)sendbuf, "%s\"%c%c;", sendbuf, CR, LF);
    send_ok = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(send_ok == -1 && errno == EAGAIN)
    {
        printf("timeout_<3>!\n");
    }
    return 0;
}


int cmd_set_leap_mod(_MSG_NODE *msg, int flag)
{
    char leapMod;

    leapMod = msg->data[0];
    conf_content3.LeapMod = 0;
    conf_content3.LeapMod = leapMod;
    save_config();

    return 0;
}

/*
2014.12.10
返回闰秒模式
0---手动
1---自动
*/
int cmd_rtrv_leap_mod(int client, _MSG_NODE *msg)
{

    int send_ok, num;
    unsigned char sendbuf[1024];

    memset(sendbuf, '\0', sizeof(unsigned char) * 1024);
    num = (int)(msg->solt - 'a');

    get_format(sendbuf, msg->ctag);
    sprintf((char *)sendbuf, "%s%c%c%c\"", sendbuf, SP, SP, SP);

    if( '0' != conf_content3.LeapMod && '1' != conf_content3.LeapMod)
    {
        conf_content3.LeapMod = '0';
        sprintf((char *)sendbuf, "%s%c", sendbuf, conf_content3.LeapMod);
    }
    else
    {
        sprintf((char *)sendbuf, "%s%c", sendbuf, conf_content3.LeapMod);
    }

    sprintf((char *)sendbuf, "%s\"%c%c;", sendbuf, CR, LF);
    send_ok = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(send_ok == -1 && errno == EAGAIN)
    {
        printf("timeout_<3>!\n");
    }
    return 0;
}

int cmd_rtrv_igb_tzone(int client, _MSG_NODE *msg)
{

    int send_ok, num;
    unsigned char sendbuf[1024];

    num = (int)(msg->solt - 'a');
    if( ((msg->solt) <= '`') || ((msg->solt) >= 'o') )
    {
        return 1;
    }
    else if( slot_type[num] != 'W' )
    {
        return 1;
    }
    else
    {
        memset(sendbuf, '\0', sizeof(unsigned char) * 1024);
    }

    get_format(sendbuf, msg->ctag);
    sprintf((char *)sendbuf, "%s%c%c%c\"", sendbuf, SP, SP, SP);
    if( '0' == rpt_content.slot[num].igb_tzone || '8' == rpt_content.slot[num].igb_tzone)
    {
        sprintf((char *)sendbuf, "%s%c", sendbuf, rpt_content.slot[num].igb_tzone);
    }
    else
    {
        sprintf((char *)sendbuf, "%s%c", sendbuf, '0');
    }

    sprintf((char *)sendbuf, "%s\"%c%c;", sendbuf, CR, LF);
    send_ok = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(send_ok == -1 && errno == EAGAIN)
    {
        printf("timeout_<3>!\n");
    }
    return 0;
}



/*
2015.5.27
set irigb16 timezone
*/
int cmd_set_igb_tzone(_MSG_NODE *msg, int flag)
{
    unsigned char sendbuf[SENDBUFSIZE];
    int num = 0;
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);

    if( ((msg->solt) > '`') && ((msg->solt) < 'o') )
    {
        num = (int)(msg->solt - 0x61);
        if(slot_type[num] != 'W')
        {
            return 1;
        }

        conf_content3.irigb_tzone[num] = (msg->data)[0];
        sprintf((char *)sendbuf, "SET-IGB-TZONE:%s::%s;", msg->ctag, msg->data);
        if(flag)
        {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
        }
        else
        {
            sendtodown(sendbuf, msg->solt);
        }

        return 0;
    }
    else
    {
        return 1;
    }
}


int cmd_sys_cpu(int client, _MSG_NODE *msg)
{

    int send_ok;
    unsigned char sendbuf[1024];
    char *sys_cpu = "6";
    char *sys_mem = "10";

    memset(sendbuf, '\0', sizeof(unsigned char) * 1024);

    get_format(sendbuf, msg->ctag);
	sprintf((char *)sendbuf, "%s%c%c%c\"", sendbuf, SP, SP, SP);
    sprintf((char *)sendbuf, "%s%s,%s", sendbuf, sys_cpu, sys_mem);
	sprintf((char *)sendbuf, "%s\"%c%c;", sendbuf, CR, LF);

    send_ok = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(send_ok == -1 && errno == EAGAIN)
    {
        printf("timeout_<3>!\n");
    }
    return 0;

}


int read_ip(unsigned const char *ip, unsigned char *out_ip)
{
    //0100 Y7Y6Y5Y4     0100 Y3Y2Y1Y0
    int i;
    unsigned char temp = 0;
	char in_ip[9];
    if(in_ip == NULL || out_ip == NULL)
    {
        printf("read ip error\n");
        return -1;
    }
    else if(*in_ip == ',')
    {
        return 1;
    }
    else
    {
        memset(in_ip,0,9);
		memcpy(in_ip,ip,8);
    }

    for(i = 0; i < 8; i++)
    {
        if( ((*(in_ip + i)) & 0xf0) != 0x40 )
        {
            return 1;
        }
        else
        {
            if( (i & 0x01) == 0x00 )
            {
                temp = (*(in_ip + i)) << 4;
            }
            else
            {
                temp |= ((*(in_ip + i)) & 0x0f);
                sprintf((char *)out_ip, "%s%d", out_ip, temp);
                if(7 == i)
                {
                    ;
                }
                else
                {
                    sprintf((char *)out_ip, "%s.", out_ip);
                }
                temp = 0;
            }
        }
    }
    return 0;
}

int cmd_rtrv_mtc_ip(int client, _MSG_NODE *msg)
{

    int num, send_ok;
    unsigned char sendbuf[1024];
	unsigned char sendtmp[256];

    memset(sendbuf, '\0', sizeof(unsigned char) * 1024);
    memset(sendtmp, '\0', sizeof(unsigned char) * 256);
    num = (int)(msg->solt - 'a');
    get_format(sendtmp, msg->ctag);
	
	sprintf((char *)sendbuf, "%s%c%c%c\"RPT-MTC-IP,%s,%s,%s,%s\"%c%c;",sendtmp,SP, SP, SP, &rpt_ptp_mtc.ptp_mtc_ip[num][0][0],
            &rpt_ptp_mtc.ptp_mtc_ip[num][1][0],
            &rpt_ptp_mtc.ptp_mtc_ip[num][2][0],
            &rpt_ptp_mtc.ptp_mtc_ip[num][3][0],
            CR,LF);
		
    /*sprintf((char *)sendbuf, "%sRPT-MTC-IP,%s,%s,%s,%s", sendbuf,
            &rpt_ptp_mtc.ptp_mtc_ip[num][0][0],
            &rpt_ptp_mtc.ptp_mtc_ip[num][1][0],
            &rpt_ptp_mtc.ptp_mtc_ip[num][2][0],
            &rpt_ptp_mtc.ptp_mtc_ip[num][3][0]);*/
	
	/*sprintf((char *)sendbuf, "%s\"%c%c", sendbuf, CR, LF);*/
	//printf("mtc send %s\n",&rpt_ptp_mtc.ptp_mtc_ip[num][0][0]);
    //printf("mtc send %d,%s\n",num,sendbuf);
    send_ok = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(send_ok == -1 && errno == EAGAIN)
    {
        printf("timeout_<3>!\n");
    }
    return 0;

}
int cmd_rtrv_uni_ip(int client, _MSG_NODE *msg)
{

    int num, send_ok;
    unsigned char sendbuf[1024];
	unsigned char sendtmp[256];

    memset(sendbuf, '\0', sizeof(unsigned char) * 1024);
    memset(sendtmp, '\0', sizeof(unsigned char) * 256);
    num = (int)(msg->solt - 'a');
    get_format(sendtmp, msg->ctag);
	
	sprintf((char *)sendbuf, "%s%c%c%c\"RPT-MTC-IP,%s|%s|%s|%s\"%c%c;",sendtmp,SP, SP, SP, &rpt_ptp_mtc.ptp_mtc_ip[num][0][0],
            &rpt_ptp_mtc.ptp_mtc_ip[num][1][0],
            &rpt_ptp_mtc.ptp_mtc_ip[num][2][0],
            &rpt_ptp_mtc.ptp_mtc_ip[num][3][0],
            CR,LF);
		
    /*sprintf((char *)sendbuf, "%sRPT-MTC-IP,%s,%s,%s,%s", sendbuf,
            &rpt_ptp_mtc.ptp_mtc_ip[num][0][0],
            &rpt_ptp_mtc.ptp_mtc_ip[num][1][0],
            &rpt_ptp_mtc.ptp_mtc_ip[num][2][0],
            &rpt_ptp_mtc.ptp_mtc_ip[num][3][0]);*/
	
	/*sprintf((char *)sendbuf, "%s\"%c%c", sendbuf, CR, LF);*/
	//printf("mtc send %s\n",&rpt_ptp_mtc.ptp_mtc_ip[num][0][0]);
    //printf("mtc send %d,%s\n",num,sendbuf);
    send_ok = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(send_ok == -1 && errno == EAGAIN)
    {
        printf("timeout_<3>!\n");
    }
    return 0;

}

int cmd_rtrv_mtc_dom(int client, _MSG_NODE *msg)
{

    int num, send_ok;
    unsigned char sendbuf[1024];

    memset(sendbuf, '\0', sizeof(unsigned char) * 1024);

    num = (int)(msg->solt - 'a');
    get_format(sendbuf, msg->ctag);
    sprintf((char *)sendbuf, "%sRPT-MTC-DOM,%s,%s,%s,%s;", sendbuf,
            &rpt_ptp_mtc.ptp_dom[num][0][0],
            &rpt_ptp_mtc.ptp_dom[num][1][0],
            &rpt_ptp_mtc.ptp_dom[num][2][0],
            &rpt_ptp_mtc.ptp_dom[num][3][0]);

    send_ok = send(client, sendbuf, strlen(sendbuf), MSG_NOSIGNAL);
    if(send_ok == -1 && errno == EAGAIN)
    {
        printf("timeout_<3>!\n");
    }
    return 0;

}


void set_sta_ip(unsigned  const char *ip, unsigned char solt)
{
    unsigned char r_ip[50];
    unsigned char ip_temp[9];
    unsigned char sendbuf[50];

    if(NULL == ip)
    {
        return;
    }
    else
    {
        memset(sendbuf, 0, 50);
        memset(r_ip, 0, 50);
        memset(ip_temp, 0, 9);
		memcpy(ip_temp, ip, 8);

        sprintf((char *)sendbuf, "SET-STA1:000000::Board61850IP=");
    }
#if 1
    if(!read_ip(ip_temp, r_ip))
    {
        memset(conf_content3.sta1.ip, 0, 9);
        memcpy(conf_content3.sta1.ip, ip_temp, 8);
        sprintf((char *)sendbuf, "%s%s;", sendbuf, r_ip);
        sendtodown(sendbuf, solt);
    }
    else
    {
    }
#endif
}

void set_sta_gateway(unsigned char *ip, unsigned char solt)
{
    unsigned char r_ip[50];
    unsigned char sendbuf[SENDBUFSIZE];

    if(NULL == ip)
    {
        return;
    }
    else
    {
        memset(sendbuf, 0, SENDBUFSIZE);
        memset(r_ip, 0, 50);
        sprintf((char *)sendbuf, "SET-STA1:000000::Board61850GateWay=");
    }

    if(!read_ip(ip, r_ip))
    {
        memset(conf_content3.sta1.gate, 0, 9);
        memcpy(conf_content3.sta1.gate, ip, 8);
        sprintf((char *)sendbuf, "%s%s;", sendbuf, r_ip);
        sendtodown(sendbuf, solt);
    }
    else
    {
        //;
    }
}

void set_sta_netmask(unsigned char *ip, unsigned char solt)
{
    unsigned char r_ip[50];
    unsigned char sendbuf[SENDBUFSIZE];

    if(NULL == ip)
    {
        return;
    }
    else
    {
        memset(sendbuf, 0, SENDBUFSIZE);
        memset(r_ip, 0, 50);
        sprintf((char *)sendbuf, "SET-STA1:000000::Board61850NetMask=");
    }

    if(!read_ip(ip, r_ip))
    {
        memset(conf_content3.sta1.mask, 0, 9);
        memcpy(conf_content3.sta1.mask, ip, 8);

        sprintf((char *)sendbuf, "%s%s;", sendbuf, r_ip);
        sendtodown(sendbuf, solt);
    }
    else
    {
        //;
    }
}


/*
SET-STA1-NET:
[<tid>]:<aid>:<ctag>:<password>:
<type>,<num>,<ip>,<mac>,<gateway>,<netmask>,<dns1>,<dns2>;
*/
int cmd_set_sta_net(_MSG_NODE *msg, int flag)
{

   	unsigned char *p = NULL;
    unsigned char comma_id = 0;
	signed char len;

    printf("cmd_set_sta_net data:\n%s\n", msg->data);
    printf("slot:%c\n", msg->solt);

    if( ((msg->solt) < 'a') || ((msg->solt) > 'm') )
    {
        return 1;
    }
    else
    {
        p = msg->data;
		len = strlen(msg->data);
    }

    while( (len>=0) && ((*p) != ';'))
    {
    	len --;
        if(',' == (*p))
        {
            comma_id++;
            switch (comma_id)
            {
            case 0 :
                //tpye
                break;
            case 1 :
                //num
                break;
            case 2 ://ip
                if( *(p+1) != ',' && *(p+1) != ';' )
                {
                    set_sta_ip((p+1), msg->solt);
                }
                break;
            case 3 ://mac
                break;
            case 4 ://gateway
                if( *(p + 1) != ',' && *(p + 1) != ';' )
                {
                    set_sta_gateway((p + 1), msg->solt);
                }
                break;
            case 5 ://mask
                if( *(p + 1) != ',' && *(p + 1) != ';' )
                {
                    set_sta_netmask((p + 1), msg->solt);
                }
                break;
            default://others
                break;
            }

        }
        else
        {
            ;
        }
        p++;
    }

    if(flag)
    {
        //sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        save_config();
    }
    else
    {
        //sendtodown(sendbuf, msg->solt);
    }
    respond_success(current_client, msg->ctag);
    return 0;
}
//
//!set pgein net parameters
//
int cmd_set_pgein_net(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    int num;
    int port;
    int i, j, i1, i2, i3;
    int len = strlen(msg->data);
    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
   
    memset(sendbuf, '\0', SENDBUFSIZE);
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        if((msg->data[2] == '1') )
        {
            num = (int)(msg->solt - 0x61);
            port = (int)(msg->data[2] - 0x31);
       
            for(i = 0; i < len; i++)
            {
                if(msg->data[i] == ',')
                {
                    j++;
                    if(j == 2)
                    {
                        i1 = i + 1;
                    }
                    if(j == 3)
                    {
                        i2 = i + 1;
                    }
                    if(j == 4)
                    {
                        i3 = i + 1;
                    }
                }
            }
            if(i3 == 0)
            {
                return 1;
            }
            conf_content.slot[num].ptp_type = msg->data[0];
            if(msg->data[i1] != ',')
            {
                memcpy(conf_content.slot[num].ptp_ip[port], &msg->data[i1], i2 - i1 - 1);
				conf_content.slot[num].ptp_ip[port][8] = '\0';
            }
            
            if(msg->data[i2] != ',')
            {
                memcpy(conf_content.slot[num].ptp_gate[port], &msg->data[i2], i3 - i2 - 1);
				conf_content.slot[num].ptp_gate[port][8] = '\0';
				
            }
            if(msg->data[i3] != '\0')
            {
                memcpy(conf_content.slot[num].ptp_mask[port], &msg->data[i3], len - i3);
				conf_content.slot[num].ptp_mask[port][8] = '\0';
            }
            
            sprintf((char *)sendbuf, "SET-PGEIN-NET:%s;", msg->data);

			if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
            //		}
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
//
//!set pgein mode parameters
//
int cmd_set_pgein_mod(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    int num;
    int port;
    int i, j, i1, i2, i3, i4, i5, i6;
	
    int len = strlen(msg->data);
    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
    i5 = 0;
    i6 = 0;
   
    memset(sendbuf, '\0', SENDBUFSIZE);
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        if((msg->data[2] == '1') )
        {
            
            num = (int)(msg->solt - 0x61);
            port = (int)(msg->data[2] - 0x31);
          
            for(i = 0; i < len; i++)
            {
                if(msg->data[i] == ',')
                {
                    j++;
                    if(j == 2)
                    {
                        i1 = i + 1;
                    }
                    if(j == 3)
                    {
                        i2 = i + 1;
                    }
                    if(j == 4)
                    {
                        i3 = i + 1;
                    }
                    if(j == 5)
                    {
                        i4 = i + 1;
                    }
                    if(j == 6)
                    {
                        i5 = i + 1;
                    }
                    if(j == 7)
                    {
                        i6 = i + 1;
                    }
                    
                }
            }
            if(i6 == 0)
            {
                return 1;
            }
            conf_content.slot[num].ptp_type = msg->data[0];
            //
            //! set delaytype: 0-P2P, 1-E2E; 
            //
            if((msg->data[i1] == '0') || (msg->data[i1] == '1'))
            {
                conf_content.slot[num].ptp_delaytype[port] = msg->data[i1];
            }
			//
			//! 0-unicast,1-multicast;
			//
            if((msg->data[i2] == '0') || (msg->data[i2] == '1'))
            {
                conf_content.slot[num].ptp_multicast[port] = msg->data[i2];
            }
			//
			//! enq : 0-two,1-three
			//
            if((msg->data[i3] == '0') || (msg->data[i3] == '1'))
            {
                conf_content.slot[num].ptp_enp[port] = msg->data[i3];
            }
			//
			//!0:one step,1:two step;
			//
            if((msg->data[i4] == '0') || (msg->data[i4] == '1'))
            {
                conf_content.slot[num].ptp_step[port] = msg->data[i4];
            }
			//
			//!delay req fre
			//
            if(msg->data[i5] != ',')
            {
                memcpy(conf_content.slot[num].ptp_delayreq[port], &msg->data[i5], i6 - i5 - 1);
				conf_content.slot[num].ptp_delayreq[port][2] = '\0';
				
            }
			//
			//!pdelay req fre
			//
            if(msg->data[i6] != '\0')
            {
                memcpy(conf_content.slot[num].ptp_pdelayreq[port], &msg->data[i6], len - i6);
				conf_content.slot[num].ptp_pdelayreq[port][2] = '\0';
				
            }
            //
            //!SET PGEIN MOD
            //
            sprintf((char *)sendbuf, "SET-PGEIN-MOD:%s;", msg->data);
			
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
            //	}
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("PI MOD %s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
//
//!set pge4s net parameters
//
int cmd_set_pge4v_net(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
	unsigned char  datalen;
	unsigned char  errflag;
    int num;
    int port;
    int i, j, i1, i2, i3, i4;
    int len = strlen(msg->data);
	datalen = 0;
	errflag = 0;
    j =  0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
	//
	//!Clear the snedbuf as null.
    memset(sendbuf, '\0', SENDBUFSIZE);
	
	//
	//!the slot num le 14 and ge 1.
    if(((msg->solt) > '`') && ((msg->solt) <= 'o'))
    {
        //
        //check pge port le MAX_PORT_NUM.
        //
        if((msg->data[2] > '0') && (msg->data[2] <= MAX_PORT_NUM))
        {
           //
           //Calculate the slot num and port num.
           //
            num = (int)(msg->solt - 0x61);
            port = (int)(msg->data[2] - 0x31);
           //
           //! find the conntect (mac、IP、netmask)
           //
            for(i = 0; i < len; i++)
            {
                if(msg->data[i] == ',')
                {
                    j++;
                    if(j == 2)
                    {
                        i1 = i + 1;
                    }
                    if(j == 3)
                    {
                        i2 = i + 1;
                    }
                    if(j == 4)
                    {
                        i3 = i + 1;
                    }
                    if(j == 5)
                    {
                        i4 = i + 1;
                    }
                }
            }
			//
			//if i4 equal to 0,the data frame is null。
			//
            if(i4 == 0)
            {
                return 1;
            }
			//
			//save the ptp type of slot.
			//
            conf_content.slot[num].ptp_type = msg->data[0];
			
			//
			//Judge the data len or not correct.
			//
			datalen = i2 - i1 -1; 
			if(datalen > MAX_MAC_LEN)
				errflag = 1;
			
			datalen = i3 - i2 -1; 
			if(datalen > MAX_IP_LEN)
				errflag = 1;
			
			datalen = i4 - i3 -1;
			if(datalen > MAX_IP_LEN)
				errflag = 1;

			datalen = len - i4;
			if(datalen > MAX_IP_LEN)
				errflag = 1;

			if(1 == errflag)
				return 1;
			
			//
			//save the mac parameters.
			//
            if(msg->data[i1] != ',')
            { 
				memset(conf_content.slot[num].ptp_mac[port],'\0',18);
                memcpy(conf_content.slot[num].ptp_mac[port], &msg->data[i1], i2 - i1 - 1);    
            }
			//
			//save the ip address.
			//
            if(msg->data[i2] != ',')
            {
                memset(conf_content.slot[num].ptp_ip[port],'\0',16);
                memcpy(conf_content.slot[num].ptp_ip[port], &msg->data[i2], i3 - i2 - 1);
            }
			//
			//save the mask address.
			//
            if(msg->data[i3] != ',')
            {
                memset(conf_content.slot[num].ptp_mask[port],'\0',16);
                memcpy(conf_content.slot[num].ptp_mask[port], &msg->data[i3], i4 - i3 - 1);
            }
			//
			//save the gate address.
			//
            if(msg->data[i4] != '\0')
            {
                memset(conf_content.slot[num].ptp_gate[port],'\0',16);
                memcpy(conf_content.slot[num].ptp_gate[port], &msg->data[i4], len - i4);
            }
     
            sprintf((char *)sendbuf, "SET-PGE4V-NET:%s;", msg->data);
			printf("SET-PGE4V-NET %s\n",msg->data);
            //
            // sned down to uart.
            //
			if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
           
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}

//
//! set the pge4s work mode.
//
int cmd_set_pge4v_mod(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    int num;
    int port;
    int i, j, i1, i2, i3, i4, i5, i6; /* i7, i8, i9, i10, i11, i12, i13;*/
    int len = strlen(msg->data);
	
    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
    i5 = 0;
    i6 = 0;
     /*i7 = 0;
    i8 = 0;
    i9 = 0;
    i10 = 0;
    i11 = 0;
    i12 = 0;
    i13 = 0;*/
	//
	//clear sendbuf .
	//
    memset(sendbuf, '\0', SENDBUFSIZE);
	//
	//judge slot num is le 14 .
	//
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        //
        //judge the port num is le MAX_PORT_NUM。
        //
        if((msg->data[2] > '0') && (msg->data[2] <= MAX_PORT_NUM))
        {
            //
            //!Calculate the num or port.
            //
            num = (int)(msg->solt - 0x61);
            port = (int)(msg->data[2] - 0x31);
          
            /*SET-PGE4S-MOD:<type>,<response message>;*/
			//
			//!
			//
            for(i = 0; i < len; i++)
            {
                if(msg->data[i] == ',')
                {
                    j++;
                    if(j == 2)
                    {
                        i1 = i + 1;
                    }
                    if(j == 3)
                    {
                        i2 = i + 1;
                    }
                    if(j == 4)
                    {
                        i3 = i + 1;
                    }
                    if(j == 5)
                    {
                        i4 = i + 1;
                    }
                    if(j == 6)
                    {
                        i5 = i + 1;
                    }
                    if(j == 7)
                    {
                        i6 = i + 1;
                    }
                  
                }
            }
            if(i5 == 0)
            {
                return 1;
            }
            conf_content.slot[num].ptp_type = msg->data[0];
			//
			//!save the ptp enable. 
			//
            if((msg->data[i1] == '0') || (msg->data[i1] == '1'))
            {
                conf_content.slot[num].ptp_en[port] = msg->data[i1];
            }
			//
			//!save the esmc enable.
			//
            if((msg->data[i2] == '0') || (msg->data[i2] == '1'))
            {
                conf_content3.ptp_esmcen[num][port] = msg->data[i2];
				//conf_content.slot[num].ptp_delaytype[port] = msg->data[i2];
            }
			//
			//!save the delay type.
			//
			if((msg->data[i3] == '0') || (msg->data[i3] == '1'))
            {
                conf_content.slot[num].ptp_delaytype[port] = msg->data[i3];
            }
			//
			//!save the multicast/unicast.
			//
            if((msg->data[i4] == '0') || (msg->data[i4] == '1'))
            {
                conf_content.slot[num].ptp_multicast[port] = msg->data[i4];
            }
			//
			//!save the enq .
			//
            if((msg->data[i5] == '0') || (msg->data[i5] == '1'))
            {
                conf_content.slot[num].ptp_enp[port] = msg->data[i5];
            }
			//
			//!save the step.
			//
            if((msg->data[i6] == '0') || (msg->data[i6] == '1'))
            {
                conf_content.slot[num].ptp_step[port] = msg->data[i5];
            }
            
            //
            //!send the command  down to uart.
            //
            sprintf((char *)sendbuf, "SET-PGE4V-MOD:%s;", msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
            //	}
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}

//
//!set pge4s work paramters(Extern)。
//
int cmd_set_pge4v_par(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    int num;
    int port;
    int i, j, i1, i2, i3, i4, i5;/*, i6, i7, i8, i9, i10, i11, i12, i13;*/
    int len = strlen(msg->data);
	unsigned char datalen = 0;
	unsigned errflag = 0;
    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
    i5 = 0;
    /*i6 = 0;
    i7 = 0;
    i8 = 0;
    i9 = 0;
    i10 = 0;
    i11 = 0;
    i12 = 0;
    i13 = 0;*/
    //
    //!clear send buffer.
    //
    memset(sendbuf, '\0', SENDBUFSIZE);
	//
	//judge the slot le 14 。
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        //
        //limit the port range 1 ~ 4
        if((msg->data[2] > '0') && (msg->data[2] <= MAX_PORT_NUM))
        {
            //
            //Calculate slot number and Port number。
            num = (int)(msg->solt - 0x61);
            port = (int)(msg->data[2] - 0x31);
            //
            //Calculate the paramters position。
            for(i = 0; i < len; i++)
            {
                if(msg->data[i] == ',')
                {
                    j++;
                    if(j == 2)
                    {
                        i1 = i + 1;
                    }
                    if(j == 3)
                    {
                        i2 = i + 1;
                    }
                    if(j == 4)
                    {
                        i3 = i + 1;
                    }
                    if(j == 5)
                    {
                        i4 = i + 1;
                    }
                    if(j == 6)
                    {
                        i5 = i + 1;
                    }
                   
                }
            }
            if(i5 == 0)
            {
                return 1;
            }
            //
            //judge the domain len is or not gt 3.
            //
			datalen = i2 - i1 -1;
			if(datalen > MAX_DOM_LEN)
				errflag = 1;
			//
			//judge the sync frequency len or not eq 2/0。
			//
			datalen = i3 - i2 -1;
			if((datalen != FRAME_FRE_LEN)&&(datalen != 0))
				errflag = 1;
			//
			//judge the announce frequency len or not eq 2/0。
			//
			datalen = i4 - i3 -1;
			if((datalen != FRAME_FRE_LEN)&&(datalen != 0))
				errflag = 1;
			//
			//judge the delaycon gt 7。
			//
			datalen = i5 - i4 -1;
			if((datalen > DELAY_COM_LEN))
				errflag = 1;	
			//
			//judge the prio gt 4
			datalen = len -i5;
			if((datalen > MAX_PRIO_LEN))
				errflag = 1;	
			if(1 == errflag)
				return 1;
			//
			//slot type。
			//
            conf_content.slot[num].ptp_type = msg->data[0];
			//
			//save the domain number。
			//
            if(msg->data[i1] != ',')
            {
                memset(conf_content3.ptp_dom[num][port], 0, 4);
                memcpy(conf_content3.ptp_dom[num][port], &msg->data[i1], i2 - i1 -1);
            }
			//
			//save the sync frequency 。
			//
            if(msg->data[i2] != ',')
            {
                memcpy(conf_content.slot[num].ptp_sync[port], &msg->data[i2], i3 - i2 - 1);
            }
			//
			//save the annouce frequency。
			//
            if(msg->data[i3] != ',')
            {
                memcpy(conf_content.slot[num].ptp_announce[port], &msg->data[i3], i4 - i3 - 1);
            }
			//
			//save the delaycom。
			//
            if(msg->data[i4] != ',')
            {
                memset(conf_content.slot[num].ptp_delaycom[port],'\0',9);
                memcpy(conf_content.slot[num].ptp_delaycom[port], &msg->data[i4], i5 - i4 - 1);
            }
			//
			//save the pro。
			//
            if(msg->data[i5] != '\0')
            {
                if(msg->data[i5] == 'A')
                {
                    memset(conf_content3.ptp_prio1[num][port], 0, 5);
                    memcpy(conf_content3.ptp_prio1[num][port], &msg->data[i5], len - i5);
                    
                }
                else if(msg->data[i5] == 'B')
                {
                    memset(conf_content3.ptp_prio2[num][port], 0, 5);
                    memcpy(conf_content3.ptp_prio2[num][port], &msg->data[i5], len - i5);
                }
                else
                {
                    return 1;
                }
            }
            
            sprintf((char *)sendbuf, "SET-PGE4V-PAR:%s;", msg->data);
			printf("SET-PGE4V-PAR %s\n",msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
            
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
//
//! set pge4s mtc.
//
int cmd_set_pge4v_mtc(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    int num;
    int port;
	
    memset(sendbuf, '\0', SENDBUFSIZE);
    //
    //slot num.
    //
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        if((msg->data[2] > '0') && (msg->data[2] <= MAX_PORT_NUM))
        {
            num = (int)(msg->solt - 0x61);
            port = (int)(msg->data[2] - 0x31);
            if(msg->data[4] != '\0')
            {
                memset(conf_content3.ptp_mtc_ip[num][port], 0, 9);
                memcpy(conf_content3.ptp_mtc_ip[num][port], &msg->data[4], MAX_IP_LEN);


                sprintf((char *)sendbuf, "SET-PGE4V-MTC:%s;", msg->data);

                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            return 0;

        }
		return 1;
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
}


//
//!set pge4s work sfp modual(Extern)。
//
int cmd_set_pge4v_sfp(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    int num;
    int port;
    int i, j, i1, i2, i3;/*, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13;*/
    int len = strlen(msg->data);
	unsigned char datalen = 0;
	unsigned errflag = 0;
    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    //
    //!clear send buffer.
    //
    memset(sendbuf, '\0', SENDBUFSIZE);
	//
	//judge the slot le 14 。
    if(((msg->solt) > '`') && ((msg->solt) < 'o'))
    {
        //
        //limit the port range 1 ~ 4
        if((msg->data[2] > '0') && (msg->data[2] <= MAX_PORT_NUM))
        {
            //
            //Calculate slot number and Port number。
            num = (int)(msg->solt - 0x61);
            port = (int)(msg->data[2] - 0x31);
            //
            //Calculate the paramters position。
            for(i = 0; i < len; i++)
            {
                if(msg->data[i] == ',')
                {
                    j++;
                    if(j == 2)
                    {
                        i1 = i + 1;
                    }
                    if(j == 3)
                    {
                        i2 = i + 1;
                    }
                    if(j == 4)
                    {
                        i3 = i + 1;
                    } 
                }
            }
            if(i3 == 0)
            {
                return 1;
            }
            //
            //judge the sfp modual setting len is or not gt 3.
            //
			datalen = i2 - i1 -1;
			if((datalen != 2)&&(datalen != 0))
				errflag = 1;
	
			if(1 == errflag)
				return 1;
			//
			//slot type。
			//
            conf_content.slot[num].ptp_type = msg->data[0];
			//
			//save the domain number。
			//
            if(msg->data[i1] != ',')
            {
                memset(conf_content3.ptp_sfp[num][port], 0, 3);
                memcpy(conf_content3.ptp_sfp[num][port], &msg->data[i1], i2 - i1 -1);
            }
			//
			//save the sfp modul op low thresh 。
			//
            if(msg->data[i2] != ',')
            {
                memset(conf_content3.ptp_oplo[num][port], 0, 7);
                memcpy(conf_content3.ptp_oplo[num][port], &msg->data[i2], i3 - i2 - 1);
            }
			//
			//save the sfp modul op hi thresh 。
			//
            if(msg->data[i3] != '\0')
            {
                memset(conf_content3.ptp_ophi[num][port], 0, 7);
                memcpy(conf_content3.ptp_ophi[num][port], &msg->data[i2], len - i3);
            }
			//
			//send down to uart 。
            sprintf((char *)sendbuf, "SET-PGE4V-SFP:%s;", msg->data); 
			printf("SET-PGE4V-SFP %s\n",msg->data);
            if(flag)
            {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
            }
            else
            {
                sendtodown(sendbuf, msg->solt);
            }
            
        }
    }
    else
    {
        return 1;
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}

/*
*********************************************************************************************************
*	Function: int cmd_set_gb_mode(_MSG_NODE *msg, int flag)
*	Descript: gbtpv2 work mode setting.
*	Parameters:  - msg  :the point of input data.
*                - flag :1:NMG SETTING; 0:Init Setting.

*	Return:      - 0 : ok
*				 - 1 : error
*				
*********************************************************************************************************
*/
int cmd_set_gb_mode(_MSG_NODE *msg, int flag)
{
    int len;
	
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    len = strlen(msg->data);
	
	if((strncmp1(msg->data,"GPS",3) != 0)&&(strncmp1(msg->data,"BD",3) != 0)&&(strncmp1(msg->data,"MIX",3) != 0))
	 return 1;
	
    if(msg->solt == GBTP1_SLOT)
    {
        memset(conf_content.slot_q.mode, '\0', 4);
        memcpy(conf_content.slot_q.mode, msg->data, len);  
    }
    else if(msg->solt == GBTP2_SLOT)
    {
        memset(conf_content.slot_r.mode, '\0', 4);
        memcpy(conf_content.slot_r.mode, msg->data, len);
    }
    else
    {
        return 1;
    }

	sprintf((char *)sendbuf, "SET-GBTP-MODE:%s::%s;", msg->ctag, msg->data);
	
    if(flag)
    {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
    }
    else
    {
            sendtodown(sendbuf, msg->solt);
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}

/*
*********************************************************************************************************
*	Function: int cmd_set_gn_mode(_MSG_NODE *msg, int flag)
*	Descript: gbtpv2 work mode setting.
*	Parameters:  - msg  :the point of input data.
*                - flag :1:NMG SETTING; 0:Init Setting.

*	Return:      - 0 : ok
*				 - 1 : error
*				
*********************************************************************************************************
*/
int cmd_set_gn_mode(_MSG_NODE *msg, int flag)
{
    int len;
	
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    len = strlen(msg->data);
	
	if((strncmp1(msg->data,"GPS",3) != 0)&&(strncmp1(msg->data,"BD",3) != 0)&&(strncmp1(msg->data,"MIX",3) != 0)
		&&(strncmp1(msg->data,"GLO",3)!= 0)&&(strncmp1(msg->data,"GAL",3)!= 0))
	 	return 1;
	
    if(msg->solt == GBTP1_SLOT)
    {
        memset(conf_content.slot_q.mode, '\0', 4);
        memcpy(conf_content.slot_q.mode, msg->data, len);  
    }
    else if(msg->solt == GBTP2_SLOT)
    {
        memset(conf_content.slot_r.mode, '\0', 4);
        memcpy(conf_content.slot_r.mode, msg->data, len);
    }
    else
    {
        return 1;
    }

	sprintf((char *)sendbuf, "SET-GBTP-MODE:%s::%s;", msg->ctag, msg->data);
	
    if(flag)
    {
            sendtodown_cli(sendbuf, msg->solt, msg->ctag);
            save_config();
    }
    else
    {
            sendtodown(sendbuf, msg->solt);
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}

/*
*********************************************************************************************************
*	Function: int cmd_set_gb_en(_MSG_NODE *msg, int flag)
*	Descript: gbtpv2 work mode setting.
*	Parameters:  - msg  :the point of input data.
*                - flag :1:NMG SETTING; 0:Init Setting.

*	Return:      - 0 : ok
*				 - 1 : error
*				
*********************************************************************************************************
*/
int cmd_set_gb_en(_MSG_NODE *msg, int flag)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);
    if((msg->data[0] != '0') && (msg->data[0] != '1'))
    {
        return 1;
    }
    if(msg->solt == GBTP1_SLOT)
    {
        
            memset(conf_content.slot_q.mask, '\0', 3);
            memcpy(conf_content.slot_q.mask, msg->data, 2);
    }
    else if(msg->solt == GBTP2_SLOT)
    {
            memset(conf_content.slot_r.mask, '\0', 3);
            memcpy(conf_content.slot_r.mask, msg->data, 2);
    }
    else
    {
        return 1;
    }
	
	sprintf((char *)sendbuf, "SET-GBTP-SATEN:%s::%s;", msg->ctag, msg->data);
	if(flag)
    {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
    }
    else
    {
                sendtodown(sendbuf, msg->solt);
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
/*
*********************************************************************************************************
*	Function: int cmd_set_gn_dly(_MSG_NODE *msg, int flag)
*	Descript: gbtpv2 work mode setting.
*	Parameters:  - msg  :the point of input data.
*                - flag :1:NMG SETTING; 0:Init Setting.

*	Return:      - 0 : ok
*				 - 1 : error
*				
*********************************************************************************************************
*/
int is_digit1(unsigned char *parg,unsigned char len)
{
	int i;
	int usError;
	usError = 0;
	
	for(i = 0; i < len; i++)
	{
		if(i == 0)
		{
		   if(parg[0] != '-')
		   {
				if(parg[0] < 0x30 || parg[0] > 0x39)
				{
					usError = 1;
						break;
				}
		   }
		}
		else if(parg[i] < 0x30 || parg[i] > 0x39)
		{
				usError = 1;
				break;
		}
	}
	return usError;
	
}

int cmd_set_gn_dly(_MSG_NODE *msg, int flag)
{
    int len, tmp_value;
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);

    len = strlen(msg->data);
    /*set delay rang -32768 ~ 32767*/
	if(len <= 0 || len > 6)
		return 1;
    /*judge the ascii (0 ~ 9)*/
	if(is_digit1(msg->data, len))
	    return 1;

	/*judge delay rang -32768 ~ 32767*/
    tmp_value = atoi(msg->data);
	if(tmp_value < -32768 || tmp_value > 32767)
		return 1;

    if(msg->solt == GBTP1_SLOT)
    {
            memcpy(conf_content3.gbtp_delay[0], msg->data, len);
	        conf_content3.gbtp_delay[0][len] = '\0';
    }
    else if(msg->solt == GBTP2_SLOT)
    {
            memcpy(conf_content3.gbtp_delay[1], msg->data, len);
	        conf_content3.gbtp_delay[1][len] = '\0';
    }
    else
    {
        return 1;
    }
	
	sprintf((char *)sendbuf, "SET-GBTP-DELAY:%s::%s;", msg->ctag, msg->data);
	if(flag)
    {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
    }
    else
    {
                sendtodown(sendbuf, msg->solt);
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}
/*GBTPIII 1PPS OUT DELAY,RANGE OF -400000~ 400000,UNIT 2.5ns*/
int cmd_set_pps_dly(_MSG_NODE *msg, int flag)
{
    int len, tmp_value;
    unsigned char  sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);

    len = strlen(msg->data);
    /*set delay rang -400000 ~ 400000*/
	if(len <= 0 || len > 10)
		return 1;
    /*judge the ascii (0 ~ 9)*/
	if(is_digit1(msg->data, len))
	    return 1;

	/*judge delay rang -200,000,000 ~ 200,000,000*/
    tmp_value = atoi(msg->data);
	if(tmp_value < -200000000 || tmp_value > 200000000)
		return 1;

    if(msg->solt == GBTP1_SLOT)
    {
            memcpy(conf_content3.gbtp_pps_delay[0], msg->data, len);
	        conf_content3.gbtp_pps_delay[0][len] = '\0';
    }
    else if(msg->solt == GBTP2_SLOT)
    {
            memcpy(conf_content3.gbtp_pps_delay[1], msg->data, len);
	        conf_content3.gbtp_pps_delay[1][len] = '\0';
    }
    else
    {
        return 1;
    }
	
	sprintf((char *)sendbuf, "SET-PPS-DELAY:%s::%s;", msg->ctag, msg->data);
	if(flag)
    {
                sendtodown_cli(sendbuf, msg->solt, msg->ctag);
                save_config();
    }
    else
    {
                sendtodown(sendbuf, msg->solt);
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s\n", NET_INFO, sendbuf);
    }
#endif
    return 0;
}

/*
2015-9-9
*/
int cmd_set_protocol(_MSG_NODE *msg, int flag)
{

    unsigned char sendbuf[SENDBUFSIZE];
	unsigned char i;
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);

	
    if((msg->data[0] != '0') && (msg->data[0] != '1') && (msg->data[0] != '2'))
    {
        return 1;
    }
	else
	{
		conf_content3.mcp_protocol = msg->data[0];
	}
	printf("### cmd_set_protocol:%c\n",msg->data[0]);
	sprintf((char *)sendbuf, "SET-PROTOCOL:%s::%c;", msg->ctag, msg->data[0]);
	for(i=0;i<13;i++)
	{
		printf("### %d--type:%c\n",i,slot_type[i]);
		if( 'I' == slot_type[i] || 	'j' == slot_type[i]  ||						//TP16 PGE4V2
			(slot_type[i] >= 'A' && slot_type[i] <= 'H') || //PTP4\PFO4\PGE4\PGO4\PTP8\PFO8\PGE8\PGO8
			(slot_type[i] >= 'n' && slot_type[i] <= 'q')  )  //PTP\PGE\PTP2\PGE2
		{
			printf("### SENDTODOWN:%s\n",sendbuf);
			if(flag)
			{
				sendtodown_cli(sendbuf, (char)('a'+i), msg->ctag);
			}
			else
			{
				sendtodown(sendbuf, (char)('a'+i));
			}
			
		}
		else
		{
			continue;
		}
	}
	
	save_config();
    respond_success(current_client, msg->ctag);
    return 0;
}

int cmd_set_ppssel(_MSG_NODE *msg, int flag)
{

    unsigned char sendbuf[SENDBUFSIZE];
	//unsigned char i;
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);

	
    if((msg->data[0] < '0') || (msg->data[0] > '6'))
    {
        return 1;
    }
	
	if(msg->solt == RB1_SLOT)
    {
            conf_content3.pps_sel[0] = msg->data[0];   
    }
    else if(msg->solt == RB2_SLOT)
    {
         
            conf_content3.pps_sel[1] = msg->data[0];
    }
    else
    {
        return 1;
    }

	sprintf((char *)sendbuf, "SET-PPS-SEL:%s::%s;", msg->ctag, msg->data);
	if(flag)
    {
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        save_config();
    }
    else
    {
        sendtodown(sendbuf, msg->solt);
    }
	
    respond_success(current_client, msg->ctag);
    return 0;
}
int cmd_set_tdeven(_MSG_NODE *msg, int flag)
{

    unsigned char sendbuf[SENDBUFSIZE];
	//unsigned char i;
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);

	
    if((msg->data[0] !='0') && (msg->data[0] != '1'))
    {
        return 1;
    }
	
	if(msg->solt == RB1_SLOT)
    {
            conf_content3.tdev_en[0] = msg->data[0];   
    }
    else if(msg->solt == RB2_SLOT)
    {
         
            conf_content3.tdev_en[1] = msg->data[0];
    }
    else
    {
        return 1;
    }

	sprintf((char *)sendbuf, "SET-TDEV-EN:%s::%s;", msg->ctag, msg->data);
	if(flag)
    {
        sendtodown_cli(sendbuf, msg->solt, msg->ctag);
        save_config();
    }
    else
    {
        sendtodown(sendbuf, msg->solt);
    }
	
    respond_success(current_client, msg->ctag);
    return 0;
}

_CMD_NODE g_cmd_fun[] =
{
    //扩展框
    {"QEXT-1-EQT", 10, NULL, cmd_ext1_eqt},
    {"QEXT-2-EQT", 10, NULL, cmd_ext2_eqt},
    {"QEXT-3-EQT", 10, NULL, cmd_ext3_eqt},
    {"QEXT-EQT",8,NULL,cmd_ext_eqt},

    {"QEXT-1-ALM", 10, NULL, cmd_ext1_alm},
    {"QEXT-2-ALM", 10, NULL, cmd_ext2_alm},
    {"QEXT-3-ALM", 10, NULL, cmd_ext3_alm},
    {"QEXT-ALM", 	8, NULL, cmd_ext_alm},

    {"QEXT-1-PRS", 10, NULL, cmd_ext1_prs},
    {"QEXT-2-PRS", 10, NULL, cmd_ext2_prs},
    {"QEXT-3-PRS", 10, NULL, cmd_ext3_prs},

    {"QEXT-DRV-MGR", 12, NULL, cmd_drv_mgr},

    {"QEXT-ONLINE", 11, NULL, cmd_qext_online},
    {"SET-EXT-ONLINE", 14, cmd_set_ext_online, NULL},

    {"RTRV-ALRMS", 10, NULL, cmd_rtrv_alrms},
    {"RTRV-EQPT", 9, NULL, cmd_rtrv_eqpt},
    {"RTRV-PM-INP", 11, NULL, cmd_rtrv_pm_inp},
    {"RTRV-PM-MSG", 11, NULL, cmd_rtrv_pm_msg},
    {"RTRV-OUT", 8, NULL, cmd_rtrv_out},
    {"RTRV-MTC-IP", 11, NULL, cmd_rtrv_mtc_ip},
    {"RTRV-UNI-IP", 11, NULL, cmd_rtrv_uni_ip},
    {"RTRV-MTC-DOM", 12, NULL, cmd_rtrv_mtc_dom},
    {"SET-TID", 7, cmd_set_tid, NULL},

    {"HAND-SHAKE", 10, cmd_hand_shake, NULL},
    {"SET-MAIN", 8, cmd_set_main, NULL},
    {"SET-DAT_S", 9, cmd_set_dat_s, NULL},
    {"SET-DAT", 7, cmd_set_dat, NULL},
    {"SET-NET", 7, cmd_set_net, NULL},
    {"SET-SSM-OEN", 11, cmd_set_out_ssm_en, NULL},
    {"SET-SSM-OTH", 11, cmd_set_out_ssm_oth, NULL},

    {"SET-MAC", 7, cmd_set_mac, NULL},
    {"ED-USER-SECU", 12, cmd_set_password, NULL},
    {"SET-MODE", 8, cmd_set_mode, NULL},
    {"SET-POS", 7, cmd_set_pos, NULL},
    {"SET-MASK-GBTP", 13, cmd_mask_gbtp, NULL},

    {"SET-BDZB", 8, cmd_bdzb, NULL},

    {"SET-SYS-REF", 11, cmd_sys_ref, NULL},
    {"SET-DELY", 8, cmd_set_dely, NULL},
    {"SET-PRIO-INP", 12, cmd_set_prio_inp, NULL},
    {"SET-MASK-E1", 11, cmd_set_mask_e1, NULL},
    {"SET-LEAP-MASK", 13, cmd_set_leap_mask, NULL},

    {"SET-TZO", 7, cmd_set_tzo, NULL},
    {"SET-PTP-NET", 11, cmd_set_ptp_net, NULL},
    {"SET-PTP-MOD", 11, cmd_set_ptp_mod, NULL},
    {"SET-PTP-MTC", 11, cmd_set_ptp_mtc, NULL},
    {"SET-PTP-DOM",11, cmd_set_ptp_dom,NULL},//添加PTP时钟域号设置。
   
    {"SET-OUT-LEV", 11, cmd_set_out_lev, NULL},
    {"SET-OUT-TYP", 11, cmd_set_out_typ, NULL},
    {"SET-OUT-PRR", 11, cmd_set_out_ppr, NULL},

    {"SET-RESET", 9, cmd_set_reset, NULL},
    {"SET-LEAP-TAG", 12, cmd_set_leap_tag, NULL},
    {"SET-TP16-EN", 11, cmd_set_tp16_en, NULL},
    {"SET-CONFIG", 10, cmd_set_CONFIG, NULL},
    {"SET-LEAP-NUM", 12, cmd_set_leap_num, NULL},

    {"SET-REF-PRIO", 12, cmd_set_ref_prio, NULL},
    {"SET-REF-MOD", 11, cmd_set_ref_mod, NULL},
    {"SET-REF-SA", 10, cmd_set_ref_sa, NULL},
    {"SET-REF-PH", 10, cmd_set_ref_ph, NULL},
    {"SET-REF-TLB", 11, cmd_set_ref_tlb, NULL},//增加2Mb信号等级设置，在2Mb设置为网管预设值时有效。
    {"SET-REF-TLM", 11, cmd_set_ref_tlm, NULL},//增加2Mbit信号获取信号等级来源。
    {"SET-REF-TL", 10, cmd_set_ref_tl, NULL},
    {"SET-REF-IEN", 11, cmd_set_ref_ssm_en, NULL},

    {"SET-RS-EN", 9, cmd_set_rs_en, NULL},
    {"SET-RS-TZO", 10, cmd_set_rs_tzo, NULL},
    {"SET-PPX-MOD", 11, cmd_set_ppx_mod, NULL},
    {"SET-TOD-EN", 10, cmd_set_tod_en, NULL},
    {"SET-TOD-PRO", 11, cmd_set_tod_pro, NULL},

    {"SET-IGB-EN", 10, cmd_set_igb_en, NULL},
    {"SET-IGB-RAT", 11, cmd_set_igb_rat, NULL},
    {"SET-IGB-MAX", 11, cmd_set_igb_max, NULL},
    //2015527 set IRIGR16 timezone
    {"SET-IGB-TZONE", 13, cmd_set_igb_tzone, NULL},
    {"SET-NTP-EN", 10, cmd_set_ntp_en, NULL},
    {"SET-NTP-NET", 11, cmd_set_ntp_net, NULL},
    {"SET-NTP-PNET", 12, cmd_set_ntp_pnet, NULL},//new add
    {"SET-NTP-PMAC", 12, cmd_set_ntp_pmac, NULL},//2017/6/9 (new add)
    {"SET-NTP-PEN", 11, cmd_set_ntp_pen, NULL},	//new add
    {"SET-NTP-KEY", 11, cmd_set_ntp_key, NULL},
    {"SET-NTP-MS", 10, cmd_set_ntp_ms, NULL},
    {"SET-MCP-FB", 10, cmd_set_mcp_fb, NULL},
    {"SET-RB-SA", 9, cmd_set_rb_sa, NULL},
    {"SET-RB-TL", 9, cmd_set_rb_tl, NULL},
     {"SET-RB-TH", 9, cmd_set_rb_thresh, NULL},//设置钟盘IRIGB设置门限

    {"SET-OUT-MOD", 11, cmd_set_out_mod, NULL},
    //设置告警屏蔽
    {"SET-ARLM-MASK", 13, cmd_set_alm_msk, NULL},
    {"RTRV-RALM-MASK", 14, NULL, cmd_rtrv_ralm_mask},
    //20141210
    {"SET-LEAP-MOD", 12, cmd_set_leap_mod, NULL},
    {"RTRV-LEAP-MOD", 13, NULL, cmd_rtrv_leap_mod},
    //20150527 查询irigb16时区
    {"RTRV-IGB-TZONE", 13, NULL, cmd_rtrv_igb_tzone},
    {"SYS-CPU", 7, NULL, cmd_sys_cpu},
    //STA-1
    {"SET-STA1-NET", 12, cmd_set_sta_net, NULL},
    //PGEIN
    {"SET-PGEIN-NET", 13, cmd_set_pgein_net, NULL},
    {"SET-PGEIN-MOD", 13, cmd_set_pgein_mod, NULL},
	//PGE4S SET-PGE4S-NET
	{"SET-PGE4V-NET", 13, cmd_set_pge4v_net, NULL},
	{"SET-PGE4V-MOD", 13, cmd_set_pge4v_mod, NULL},
	{"SET-PGE4V-PAR", 13, cmd_set_pge4v_par, NULL},
	{"SET-PGE4V-MTC", 13, cmd_set_pge4v_mtc, NULL},
	{"SET-PGE4V-SFP", 13, cmd_set_pge4v_sfp, NULL},
	//GBTPIIV2 SETTING
    {"SET-GB-MODE", 11, cmd_set_gb_mode, NULL},
    {"SET-GB-EN", 9,    cmd_set_gb_en, NULL},
    //GBTPIII SETTING
    {"SET-GN-MODE", 11, cmd_set_gn_mode, NULL},
    {"SET-GN-EN", 9,    cmd_set_gb_en, NULL},
    {"SET-GN-DLY", 10,  cmd_set_gn_dly, NULL},
    {"SET-PPS-DLY", 11, cmd_set_pps_dly, NULL},
    
     //2015-9-9  联通移动版本选择
    {"SET-PROTOCOL", 12, cmd_set_protocol, NULL},
    {"SET-PPS-SEL", 11, cmd_set_ppssel, NULL},
    {"SET-TDEV-EN", 11, cmd_set_tdeven, NULL},
    
};
int _CMD_NODE_MAX = ((int)(sizeof(g_cmd_fun) / sizeof(_CMD_NODE))); //2014.12.5

