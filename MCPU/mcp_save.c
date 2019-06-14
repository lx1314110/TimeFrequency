#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/msg.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <ctype.h>

#include "ext_global.h"
#include "ext_crpt.h"
#include "ext_trpt.h"

#include "mcp_main.h"
#include "mcp_process.h"

#include "mcp_def.h"
#include "mcp_save_struct.h"

#include "mcp_set_struct.h"
//#include "memwatch.h"


//#define ErrorOK  0


/*
*********************************************************************************************************
*	Function: int save_gb_rcv_mode(char slot, unsigned char *parg)
*	Descript: gbtpv2 alarm infomation.
*	Parameters:  - slot 
*                - parg :the point of input data.

*	Return:      - 0 : ok
*				 - 1 : error
*				
*********************************************************************************************************
*/

int save_gb_rcv_mode(char slot, unsigned char *parg)
{
    /*<acmode>*/
    //int len;

    if(slot == GBTP1_SLOT)
    {
        memset(rpt_content.slot_q.gb_acmode, 0, 4);
        memcpy(rpt_content.slot_q.gb_acmode, parg, strlen(parg));
    }
    else if(slot == GBTP2_SLOT)
    {
        memset(rpt_content.slot_r.gb_acmode, 0, 4);
        memcpy(rpt_content.slot_r.gb_acmode, parg, strlen(parg));
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif

    return 0;
}

int save_gb_work_mode(char slot, unsigned char *parg)
{
    /*<pos_mode>*/
    //int len;
    if(slot == GBTP1_SLOT)
    {
        memcpy(rpt_content.slot_q.gb_pos_mode, parg, 4);

    }
    else if(slot == GBTP2_SLOT)
    {
        memcpy(rpt_content.slot_r.gb_pos_mode, parg, 4);
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}

/*
*********************************************************************************************************
*	Function: int save_gb_mask(char slot, unsigned char *parg)
*	Descript: gbtpv2 enble setting event.
*	Parameters:  - slot 
*                - parg :the point of input data.

*	Return:      - 0 : ok
*				 - 1 : error
*				
*********************************************************************************************************
*/

int save_gb_mask(char slot, unsigned char *parg)
{
    int num;//, i, len
    num = (int)(slot - 'a');
    
    if((parg[0]!='0') && (parg[0]!='1'))

	   return 1;
	
    if(slot == GBTP1_SLOT)
    {
        if(slot_type[num] != 'v')
        {
           memcpy(rpt_content.slot_q.gb_mask, parg, 2);
		   rpt_content.slot_q.gb_mask[2] = '\0';
		   
        }
		else 
		{
		   rpt_content.slot_q.staen = parg[0];
		   //rpt_content.slot_q.gb_mask[0] = parg[0];
		   //rpt_content.slot_q.gb_mask[1] = '\0';
		}
			
    }
    else if(slot == GBTP2_SLOT)
    {
        if(slot_type[num] != 'v')
        {
        	memcpy(rpt_content.slot_r.gb_mask, parg, 2);
			rpt_content.slot_r.gb_mask[2] = '\0';
        }
		else 
		{
		   rpt_content.slot_r.staen = parg[0];
		   //rpt_content.slot_r.gb_mask[0] = parg[0];
		   //rpt_content.slot_r.gb_mask[1] = '\0';
		}
		
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}

int save_gb_bdzb(char slot, unsigned char *parg)
{
    int len = 0;
    if((strncmp1(parg, "BJ54", 4) != 0) && (strncmp1(parg, "CGS2000", 7) != 0))
    {
        return 1;
    }
    len = strlen(parg);
    if(slot == GBTP1_SLOT)
    {
        memcpy(rpt_content.slot_q.bdzb, parg, len	);
    }
    else if(slot == GBTP2_SLOT)
    {
        memcpy(rpt_content.slot_r.bdzb, parg, len);
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}

int save_rb_msmode(char slot, unsigned char *parg)
{
    unsigned char buf[4];
    /*<msmode>::=MAIN|STAN*/
    if(slot == RB1_SLOT)
    {
        memcpy(rpt_content.slot_o.rb_msmode, parg, 4);
        get_out_lev(0, 0);
        memcpy(buf, rpt_content.slot_o.tod_buf, 3);
        //printf("<save_rb_msmode>:%s\n",buf);
        send_tod(buf, 'o');
    }
    else if(slot == RB2_SLOT)
    {
        memcpy(rpt_content.slot_p.rb_msmode, parg, 4);
        get_out_lev(0, 0);
        memcpy(buf, rpt_content.slot_p.tod_buf, 3);
        //printf("<save_rb_msmode>:%s\n",buf);
        send_tod(buf, 'p');
    }
#if DEBUG_NET_INFO
    if(slot == RB1_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_o.rb_msmode);
        }
    }
    else if(slot == RB2_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_p.rb_msmode);
        }
    }
#endif

    return 0;
}

int save_rb_src(char slot, unsigned char *parg)
{
    /*<ref>::= y */
    //int flag_lev=0;
    //int i;
    if(slot == RB1_SLOT)
    {
        rpt_content.slot_o.rb_ref = parg[0];
        get_out_lev(0, 0);
    }
    else if(slot == RB2_SLOT)
    {
        rpt_content.slot_p.rb_ref = parg[0];
        get_out_lev(0, 0);
    }
    /*#if DEBUG_NET_INFO
    	if(slot==RB1_SLOT)
    	{
    		printf("%s:<%c>%s#\n",NET_SAVE,slot,rpt_content.slot_o.rb_ref);
    	}
    	else if(slot==RB2_SLOT)
    	{
    		printf("%s:<%c>%s#\n",NET_SAVE,slot,rpt_content.slot_p.rb_ref);
    	}
    #endif*/
    return 0;
}

int save_rb_prio(char slot, unsigned char *parg)
{
    /*<priority>::=xxxxxxxx*/
    if(slot == RB1_SLOT)
    {
        memcpy(rpt_content.slot_o.rb_prio, parg, 8);
    }
    else if(slot == RB2_SLOT)
    {
        memcpy(rpt_content.slot_p.rb_prio, parg, 8);
    }

#if DEBUG_NET_INFO
    if(slot == RB1_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_o.rb_prio);
        }
    }
    else if(slot == RB2_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_p.rb_prio);
        }
    }
#endif

    return 0;
}

int save_rb_mask(char slot, unsigned char *parg)
{
    /*<mask>::=xxxxxxxx*/
    if(slot == RB1_SLOT)
    {
        memcpy(rpt_content.slot_o.rb_mask, parg, 8);
    }
    else if(slot == RB2_SLOT)
    {
        memcpy(rpt_content.slot_p.rb_mask, parg, 8);
    }
#if DEBUG_NET_INFO
    if(slot == RB1_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_o.rb_mask);
        }
    }
    else if(slot == RB2_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_p.rb_mask);
        }
    }
#endif

    return 0;
}

int save_rf_tzo(char slot, unsigned char *parg)
{
    /*<we>,<out>*/
    if(slot == RB1_SLOT)
    {
        memcpy(rpt_content.slot_o.rf_tzo, parg, 3);
    }
    else if(slot == RB2_SLOT)
    {
        memcpy(rpt_content.slot_p.rf_tzo, parg, 3);
    }
#if DEBUG_NET_INFO
    if(slot == RB1_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_o.rf_tzo);
        }
    }
    else if(slot == RB2_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_p.rf_tzo);
        }
    }
#endif

    return 0;
}

int save_rb_dey_chg(char slot, unsigned char *parg)
{
    /**/
    int num;
    num = (int)(parg[0] - 0x31);
    if(slot == RB1_SLOT)
    {
    	memset(rpt_content.slot_o.rf_dey[num], 0, 9);
        memcpy(rpt_content.slot_o.rf_dey[num], &parg[2], strlen(&parg[2]));
    }
    else if(slot == RB2_SLOT)
    {
    	memset(rpt_content.slot_p.rf_dey[num], 0, 9);
        memcpy(rpt_content.slot_p.rf_dey[num], &parg[2], strlen(&parg[2]));
    }
#if DEBUG_NET_INFO
    if(slot == RB1_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_o.rf_dey[num]);
        }
    }
    else if(slot == RB2_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_p.rf_dey[num]);
        }
    }
#endif

    return 0;
}

int save_rb_sta_chg(char slot, unsigned char *parg)
{
    /*<trmode>::=TRCK|HOLD|FATR|TEST|FREE*/
    if(slot == RB1_SLOT)
    {
        memcpy(rpt_content.slot_o.rb_trmode, parg, 4);
        get_out_lev(0, 0);
    }
    else if(slot == RB2_SLOT)
    {
        memcpy(rpt_content.slot_p.rb_trmode, parg, 4);
        get_out_lev(0, 0);
    }
#if DEBUG_NET_INFO
    if(slot == RB1_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_o.rb_trmode);
        }
    }
    else if(slot == RB2_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_p.rb_trmode);
        }
    }
#endif

    return 0;
}

int save_rb_leap_chg(char slot, unsigned char *parg)
{
    /*<leap>::=xx*/
    if(slot == RB1_SLOT)
    {
        memcpy(rpt_content.slot_o.rb_leap, parg, 2);
    }
    else if(slot == RB2_SLOT)
    {
        memcpy(rpt_content.slot_p.rb_leap, parg, 2);
    }
#if DEBUG_NET_INFO
    if(slot == RB1_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_o.rb_leap);
        }
    }
    else if(slot == RB2_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_o.rb_leap);
        }
    }
#endif
    return 0;
}

int save_rb_leap_flg(char slot, unsigned char *parg)
{

    /*<leapmask>::=xxxxx*/
    if(slot == RB1_SLOT)
    {
        memcpy(rpt_content.slot_o.rb_leapmask, parg, 5);
    }
    else if(slot == RB2_SLOT)
    {
        memcpy(rpt_content.slot_p.rb_leapmask, parg, 5);
    }
#if DEBUG_NET_INFO
    if(slot == RB1_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_o.rb_leapmask);
        }
    }
    else if(slot == RB2_SLOT)
    {
        if(print_switch == 0)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, rpt_content.slot_p.rb_leapmask);
        }
    }
#endif

    return 0;
}

int save_tod_sta(char slot, unsigned char *parg)
{
    int num;
    //	unsigned char tmp[17];
    //	memset(tmp,'\0',sizeof(unsigned char )*17);
    num = (int)(slot - 'a');
    if(slot_type[num] != 'I')
    {
        //
        send_online_framing(num, 'I');
        slot_type[num] = 'I';
    }

    memcpy(rpt_content.slot[num].tp16_tod, parg, 3);
    memcpy(rpt_content.slot[num].tp16_en, &parg[4], 16);

    //rpt_content.slot[num].tp16_en[16]='\0';
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    //printf("<save_tod_sta>:%s\n",parg);
    return 0;
}



int save_out_lev(char slot, unsigned char *parg)
{
    int num, len;

    if(slot >= 'a' && slot <= 'n')
    {
        num = (int)(slot - 'a');
        len = strlen(parg);
        if(2 == len)
        {
            memcpy(rpt_content.slot[num].out_lev, parg, 2);
        }
        else
        {
            return(1);
        }
    }
    else
    {
        if(2 == strlen(parg))
        {
            if(0 == ext_out_ssm_crpt(0, slot, parg, &gExtCtx))
            {
                return 1;
            }
        }
        else
        {
            if(0 == ext_out_ssm_trpt(slot, parg, &gExtCtx))
            {
                return 1;
            }
        }
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif

    return 0;
}


//
//
//
int save_out_typ(char slot, unsigned char *parg)
{
    int num, len;

    if(slot >= 'a' && slot <= 'n')
    {
        num = (int)(slot - 'a');
		
        len = strlen(parg);
        if(16 == len)
        {
            memcpy(rpt_content.slot[num].out_typ, parg, 16);
        }
        else
        {
            return(1);
        }
    }
    else
    {
        if(16 == strlen(parg))
        {
            if(0 == ext_out_signal_crpt(0, slot, parg, &gExtCtx))
            {
                return 1;
            }
        }
        else
        {
            if(0 == ext_out_signal_trpt(slot, parg, &gExtCtx))
            {
                return 1;
            }
        }
    }
	//
	//!OUT32S ADD.
	//
	//printf("out32s type change:%d,%s#\n", num,parg);

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif

    return 0;
}



int save_ptp_net(char slot, unsigned char *parg)
{
    int num, port;
    int i, j, i1, i2, i3, i4, i5, i6, len;
    j = i1 = i2 = i3 = i4 = i5 = i6 = 0;
    num = (int)(slot - 'a');
    port = (int)(parg[0] - 0x31);
    len = strlen(parg);
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
            if(j == 5)
            {
                i5 = i + 1;
            }
            if(j == 6)
            {
                i6 = i + 1;
            }
        }
    }
    if(i6 == 0)
    {
        return (1);
    }
    //memcpy(rpt_content.slot[num].ptp_net[port],parg,99);
    //	flag=i2-i1-1;
    //if(flag!=0)
    if(parg[i1] != ',')
    {
        memcpy(rpt_content.slot[num].ptp_ip[port], &parg[i1], i2 - i1 - 1);
    }

    if(parg[i2] != ',')
    {
        memcpy(rpt_content.slot[num].ptp_mac[port], &parg[i2], i3 - i2 - 1);
    }

    if(parg[i3] != ',')
    {
        memcpy(rpt_content.slot[num].ptp_gate[port], &parg[i3], i4 - i3 - 1);
    }

    if(parg[i4] != ',')
    {
        memcpy(rpt_content.slot[num].ptp_mask[port], &parg[i4], i5 - i4 - 1);
    }

    if(parg[i5] != ',')
    {
        memcpy(rpt_content.slot[num].ptp_dns1[port], &parg[i5], i6 - i5 - 1);
    }

    if(parg[i6] != '\0')
    {
        memcpy(rpt_content.slot[num].ptp_dns2[port], &parg[i6], len - i6);
    }

    //printf("\n\n\nLUOHAO------22,Get: rpt_content.slot[%d].ptp_ip[%d]=%s\n\n\n",num,port,rpt_content.slot[num].ptp_ip[port]);

#if DEBUG_NET_INFO
    if(print_switch == 0)
        printf("%s:%s,%s,%s,%s,%s,%s#\n", NET_SAVE, rpt_content.slot[num].ptp_ip[port],
               rpt_content.slot[num].ptp_mac[port], rpt_content.slot[num].ptp_gate[port],
               rpt_content.slot[num].ptp_mask[port], rpt_content.slot[num].ptp_dns1[port],
               rpt_content.slot[num].ptp_dns2[port]);
#endif
    return 0;
}

int save_ptp_mod(char slot, unsigned char *parg)
{
    int i, j, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, len, i12;
    int num, port;
    j = i1 = i2 = i3 = i4 = i5 = i6 = i7 = i8 = i9 = i10 = i11 = i12 = 0;
    num = (int)(slot - 'a');
    port = (int)(parg[0] - 0x31);
    len = strlen(parg);
	//parg[len] = '\0';
	//printf("ptp recv %s\n",parg);
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
            if(j == 5)
            {
                i5 = i + 1;
            }
            if(j == 6)
            {
                i6 = i + 1;
            }
            if(j == 7)
            {
                i7 = i + 1;
            }
            if(j == 8)
            {
                i8 = i + 1;
            }
            if(j == 9)
            {
                i9 = i + 1;
            }
            if(j == 10)
            {
                i10 = i + 1;
            }
            if(j == 11)
            {
                i11 = i + 1;
            }
            if(j == 12)
            {
                i12 = i + 1;
            }
        }
    }
    if(i11 == 0)
    {
        return (1);
    }
    if((parg[i1] == '1') || (parg[i1] == '0') )
    {
        rpt_content.slot[num].ptp_en[port] = parg[i1];
    }
    if((parg[i2] == '1') || (parg[i2] == '0'))
    {
        rpt_content.slot[num].ptp_delaytype[port] = parg[i2];
    }
    if((parg[i3] == '1') || (parg[i3] == '0'))
    {
        rpt_content.slot[num].ptp_multicast[port] = parg[i3];
    }
    if((parg[i4] == '1') || (parg[i4] == '0'))
    {
        rpt_content.slot[num].ptp_enp[port] = parg[i4];
    }
    if((parg[i5] == '1') || (parg[i5] == '0'))
    {
        rpt_content.slot[num].ptp_step[port] = parg[i5];
    }
    if(parg[i6] != ',')
    {
        memcpy(rpt_content.slot[num].ptp_sync[port], &parg[i6], i7 - i6 - 1);
    }
    if(parg[i7] != ',')
    {
        memcpy(rpt_content.slot[num].ptp_announce[port], &parg[i7], i8 - i7 - 1);
    }
    if(parg[i8] != ',')
    {
        memcpy(rpt_content.slot[num].ptp_delayreq[port], &parg[i8], i9 - i8 - 1);
    }
    if(parg[i9] != ',')
    {
        memcpy(rpt_content.slot[num].ptp_pdelayreq[port], &parg[i9], i10 - i9 - 1);
    }
    if(parg[i10] != ',')
    {
        memset(rpt_content.slot[num].ptp_delaycom[port],0,9);
        memcpy(rpt_content.slot[num].ptp_delaycom[port], &parg[i10], i11 - i10 - 1);
    }
    if((parg[i11] == '1') || (parg[i11] == '0'))
    {
        rpt_content.slot[num].ptp_linkmode[port] = parg[i11];
    }
    if((parg[i12] == '1') || (parg[i12] == '0'))
    {
        rpt_content.slot[num].ptp_out[port] = parg[i12];
    }

    //printf("\n\n\nLUOHAO------33,Get: rpt_content.slot[%d].ptp_multicast[%d]=%d\n\n\n",num,port,rpt_content.slot[num].ptp_multicast[port]);

    /*rpt_content.slot[num].ptp_out[port]='1';*/
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s,<%c>#\n", NET_SAVE, parg, rpt_content.slot[num].ptp_linkmode[port]);
    }
#endif
    return 0;
}

int save_ptp_mtc(char slot, unsigned char *parg)
{
    int i, j, i_ip, i_prio1, i_prio2;
    int len, num, port;
    i = j = i_ip = i_prio1 = i_prio2 = 0;
    num = (int)(slot - 'a');
    port = (int)(parg[0] - 0x31);
    len = strlen(parg);
    //
    //!
    //
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
        {
            j++;
            if(j == 1)
            {
                i_ip = i + 1;
            }
            if(j == 2)
            {
                i_prio1 = i + 1;
            }
            if(j == 3)
            {
                i_prio2 = i + 1;
            }
        }
    }
    if(i_ip == 0)
    {
        return (1);
    }
    if(parg[i_ip] != ',')
    {
        memset(&rpt_ptp_mtc.ptp_mtc_ip[num][port][0],'\0',9);
        memcpy(&rpt_ptp_mtc.ptp_mtc_ip[num][port][0], &parg[i_ip], 8);
		
    }

    if(parg[i_prio1] != ',')
    {
        memset(&rpt_ptp_mtc.ptp_prio1[num][port][0], 0, 5);
        for (i = i_prio1; parg[i] != ','; i++)
        {
            memcpy(&rpt_ptp_mtc.ptp_prio1[num][port][i - i_prio1], &parg[i], 1);
        }
    }

    if(parg[i_prio2] != ',')
    {
        memset(&rpt_ptp_mtc.ptp_prio2[num][port][0], 0, 5);
        for (i = i_prio2; parg[i] != '\0'; i++) //
        {
            memcpy(&rpt_ptp_mtc.ptp_prio2[num][port][i - i_prio2], &parg[i], 1);
        }
    }
	
    return 0;
}

int save_ptp_mtc16(char slot, unsigned char *parg)
{
    int i, j, i_ip, i_inx;
    int len, num, port,str_len;
    i = j = i_inx = i_ip = 0;
    num = (int)(slot - 'a');
    port = (int)(parg[0] - 0x31);

	len = strlen(parg);
    str_len = 0;
	
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
        {
            j++;
            if(j == 1)
            {
                i_inx = i + 1;
            }
            if(j == 2)
            {
                i_ip = i + 1;
            }
        }
    }
    if(i_inx == 0)
    	return 1;

	if(port < 0 || port > 3)
		return 1;
   
    

	
	
    if(parg[i_inx] == '1')
    {
        //一帧上报8个IP地址，每个IP 8个字节，XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX
	   if((len - 4) < 8 || (len - 4) > 71)
		return 1;

	   memset(&rpt_ptp_mtc.ptp_mtc_ip[num][port][0],'\0',sizeof(rpt_ptp_mtc.ptp_mtc_ip[num][port]));
       memcpy(&rpt_ptp_mtc.ptp_mtc_ip[num][port][0], &parg[i_ip], len - 4);
		
    }
	else if(parg[i_inx] == '2')
	{
	    //一帧上报8个IP地址，每个IP 8个字节，,XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX
	    if((len - 4) < 8 || (len - 4) > 72)
			return 1;
		
	    str_len = strlen(rpt_ptp_mtc.ptp_mtc_ip[num][port]);

		//一帧上报8个IP地址，每个IP 8个字节，XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX,XXXXXXXX
		if(str_len > 71)
			return 1;
		
	    memcpy(&rpt_ptp_mtc.ptp_mtc_ip[num][port][0] + str_len , &parg[i_ip], len - 4);
	}
	else
	{
	    return 1;
	}

    return 0;
}

//
//! 2017/6/16 添加PTP闰秒事件上报CC7
//
int save_ptp_sta(char slot, unsigned char *parg)
{
	int num, i;
	int  len;
	
	num = (int)(slot - 'a');
	len = strlen(parg);
	
	if((0 == len)||(2 < len))
	{
		return 1;
	}

	i = atoi(parg);
	if((i < 0)||(i > 99))
		return 1;
	
	if(slot_type[num] == 'v' || slot_type[num] == 'x' )
    {
		if(slot == GBTP1_SLOT)
		{
		    memset(rpt_content.slot_q.leapnum,0,sizeof(rpt_content.slot_q.leapnum));
			memcpy(rpt_content.slot_q.leapnum, &parg[0], len);
		}
		else
		{
			memset(rpt_content.slot_r.leapnum,0,sizeof(rpt_content.slot_r.leapnum));
			memcpy(rpt_content.slot_r.leapnum, &parg[0], len);
		}
    }
    else
    {
		memset(rpt_content.slot[num].ntp_leap,0,sizeof(rpt_content.slot[num].ntp_leap));
		memcpy(rpt_content.slot[num].ntp_leap, &parg[0], len);
    }
	
    //printf("ptp leap %d,%s\n",num,parg);
	return 0;
}

int save_Leap_sta(char slot, unsigned char *parg)
{
	int num, i;
	int  len;
	
	num = (int)(slot - 'a');
	len = strlen(parg);
	
	if((0 == len)||(2 < len))
	{
		return 1;
	}

	i = atoi(parg);
	
	if((i < 0)||(i > 99))
		return 1;
	
	if(slot_type[num] == 'v' || slot_type[num] == 'x')
    {
		if(slot == GBTP1_SLOT)
		{
		    memset(rpt_content.slot_q.leapnum,0,sizeof(rpt_content.slot_q.leapnum));
			memcpy(rpt_content.slot_q.leapnum, &parg[0], len);
		}
		else
		{
			memset(rpt_content.slot_r.leapnum,0,sizeof(rpt_content.slot_r.leapnum));
			memcpy(rpt_content.slot_r.leapnum, &parg[0], len);
		}
    }
    
    //printf("ptp leap %d,%s\n",num,parg);
	return 0;
}

int save_ptp_dom(char slot, unsigned char *parg)
{
	int num,port ;
	int len,i_dom,i,j ;
	
	num = port = 0;
	i = j = len = i_dom = 0;
	
	num = (int)(slot - 'a');
	port = (int)(parg[0] - 0x31);
	len =strlen(parg);
	
	for(i = 0; i < len; i++)
	{
	      if(parg[i] == ',')
	      {
	            j++;
	            if(j == 1)
	            {
	                i_dom = i+1;
	            }
	      }
	}
	if(i_dom != 2)
	    return (1);
	
	if((port < 0)||(port > 4))
		return 1;
	
	if((len == 0)||(len > 5))
		return 1;
	
	if(parg[i_dom] != ',')
	{
	    memset(&rpt_ptp_mtc.ptp_dom[num][port][0],0,4);
	    memcpy(&rpt_ptp_mtc.ptp_dom[num][port][0], &parg[i_dom],len - i_dom );
	}

	return 0;
}

/**********************/

int save_gb_sta(char slot, unsigned char *parg)
{
    int i, j, i1, i2, i3, i4, len, i5, i6;
    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
    i5 = 0;
    i6 = 0;
    len = strlen(parg);
    /*<pos_mode>,<delay>,<elevation>,<format>,<acmode>
    POST,20,8888,IRIGA,GPS*/
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
            if(j == 5)
            {
                i5 = i + 1;
            }
            if(j == 6)
            {
                i6 = i + 1;
            }
        }
    }
    //if(i6==0)/***************/
    //	return (1);
    if(slot == GBTP1_SLOT)
    {
        if(parg[0] != ',')
        {
            memcpy(rpt_content.slot_q.gb_pos_mode, parg, 4);
        }
        if(parg[i2] != ',')
        {
            memcpy(rpt_content.slot_q.gb_elevation, &parg[i2], i3 - i2 - 1);
        }
        if(parg[i3] != ',')
        {
            memcpy(rpt_content.slot_q.gb_format, &parg[i3], i4 - i3 - 1);
        }
        if(parg[i4] != ',')
        {
            memset(rpt_content.slot_q.gb_acmode, 0, 4);
            memcpy(rpt_content.slot_q.gb_acmode, &parg[i4], i5 - i4 - 1);
        }

        if(parg[i5] == '0' || parg[i5] == '1' || parg[i5] == '2')
        {
            rpt_content.slot_q.gb_sta = parg[i5];
        }
        if(parg[i6] == '0')
        {
            memcpy(rpt_content.slot_q.bdzb, "BJ54", 4);
        }
        else if(parg[i6] == '1')
        {
            memcpy(rpt_content.slot_q.bdzb, "CGS2000", 7);
        }
    }
    else if(slot == GBTP2_SLOT)
    {
        if(parg[0] != ',')
        {
            memcpy(rpt_content.slot_r.gb_pos_mode, parg, 4);
        }
        if(parg[i2] != ',')
        {
            memcpy(rpt_content.slot_r.gb_elevation, &parg[i2], i3 - i2 - 1);
        }
        if(parg[i3] != ',')
        {
            memcpy(rpt_content.slot_r.gb_format, &parg[i3], i4 - i3 - 1);
        }
        if(parg[i4] != ',')
        {
            memset(rpt_content.slot_r.gb_acmode, 0, 4);
            memcpy(rpt_content.slot_r.gb_acmode, &parg[i4], i5 - i4 - 1);

        }
        if(parg[i5] == '0' || parg[i5] == '1' || parg[i5] == '2')
        {
            rpt_content.slot_r.gb_sta = parg[i5];
        }
        if(parg[i6] == '0')
        {
            memcpy(rpt_content.slot_r.bdzb, "BJ54", 4);
        }
        else if(parg[i6] == '1')
        {
            memcpy(rpt_content.slot_r.bdzb, "CGS2000", 7);
        }
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}

/*
*********************************************************************************************************
*	Function: int save_gbtp_sta(char slot, unsigned char *parg)
*	Descript: gbtpv2 status infomation.
*	Parameters:  - slot 
*                - parg :the point of input data.

*	Return:      - 0 : ok
*				 - 1 : error
*				
*********************************************************************************************************
*/
int save_gbtp_sta(char slot, unsigned char *parg)
{
    int i, j, i1, i2, i3, i4, len, i5;//, i6
	int usError;
    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
    i5 = 0;
	usError = 0;
    //i6 = 0;
    len = strlen(parg);
    /*<saten>,<acmode>,<linkstatus>,<pos_mode>,<rec_type>,<rec_ver>
    POST,20,8888,IRIGA,GPS*/
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
            if(j == 5)
            {
                i5 = i + 1;
            }
        }
    }

	if(i5 == 0)
		return 1;
    if(slot == GBTP1_SLOT)
    {
        if((parg[0] == '0')||(parg[0] == '1'))
        {
           rpt_content.slot_q.staen = parg[0];
        }
		if(parg[i1] != ',')
		{
		   //judge the len lt 4.
		   usError = ((i2 - i1 - 1) < sizeof(rpt_content.slot_q.gb_acmode)) ? 0:1;
		   if(usError)
		   	 return 1;
		   
		   //excete memset and memcpy,save data.
		   memset(rpt_content.slot_q.gb_acmode, 0, 4);
		   memcpy(rpt_content.slot_q.gb_acmode, &parg[i1], i2 - i1 - 1);
		   
		}
		if(parg[i2] == '0' || parg[i2] == '1' || parg[i2] == '2')
        {
            rpt_content.slot_q.gb_sta = parg[i2];
        }
        if(parg[i3] != ',')
		{
		    //judge the len et 4.
		    usError = ((i4 - i3 - 1) < sizeof(rpt_content.slot_q.gb_pos_mode)) ? 0:1;
			if(usError)
		   	 return 1;
			
			//excete memset and memcpy,save data.
			memcpy(rpt_content.slot_q.gb_pos_mode,&parg[i3], i4 - i3 - 1);
			rpt_content.slot_q.gb_pos_mode[i4 - i3 - 1] = '\0';
		}
        if(parg[i4] != ',')
        {
            //judge the len et 11.
		    usError = ((i5 - i4 - 1) < sizeof(rpt_content.slot_q.rec_type)) ? 0:1;
			if(usError)
		   	 return 1;
			
			//excete memset and memcpy,save data.
			memcpy(rpt_content.slot_q.rec_type, &parg[i4], i5 - i4 - 1);
			rpt_content.slot_q.rec_type[i5 - i4 - 1] = '\0';
        }
		if(parg[i5] != '\0')
        {
            //judge the len et 9.
		    usError = ((len - i5) < sizeof(rpt_content.slot_q.rec_ver)) ? 0:1;
			if(usError)
		   	 return 1;
			
			//excete memset and memcpy,save data.
			memcpy(rpt_content.slot_q.rec_ver, &parg[i5], len -i5);
			rpt_content.slot_q.rec_ver[len - i5] = '\0';
        }
    }
    else if(slot == GBTP2_SLOT)
    {
        if((parg[0] == '0')||(parg[0] == '1'))
        {
           rpt_content.slot_r.staen = parg[0];
        }
		if(parg[i1] != ',')
		{
		   //judge the len lt 4.
		   usError = ((i2 - i1 - 1) < sizeof(rpt_content.slot_r.gb_acmode)) ? 0:1;
		   if(usError)
		   	 return 1;
		   
		   //excete memset and memcpy,save data.
		   memset(rpt_content.slot_r.gb_acmode, 0, 4);
		   memcpy(rpt_content.slot_r.gb_acmode, &parg[i1], i2 - i1 - 1);
		   
		}
		if(parg[i2] == '0' || parg[i2] == '1' || parg[i2] == '2')
        {
            rpt_content.slot_r.gb_sta = parg[i2];
        }
        if(parg[i3] != ',')
		{
		    //judge the len et 4.
		    usError = ((i4 - i3 - 1) < sizeof(rpt_content.slot_r.gb_pos_mode)) ? 0:1;
			if(usError)
		   	 return 1;
			
			//excete memset and memcpy,save data.
			memcpy(rpt_content.slot_r.gb_pos_mode,&parg[i3], i4 - i3 - 1);
			rpt_content.slot_r.gb_pos_mode[i4 - i3 - 1] = '\0';
		}
        if(parg[i4] != ',')
        {
            //judge the len et 10.
		    usError = ((i5 - i4 - 1) < sizeof(rpt_content.slot_r.rec_type)) ? 0:1;
			if(usError)
		   	 return 1;
			
			//excete memset and memcpy,save data.
			memcpy(rpt_content.slot_r.rec_type, &parg[i4], i5 - i4 - 1);
			rpt_content.slot_r.rec_type[i5 - i4 - 1] = '\0';
        }
		if(parg[i5] != '\0')
        {
            //judge the len et 9.
		    usError = ((len - i5) < sizeof(rpt_content.slot_r.rec_ver)) ? 0:1;
			if(usError)
		   	 return 1;
			
			//excete memset and memcpy,save data.
			memcpy(rpt_content.slot_r.rec_ver, &parg[i5], len -i5);
			rpt_content.slot_r.rec_ver[len - i5] = '\0';
        }
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}


/*
*********************************************************************************************************
*	Function: int save_gbtp_delay(char slot, unsigned char *parg)
*	Descript: gbtpIII delay.
*	Parameters:  - slot 
*                - parg :the point of input data.

*	Return:      - 0 : ok
*				 - 1 : error
*				
*********************************************************************************************************
*/

int is_digit(unsigned char *parg,unsigned char len)
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
int save_gbtp_delay(char slot, unsigned char *parg)
{
    int len;
	int usError;
	usError = 0;
    len = strlen(parg);

	/*<delay>; -32768 ~ 32767 */
	if(len <= 0 || len > 6)
		return 1;
	
	/*judge if the parg is or digit*/	
	if(is_digit(parg, len))
		return 1;
         
	     
    if(slot == GBTP1_SLOT)
    {
           //memset(rpt_content.slot_q.delay, '\0', sizeof(rpt_content.slot_q.delay));
           memcpy(rpt_content.slot_q.delay, &parg[0], len);	
		   rpt_content.slot_q.delay[len] = '\0';
    }
    else if(slot == GBTP2_SLOT)
    {
    	   //memset(rpt_content.slot_r.delay, '\0', sizeof(rpt_content.slot_r.delay));
           memcpy(rpt_content.slot_r.delay, &parg[0], len);
		   rpt_content.slot_r.delay[len] = '\0';
    }

	printf("%s:gn_dly,%s#\n", NET_SAVE, parg);
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}
/*GBTPIII 1PPS out delay,range of -400000 ~ 400000,unit 2.5ns*/
int save_pps_delay(char slot, unsigned char *parg)
{
    int len;
	int usError;
	usError = 0;
    len = strlen(parg);

	/*<delay>; -400000 ~ 400000 */
	if(len <= 0 || len > 10)
		return 1;
	
	/*judge if the parg is or digit*/	
	if(is_digit(parg, len))
		return 1;
         
	     
    if(slot == GBTP1_SLOT)
    {
           //memset(rpt_content.slot_q.delay, '\0', sizeof(rpt_content.slot_q.delay));
           memcpy(rpt_content.slot_q.pps_out_delay, &parg[0], len);	
		   rpt_content.slot_q.pps_out_delay[len] = '\0';
    }
    else if(slot == GBTP2_SLOT)
    {
    	   //memset(rpt_content.slot_r.delay, '\0', sizeof(rpt_content.slot_r.delay));
           memcpy(rpt_content.slot_r.pps_out_delay, &parg[0], len);
		   rpt_content.slot_r.pps_out_delay[len] = '\0';
    }
    printf("%s:pps_dly,%s#\n", NET_SAVE, parg);
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}

int save_gb_ori(char slot, unsigned char *parg)
{
    if(slot == GBTP1_SLOT)
    {
        memcpy(rpt_content.slot_q.gb_ori, parg, strlen(parg));
    }
    else if(slot == GBTP2_SLOT)
    {
        memcpy(rpt_content.slot_r.gb_ori, parg, strlen(parg));
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}

/*********************************************************************************************************
*	Function: int save_gbtp_pst(char slot, unsigned char *parg)
*	Descript: gbtpv2 post time infomation.
*	Parameters:  - slot 
*                - parg :the point of input data.

*	Return:      - 0 : ok
*				 - 1 : error
*				
*********************************************************************************************************/

int save_gbtp_pst(char slot, unsigned char *parg)
{
   int sLen = 0;
   
   //string length .
   sLen = ((0 == strlen(parg))||(strlen(parg) > 70))? 0:strlen(parg);
   if(!sLen)
   	return 1;

   //copy the data.
    if(slot == GBTP1_SLOT)
    {
        memcpy(rpt_content.slot_q.gb_ori, parg, sLen);
		rpt_content.slot_q.gb_ori[sLen] = '\0';
		
    }
    else if(slot == GBTP2_SLOT)
    {
        memcpy(rpt_content.slot_r.gb_ori, parg, sLen);
		rpt_content.slot_r.gb_ori[sLen] = '\0';
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}

int save_gb_rev(char slot, unsigned char *parg)
{

    if(slot == GBTP1_SLOT)
    {
        if(parg[0] == 'G')
        {
            memset(rpt_content.slot_q.gb_rev_g, '\0', 256);
            memcpy(rpt_content.slot_q.gb_rev_g, parg, strlen(parg));
        }
        else if(parg[0] == 'B')
        {
            memset(rpt_content.slot_q.gb_rev_b, '\0', 256);
            memcpy(rpt_content.slot_q.gb_rev_b, parg, strlen(parg));
        }
    }
    else if(slot == GBTP2_SLOT)
    {
        if(parg[0] == 'G')
        {
            memset(rpt_content.slot_r.gb_rev_g, '\0', 256);
            memcpy(rpt_content.slot_r.gb_rev_g, parg, strlen(parg));
        }
        else if(parg[0] == 'B')
        {
            memset(rpt_content.slot_r.gb_rev_b, '\0', 256);
            memcpy(rpt_content.slot_r.gb_rev_b, parg, strlen(parg));
        }
    }
    
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}

/*
*********************************************************************************************************
*	Function: int save_gbtp_info(char slot, unsigned char *parg)
*	Descript: gbtpv2 status infomation.
*	Parameters:  - slot 
*                - parg :the point of input data.

*	Return:      - 0 : ok
*				 - 1 : error
*				
*********************************************************************************************************
*/
int save_gbtp_info(char slot, unsigned char *parg)
{

    if(slot == GBTP1_SLOT)
    {
        if(parg[0] == 'G' && parg[1] == 'P' && parg[2] == 'S')
        {
            memset(rpt_content.slot_q.gb_rev_g, '\0', 256);
            memcpy(rpt_content.slot_q.gb_rev_g, parg, strlen(parg));
        }
        else if(parg[0] == 'B' && parg[1] == 'D')
        {
            memset(rpt_content.slot_q.gb_rev_b, '\0', 256);
            memcpy(rpt_content.slot_q.gb_rev_b, parg, strlen(parg));
        }
		else if(parg[0] == 'G' && parg[1] == 'L'&& parg[2] == 'O')
        {
            memset(rpt_content.slot_q.gb_rev_glo, '\0', 256);
            memcpy(rpt_content.slot_q.gb_rev_glo, parg, strlen(parg));
        }
		else if(parg[0] == 'G' && parg[1] == 'A'&& parg[2] == 'L')
        {
            memset(rpt_content.slot_q.gb_rev_gal, '\0', 256);
            memcpy(rpt_content.slot_q.gb_rev_gal, parg, strlen(parg));
        }
    }
    else if(slot == GBTP2_SLOT)
    {
        if(parg[0] == 'G' && parg[1] == 'P' && parg[2] == 'S')
        {
            memset(rpt_content.slot_r.gb_rev_g, '\0', 256);
            memcpy(rpt_content.slot_r.gb_rev_g, parg, strlen(parg));
        }
        else if(parg[0] == 'B' && parg[1] == 'D')
        {
            memset(rpt_content.slot_r.gb_rev_b, '\0', 256);
            memcpy(rpt_content.slot_r.gb_rev_b, parg, strlen(parg));
        }
		else if(parg[0] == 'G' && parg[1] == 'L'&& parg[2] == 'O')
        {
            memset(rpt_content.slot_r.gb_rev_glo, '\0', 256);
            memcpy(rpt_content.slot_r.gb_rev_glo, parg, strlen(parg));
        }
		else if(parg[0] == 'G' && parg[1] == 'A'&& parg[2] == 'L')
        {
            memset(rpt_content.slot_r.gb_rev_gal, '\0', 256);
            memcpy(rpt_content.slot_r.gb_rev_gal, parg, strlen(parg));
        }
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}

int save_gb_tod(char slot, unsigned char *parg)
{
    unsigned char  sendbuf[SENDBUFSIZE];
    unsigned char  ctag[6] = "000000";

    memset(sendbuf, '\0', SENDBUFSIZE);
    if(slot == GBTP1_SLOT)
    {
        memcpy(rpt_content.slot_q.gb_tod, parg, 3);
    }
    else if(slot == GBTP2_SLOT)
    {
        memcpy(rpt_content.slot_r.gb_tod, parg, 3);
    }
    /**  SET-TOD-STA:<ctag>::<ms>,<ppssta>,<sourcetype>; ***/
    sprintf((char *)sendbuf, "SET-TOD-STA:%s::%d,%s;", ctag, (slot == GBTP1_SLOT) ? 0 : 1, parg);
    //printf("<save_gb_tod>:%s\n",sendbuf);

    if(slot_type[15] == 'K' || slot_type[15] == 'R')
    {
        sendtodown(sendbuf, (unsigned char)RB2_SLOT);
    }
    if(slot_type[14] == 'K' || slot_type[14] == 'R')
    {
        sendtodown(sendbuf, (unsigned char)RB1_SLOT);
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}

int save_gb_alm(char slot, unsigned char *parg)
{
    int num;
    unsigned char  tmp[12];
    memset(tmp, '\0', 12);

    num = (int)(slot - 'a');
    rpt_alm_to_client(slot, parg);
    sprintf((char *)tmp, "\"%c,%c,%s\"", slot, slot_type[num], parg);
    if(slot == GBTP1_SLOT)
    { 
        memset(alm_sta[16],'\0',14);
        memcpy(alm_sta[16], tmp, strlen(tmp));
    }
    else if(slot == GBTP2_SLOT)
    {
        memset(alm_sta[17],'\0',14);
        memcpy(alm_sta[17], tmp, strlen(tmp));
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}

/*
*********************************************************************************************************
*	Function: int save_gbtp_alm(char slot, unsigned char *parg)
*	Descript: gbtpv2 alarm infomation.
*	Parameters:  - slot 
*                - parg :the point of input data.

*	Return:      - 0 : ok
*				 - 1 : error
*				
*********************************************************************************************************
*/

int save_gbtp_alm(char slot, unsigned char *parg)
{
    int num;
    unsigned char  tmp[12];
    memset(tmp, '\0', 12);

    num = (int)(slot - 'a');
    rpt_alm_to_client(slot, parg);
	
    sprintf((char *)tmp, "\"%c,%c,%s\"", slot, slot_type[num], parg);
    if(slot == GBTP1_SLOT)
    { 
        memset(alm_sta[16],'\0',14);
        memcpy(alm_sta[16], tmp, strlen(tmp));
    }
    else if(slot == GBTP2_SLOT)
    {
        memset(alm_sta[17],'\0',14);
        memcpy(alm_sta[17], tmp, strlen(tmp));
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}


int save_sy_ver(char slot, unsigned char *parg)
{
    int num, len;
    int i, j, i1, i2, i3,i4,i5;
    unsigned char mcu_ver[7];
    unsigned char fpga_ver[7];
	unsigned char pcb_ver[7];
    unsigned char temp[40];

    if( (('m' == slot) && (EXT_DRV == slot_type[12]) ) ||
            (('n' == slot) && (EXT_DRV == slot_type[13])) ||
            (EXT_1_MGR_PRI == slot) || (EXT_1_MGR_RSV == slot) ||
            (EXT_2_MGR_PRI == slot) || (EXT_2_MGR_RSV == slot) ||
            (EXT_3_MGR_PRI == slot) || (EXT_3_MGR_RSV == slot) ||
            (slot >= EXT_1_OUT_LOWER && slot <= EXT_1_OUT_UPPER) ||
            (slot >= EXT_2_OUT_LOWER && slot <= EXT_2_OUT_UPPER) ||
            (slot >= EXT_3_OUT_LOWER && slot <= EXT_3_OUT_UPPER) )
    {
        if(0 == ext_out_ver_trpt(slot, parg, &gExtCtx))
        {
            return 1;
        }
    }
    else
    {
        //printf("main-----ver :%s   slot :%d\n",parg,slot);
        j = 0;
        i1 = 0;
        i2 = 0;
        i3 = 0;
		i4 =0;
		i5 = 0;
        len = strlen(parg);
        if(strncmp1(parg, "MCU", 3) != 0) //
        {
            if(slot == RB1_SLOT)
            {
                memcpy(rpt_content.slot_o.sy_ver, parg, len);
            }
            else if(slot == RB2_SLOT)
            {
                memcpy(rpt_content.slot_p.sy_ver, parg, len);
            }
            else if(slot == GBTP1_SLOT)
            {
                memset(rpt_content.slot_q.sy_ver, '\0', sizeof(rpt_content.slot_q.sy_ver));
                memcpy(rpt_content.slot_q.sy_ver, parg, len);
                /*MCU,V01.00,FPGA,V01.00,CPLD,V01.00,PCB,V01.00,*/
                /*V01.00,V01.00,V01.00,<sta>*/
                if(rpt_content.slot_q.gb_sta == '0' || rpt_content.slot_q.gb_sta == '1' || rpt_content.slot_q.gb_sta == '2')
                {
                    rpt_content.slot_q.sy_ver[21] = rpt_content.slot_q.gb_sta;
                    rpt_content.slot_q.sy_ver[22] = '\0';
                }
                else
                {
                    rpt_content.slot_q.sy_ver[21] = '\0';
                }
            }
            else if(slot == GBTP2_SLOT)
            {
                memset(rpt_content.slot_r.sy_ver, '\0', sizeof(rpt_content.slot_r.sy_ver));
                memcpy(rpt_content.slot_r.sy_ver, parg, len);

                if(rpt_content.slot_r.gb_sta == '0' || rpt_content.slot_r.gb_sta == '1' || rpt_content.slot_r.gb_sta == '2')
                {
                    rpt_content.slot_r.sy_ver[21] = rpt_content.slot_r.gb_sta;
                    rpt_content.slot_r.sy_ver[22] = '\0';
                }
                else
                {
                    rpt_content.slot_r.sy_ver[21] = '\0';
                }
            }
            else
            {
                num = (int)(slot - 'a');
                if((num >= 0) && (num < 14))
                {
                    memcpy(rpt_content.slot[num].sy_ver, parg, len);
                    //printf("--%s--\n", rpt_content.slot[num].sy_ver);
                }
            }
        }
        else
        {
			
            for(i = 0; i < len; i++)
            {
                if(parg[i] == ',')
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
						i4 = i+1;
					if(j == 5)
						i5 = i+1;
                }
            }
            if(i3 == 0)
            {
                return 1;
            }
			/*MCU,V01.00,FPGA,V01.00,PCB,V01.00*/
			memset(pcb_ver,0,7);
            memset(mcu_ver, 0, 7);
            memcpy(mcu_ver, &parg[i1], i2 - i1 - 1);
            memset(fpga_ver, 0, 7);
			if(i4 == 0)
			{
            	memcpy(fpga_ver, &parg[i3], len - i3);
            	sprintf((char *)temp, "%s,%s,,", mcu_ver, fpga_ver);
			}
			else
			{
				memcpy(fpga_ver, &parg[i3], i4 - i3-1);
				memcpy(pcb_ver, &parg[i5], len - i5);
				sprintf((char *)temp, "%s,%s,,%s", mcu_ver, fpga_ver,pcb_ver);
			}
            len = strlen(temp);
            if(slot == RB1_SLOT)
            {
                memcpy(rpt_content.slot_o.sy_ver, temp, len);
            }
            else if(slot == RB2_SLOT)
            {
                memcpy(rpt_content.slot_p.sy_ver, temp, len);
            }
            else if(slot == GBTP1_SLOT)
            {
                memset(rpt_content.slot_q.sy_ver, '\0', sizeof(rpt_content.slot_q.sy_ver));
                memcpy(rpt_content.slot_q.sy_ver, temp, len);
            }
            else if(slot == GBTP2_SLOT)
            {
                memset(rpt_content.slot_r.sy_ver, '\0', sizeof(rpt_content.slot_r.sy_ver));
                memcpy(rpt_content.slot_r.sy_ver, temp, len);
            }
            else
            {
                
                num = (int)(slot - 'a');
                if((num >= 0) && (num < 14))
                {
                    memset(rpt_content.slot[num].sy_ver, '\0', sizeof(rpt_content.slot[num].sy_ver));
                    memcpy(rpt_content.slot[num].sy_ver, temp, len);
                }
            }
        }
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif

    return 0;
}


int save_ch1andch3dph(char slot, unsigned char *parg)
{
    int i, j, i1, i2, i3, i4, i5, i6, i7,i8, len;
	j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
    i5 = 0;
    i6 = 0;
    i7 = 0;
	i8 = 0;
	len = strlen(parg);
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
            if(j == 5)
            {
                i5 = i + 1;
            }
            if(j == 6)
            {
                i6 = i + 1;
            }
            if(j == 7)
            {
                i7 = i + 1;
            }
			if(j == 8)
			{
                i8 = i + 1;
			}
        }
    }
	 if(i8 == 0)
    {
        return 1;
    }
	if(slot == RB1_SLOT)
    {
        
        if(parg[i1] != ',')
        {
            memset(rpt_content.slot_o.rb_phase[0], 0, 9);
            memcpy(rpt_content.slot_o.rb_phase[0], &parg[i1], i2 - i1 - 1);
        }
		else
		{
		    memset(rpt_content.slot_o.rb_phase[0], 0, 9);
		}
        
        if(parg[i3] != ',')
        {
            memset(rpt_content.slot_o.rb_phase[1], 0, 9);
            memcpy(rpt_content.slot_o.rb_phase[1], &parg[i3], i4 - i3 - 1);
        }
		else
		{ 
		   memset(rpt_content.slot_o.rb_phase[1], 0, 9);
		}  
        
    }
    else if(slot == RB2_SLOT)
    {

        
        if(parg[i1] != ',')
        {
            memset(rpt_content.slot_p.rb_phase[0], 0, 9);
            memcpy(rpt_content.slot_p.rb_phase[0], &parg[i1], i2 - i1 - 1);
        }
		else
		{
		   memset(rpt_content.slot_p.rb_phase[0], 0, 9);
		}
        
        if(parg[i3] != ',')
        {
            memset(rpt_content.slot_p.rb_phase[1], 0, 9);
            memcpy(rpt_content.slot_p.rb_phase[1], &parg[i3], i4 - i3 - 1);
        }
		else
		{
		   memset(rpt_content.slot_p.rb_phase[1], 0, 9);
		}
    }

    return 0;
	
}


int save_rx_dph(char slot, unsigned char *parg)
{
	//
	//!new defined.
	int i;
	int j,i1,i2,i3,i4,i5,i6,i7,i8,len;

    unsigned char  tmp[SENDBUFSIZE];
    memset(tmp, '\0', SENDBUFSIZE);

	//
    //!new initial.
    len=i= 0;
    j=i1=i2=i3=i4=i5=i6=i7=i8=0;

	len = strlen(parg);
    for(i =0;i<len;i++)
    {
	   if(parg[i] == ',')
	   {
	      j++;
	      if(j == 1) i1 = i+1;
	      if(j == 2) i2 = i+1;
	      if(j == 3) i3 = i+1;
	      if(j == 4) i4 = i+1;
	      if(j == 5) i5 = i+1;
	      if(j == 6) i6 = i+1;
	      if(j == 7) i7 = i+1;
	      if(j == 8) i8 = i+1;
	   }
    }
	if(i8 == 0)
     return 1;
	
    get_time_fromnet();
    if(slot == RB1_SLOT)
    {
        sprintf((char *)tmp, "\"%c,%s,%s,%s\"%c%c%c%c%c", slot, parg, mcp_date, mcp_time, CR, LF, SP, SP, SP);
        memcpy(rpt_content.slot_o.rb_inp[flag_i_o], tmp, strlen(tmp));
        flag_i_o++;
        if(flag_i_o == 10)
        {
            flag_i_o = 0;
        }
		
		//
		//!new record #17 #18 #1 time 2# time 2# fre 1#  
		//            #17 #18                 2mhz   2mb
		if(parg[0] != ',')
			strncpynew(rpt_content.slot_o.rb_ph[0],&parg[0],i1-1);
		else
			memset(rpt_content.slot_o.rb_ph[0],'\0',10);
 		
		if(parg[i2] != ',')
			strncpynew(rpt_content.slot_o.rb_ph[1],&parg[i2],i3-i2-1);
		else
			memset(rpt_content.slot_o.rb_ph[1],'\0',10);

		if(parg[i1] != ',')
			strncpynew(rpt_content.slot_o.rb_ph[2],&parg[i1],i2-i1-1);
		else
			memset(rpt_content.slot_o.rb_ph[2],'\0',10);
		
        if(parg[i3] != ',')
			strncpynew(rpt_content.slot_o.rb_ph[3],&parg[i3],i4-i3-1);
		else
			memset(rpt_content.slot_o.rb_ph[3],'\0',10);
		
		if(parg[i5] != ',')    
        	strncpynew(rpt_content.slot_o.rb_ph[4],&parg[i5],i6-i5-1); 
		else
			memset(rpt_content.slot_o.rb_ph[4],'\0',10);
		
		if(parg[i7] != ',')    
        	strncpynew(rpt_content.slot_o.rb_ph[5],&parg[i7],i8-i7-1);
		else
			memset(rpt_content.slot_o.rb_ph[5],'\0',10);
		
		if(parg[i8] != '\0')    
        		strncpynew(rpt_content.slot_o.rb_ph[6],&parg[i8],len-i8);
		
    }
    if(slot == RB2_SLOT)
    {
        sprintf((char *)tmp, "\"%c,%s,%s,%s\"%c%c%c%c%c", slot, parg, mcp_date, mcp_time, CR, LF, SP, SP, SP);
        memcpy(rpt_content.slot_p.rb_inp[flag_i_p], tmp, strlen(tmp));
        flag_i_p++;
        if(flag_i_p == 10)
        {
            flag_i_p = 0;
        }
		//
		//!new record #17 #18 #1 time 2# time 2# fre 1#  
		//            #17 #18                 2mhz   2mb
		if(parg[0] != ',')
			strncpynew(rpt_content.slot_p.rb_ph[0],&parg[0],i1-1);
		else
			memset(rpt_content.slot_p.rb_ph[0],'\0',10);
		
		if(parg[i2] != ',')
			strncpynew(rpt_content.slot_p.rb_ph[1],&parg[i2],i3-i2-1); 
		else
			memset(rpt_content.slot_p.rb_ph[1],'\0',10);

		if(parg[i1] != ',')
			strncpynew(rpt_content.slot_p.rb_ph[2],&parg[i1],i2-i1-1);
		else
			memset(rpt_content.slot_p.rb_ph[2],'\0',10);
		
        if(parg[i3] != ',')
			strncpynew(rpt_content.slot_p.rb_ph[3],&parg[i3],i4-i3-1);
		else
			memset(rpt_content.slot_p.rb_ph[3],'\0',10);

		if(parg[i4] != ',')    
        	strncpynew(rpt_content.slot_p.rb_ph[4],&parg[i4],i5-i4-1);
		else
			memset(rpt_content.slot_p.rb_ph[4],'\0',10);

		if(parg[i6] != ',')    
        	strncpynew(rpt_content.slot_p.rb_ph[5],&parg[i6],i7-i6-1);
		else
			memset(rpt_content.slot_p.rb_ph[5],'\0',10);

		if(parg[i8] != '\0')    
        		strncpynew(rpt_content.slot_p.rb_ph[6],&parg[i8],len-i8);
    }
    save_ch1andch3dph(slot,parg);
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}

int save_rf_src(char slot, unsigned char *parg)
{
    int i, j, i1, i2, i3, i4, i5, len;
    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
    i5 = 0;
    len = strlen(parg);
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
            if(j == 5)
            {
                i5 = i + 1;
            }
        }
    }
    if(i5 == 0)
    {
        return 1;
    }
    if(slot == RB1_SLOT)
    {
        if(parg[0] != ',')
        {
            rpt_content.slot_o.rb_ref = parg[0];
            get_out_lev(0, 0);
        }
        if(parg[i1] != ',')
        {
            memcpy(rpt_content.slot_o.rb_prio, &parg[i1], i2 - i1 - 1);
        }
        if(parg[i2] != ',')
        {
            memcpy(rpt_content.slot_o.rb_mask, &parg[i2], i3 - i2 - 1);
        }
        if(parg[i3] != ',')
        {
            memcpy(rpt_content.slot_o.rb_leapmask, &parg[i3], i4 - i3 - 1);
        }
        if(parg[i4] != ',')
        {
            memcpy(rpt_content.slot_o.rb_leap, &parg[i4], i5 - i4 - 1);
        }
        if(parg[i5] != '\0')
        {
            rpt_content.slot_o.rb_leaptag = parg[i5];
        }
    }
    else if(slot == RB2_SLOT)
    {
        if(parg[0] != ',')
        {
            rpt_content.slot_p.rb_ref = parg[0];
            get_out_lev(0, 0);
        }
        if(parg[i1] != ',')
        {
            memcpy(rpt_content.slot_p.rb_prio, &parg[i1], i2 - i1 - 1);
        }
        if(parg[i2] != ',')
        {
            memcpy(rpt_content.slot_p.rb_mask, &parg[i2], i3 - i2 - 1);
        }
        if(parg[i3] != ',')
        {
            memcpy(rpt_content.slot_p.rb_leapmask, &parg[i3], i4 - i3 - 1);
        }
        if(parg[i4] != ',')
        {
            memcpy(rpt_content.slot_p.rb_leap, &parg[i4], i5 - i4 - 1);
        }
        if(parg[i5] != '\0')
        {
            rpt_content.slot_p.rb_leaptag = parg[i5];
        }
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        if(slot == RB1_SLOT)
            printf("%s:<%c>%s,%s,%s,%s#\n", NET_SAVE, slot, rpt_content.slot_o.rb_prio,
                   rpt_content.slot_o.rb_mask, rpt_content.slot_o.rb_leapmask,
                   rpt_content.slot_o.rb_leap);
        else if(slot == RB2_SLOT)
            printf("%s:<%c>%s,%s,%s,%s#\n", NET_SAVE, slot, rpt_content.slot_p.rb_prio,
                   rpt_content.slot_p.rb_mask, rpt_content.slot_p.rb_leapmask,
                   rpt_content.slot_p.rb_leap);
    }
#endif

    return 0;
}

int save_rb_tod(char slot, unsigned char *parg)
{
    /** SET-TOD-STA:<ctag>::<ms>,<ppssta>,<sourcetype>; **/
    unsigned char  sendbuf[SENDBUFSIZE];
    //unsigned char  *ctag="000000";
    //char send_slot;
    //int i;
    int value = 0;
    char tmp_buf[64];
    //printf("<save_rb_tod>:%s\n",parg);
    memset(sendbuf, '\0', SENDBUFSIZE);
    if(slot == RB1_SLOT)
    {
        if(strncmp1(rpt_content.slot_o.rb_msmode, "MAIN", 4) == 0)
        {
            value = 0;
        }
        //else if(strncmp1(rpt_content.slot_o.rb_msmode,"STBY",4)==0)
        //	value=1;
        else
        {
            return 0;
        }
    }
    else if(slot == RB2_SLOT)
    {
        if(strncmp1(rpt_content.slot_p.rb_msmode, "MAIN", 4) == 0)
        {
            value = 0;
        }
        //else if(strncmp1(rpt_content.slot_p.rb_msmode,"STBY",4)==0)
        //	value=1;
        else
        {
            return 0;
        }
    }
    else
    {
        return 0;
    }

    //modified by zhanghui 2013-9-10 15:50:12
    memset(tmp_buf, 0, 64);
    memcpy(tmp_buf, parg, strlen(parg));
    //printf("<save_rb_tod>:%s\n",parg);
    send_tod(tmp_buf, slot);

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif

    return 0;
}

int save_rf_dey(char slot, unsigned char *parg)
{
    int i, j, i1, i2, i3, i4, i5, i6, i7, len;
    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
    i5 = 0;
    i6 = 0;
    i7 = 0;
    len = strlen(parg);
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
            if(j == 5)
            {
                i5 = i + 1;
            }
            if(j == 6)
            {
                i6 = i + 1;
            }
            if(j == 7)
            {
                i7 = i + 1;
            }
        }
    }
    if(i7 == 0)
    {
        return 1;
    }
    if(slot == RB1_SLOT)
    {
        if(parg[0] != ',')
        {
            memset(rpt_content.slot_o.rf_dey[0], 0, 9);
            memcpy(rpt_content.slot_o.rf_dey[0], &parg[0], i1 - 1);
        }
        if(parg[i1] != ',')
        {
            memset(rpt_content.slot_o.rf_dey[1], 0, 9);
            memcpy(rpt_content.slot_o.rf_dey[1], &parg[i1], i2 - i1 - 1);
        }
        if(parg[i2] != ',')
        {
            memset(rpt_content.slot_o.rf_dey[2], 0, 9);
            memcpy(rpt_content.slot_o.rf_dey[2], &parg[i2], i3 - i2 - 1);
        }
        if(parg[i3] != ',')
        {
            memset(rpt_content.slot_o.rf_dey[3], 0, 9);
            memcpy(rpt_content.slot_o.rf_dey[3], &parg[i3], i4 - i3 - 1);
        }
        if(parg[i4] != ',')
        {
            memset(rpt_content.slot_o.rf_dey[4], 0, 9);
            memcpy(rpt_content.slot_o.rf_dey[4], &parg[i4], i5 - i4 - 1);
        }
        if(parg[i5] != ',')
        {
            memset(rpt_content.slot_o.rf_dey[5], 0, 9);
            memcpy(rpt_content.slot_o.rf_dey[5], &parg[i5], i6 - i5 - 1);
        }
        if(parg[i6] != ',')
        {
            memset(rpt_content.slot_o.rf_dey[6], 0, 9);
            memcpy(rpt_content.slot_o.rf_dey[6], &parg[i6], i7 - i6 - 1);
        }
        if(parg[i7] != '\0')
        {
            memset(rpt_content.slot_o.rf_dey[7], 0, 9);
            memcpy(rpt_content.slot_o.rf_dey[7], &parg[i7], len - i7);
        }
    }
    else if(slot == RB2_SLOT)
    {

        if(parg[0] != ',')
        {
            memset(rpt_content.slot_p.rf_dey[0], 0, 9);
            memcpy(rpt_content.slot_p.rf_dey[0], &parg[0], i1 - 1);
        }
        if(parg[i1] != ',')
        {
            memset(rpt_content.slot_p.rf_dey[1], 0, 9);
            memcpy(rpt_content.slot_p.rf_dey[1], &parg[i1], i2 - i1 - 1);
        }
        if(parg[i2] != ',')
        {
            memset(rpt_content.slot_p.rf_dey[2], 0, 9);
            memcpy(rpt_content.slot_p.rf_dey[2], &parg[i2], i3 - i2 - 1);
        }
        if(parg[i3] != ',')
        {
            memset(rpt_content.slot_p.rf_dey[3], 0, 9);
            memcpy(rpt_content.slot_p.rf_dey[3], &parg[i3], i4 - i3 - 1);
        }
        if(parg[i4] != ',')
        {
            memset(rpt_content.slot_p.rf_dey[4], 0, 9);
            memcpy(rpt_content.slot_p.rf_dey[4], &parg[i4], i5 - i4 - 1);
        }
        if(parg[i5] != ',')
        {
            memset(rpt_content.slot_p.rf_dey[5], 0, 9);
            memcpy(rpt_content.slot_p.rf_dey[5], &parg[i5], i6 - i5 - 1);
        }
        if(parg[i6] != ',')
        {
            memset(rpt_content.slot_p.rf_dey[6], 0, 9);
            memcpy(rpt_content.slot_p.rf_dey[6], &parg[i6], i7 - i6 - 1);
        }
        if(parg[i7] != '\0')
        {
            memset(rpt_content.slot_p.rf_dey[7], 0, 9);
            memcpy(rpt_content.slot_p.rf_dey[7], &parg[i7], len - i7);
        }
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}

int save_rx_sta(char slot, unsigned char *parg)
{
    int i, j, i1, i2, len, i3, i4, i5;
    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', sizeof(unsigned char) * SENDBUFSIZE);
    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
    i5 = 0;
    len = strlen(parg);

    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
            if(j == 5)
            {
                i5 = i + 1;
            }
        }
    }
    if(i5 == 0)
    {
        return 1;
    }
    if(slot == RB1_SLOT)
    {
        if(parg[0] != ',')
        {
            memcpy(rpt_content.slot_o.rb_typework, &parg[0], i1 - 1);
        }
        if(parg[i1] != ',')
        {
            memcpy(rpt_content.slot_o.rb_msmode, &parg[i1], i2 - i1 - 1);
            get_out_lev(0, 0);
        }
        if(parg[i2] != ',')
        {
            memcpy(rpt_content.slot_o.rb_trmode, &parg[i2], i3 - i2 - 1);
            get_out_lev(0, 0);
        }
        if(parg[i3] != ',')
        {
            if(parg[i3] != conf_content.slot_u.fb)
            {
                sprintf((char *)sendbuf, "SET-RB-FB:000000::%c;", conf_content.slot_u.fb);
                sendtodown(sendbuf, RB1_SLOT);
            }
        }
        if(parg[i4] != ',')
        {
            if((parg[i4] < '@') || (parg[i4] > 'E'))
            {}
            else
            {
                rpt_content.slot_o.rb_sa = parg[i4];
            }
        }
        if(parg[i5] != '\0')
        {
            if((parg[i5] < 'a') || (parg[i5] > 'g'))
            {}
            else
            {
                rpt_content.slot_o.rb_tl = parg[i5];
            }
        }
    }
    else if(slot == RB2_SLOT)
    {
        if(parg[0] != ',')
        {
            memcpy(rpt_content.slot_p.rb_typework, &parg[0], i1 - 1);
        }
        if(parg[i1] != ',')
        {
            memcpy(rpt_content.slot_p.rb_msmode, &parg[i1], i2 - i1 - 1);
            get_out_lev(0, 0);
        }
        if(parg[i2] != ',')
        {
            memcpy(rpt_content.slot_p.rb_trmode, &parg[i2], i3 - i2 - 1);
            get_out_lev(0, 0);
        }
        if(parg[i3] != ',')
        {
            if(parg[i3] != conf_content.slot_u.fb)
            {
                sprintf((char *)sendbuf, "SET-RB-FB:000000::%c;", conf_content.slot_u.fb);
                sendtodown(sendbuf, RB2_SLOT);
            }
        }
        if(parg[i4] != ',')
        {
            if((parg[i4] < '@') || (parg[i4] > 'E'))
            {
            
            }
            else
            {
                rpt_content.slot_p.rb_sa = parg[i4];
            }
        }
        if(parg[i5] != '\0')
        {
            if((parg[i5] < 'a') || (parg[i5] > 'g'))
            {
            
            }
            else
            {
                rpt_content.slot_p.rb_tl = parg[i5];
            }
        }
    }
    get_out_lev(0, 0);
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        if(slot == RB1_SLOT)
            printf("%s:<%c>%s,%s,%s#\n", NET_SAVE, slot, rpt_content.slot_o.rb_typework,
                   rpt_content.slot_o.rb_msmode, rpt_content.slot_o.rb_trmode);
        else if(slot == RB2_SLOT)
            printf("%s:<%c>%s,%s,%s#\n", NET_SAVE, slot, rpt_content.slot_p.rb_typework,
                   rpt_content.slot_p.rb_msmode, rpt_content.slot_p.rb_trmode);
    }
#endif

    return 0;
}

int save_rx_alm(char slot, unsigned char *parg)
{
    unsigned char  tmp[14];
    int num;
    unsigned char  buf[4];


    memset(tmp, '\0', 14);
    num = (int)(slot - 'a');
    rpt_alm_to_client(slot, parg);
    get_out_lev(0, 0);

    sprintf((char *)tmp, "\"%c,%c,%s\"", slot, slot_type[num], parg);
    if(slot == RB1_SLOT)
    {
        memcpy(alm_sta[14], tmp, strlen(tmp));

        strncpynew(buf, rpt_content.slot_o.tod_buf, 3);
        //printf("<save_rx_alm>:%s\n",buf);

        send_tod(buf, slot);

    }
    else if(slot == RB2_SLOT)
    {
        memcpy(alm_sta[15], tmp, strlen(tmp));

        strncpynew(buf, rpt_content.slot_p.tod_buf, 3);

        //printf("<save_rx_alm>:%s\n",buf);
        send_tod(buf, slot);

    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        if(slot == RB1_SLOT)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, alm_sta[14]);
        }
        else if(slot == RB2_SLOT)
        {
            printf("%s:<%c>%s#\n", NET_SAVE, slot, alm_sta[15]);
        }
    }
#endif
    return 0;
}

int save_tp_alm(char slot, unsigned char *parg)
{
    int num;
    unsigned char  tmp[14];
    memset(tmp, '\0', 14);
    rpt_alm_to_client(slot, parg);
    sprintf((char *)tmp, "\"%c,I,%s\"", slot, parg);
    num = (int)(slot - 'a');
    memcpy(alm_sta[num], tmp, strlen(tmp));
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}

int save_out_alm(char slot, unsigned char *parg)
{
    int num;
    unsigned char  tmp[14];

    if (strlen(parg) > 14)
    {
        printf("\n\n\n! ! ! para  overlength : %s\n\n\n", __func__);
        return -1;
    }

    if(slot >= 'a' && slot <= 'w')
    {
        memset(tmp, '\0', 14);
        num = (int)(slot - 'a');
        rpt_alm_to_client(slot, parg);
        sprintf((char *)tmp, "\"%c,%c,%s\"", slot, slot_type[num], parg);
        memcpy(alm_sta[num], tmp, strlen(tmp));
    }
    else
    {
        if(0 == ext_out_alm_trpt(slot, parg, &gExtCtx))
        {
            return 1;
        }
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif

    return 0;
}

int save_ptp_alm(char slot, unsigned char *parg)
{
    int num;
    unsigned char  tmp[14];
    memset(tmp, '\0', 14);
    rpt_alm_to_client(slot, parg);
    num = (int)(slot - 'a');
    sprintf((char *)tmp, "\"%c,%c,%s\"", slot, slot_type[num], parg);
    memcpy(alm_sta[num], tmp, strlen(tmp));
	//printf("PTP ALM %s \n",alm_sta[num]);
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
}



void JudgeAllowExtOnLine(const char slot, unsigned char *parg)
{
    u8_t online;
    u8_t flag;

    online = gExtCtx.save.onlineSta;

    if( (EXT_1_MGR_PRI == slot || EXT_1_MGR_RSV == slot)
            && ( (online & 0x01) == 0x00 ) )
    {
        printf("---1\n");
        flag = 0;
    }
    else if( (EXT_2_MGR_PRI == slot || EXT_2_MGR_RSV == slot)
             && ( (online & 0x02) == 0x00 ) )
    {
        printf("---2\n");
        flag = 0;
    }
    else if( (EXT_3_MGR_PRI == slot || EXT_3_MGR_RSV == slot)
             && ( (online & 0x04) == 0x00 ) )
    {
        printf("---3\n");
        flag = 0;
    }
    else
    {
        flag = 1;
    }

    if(0 == flag)
    {
        memset(parg, 0, 14);
        memset(parg, 'O', 14);
    }
    else
    {
    }

}


int save_ext_bid(char slot, unsigned char *parg)
{

    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }

    JudgeAllowExtOnLine(slot, parg);

    if(0 == ext_bpp_cfg(slot, parg, &gExtCtx))
    {
        return 1;
    }

    if(0 == ext_bid_cmp(slot, parg, &gExtCtx))
    {
        return 1;
    }

    if(0 == ext_out_bid_trpt(slot, parg, &gExtCtx))
    {
        return 1;
    }

    return 0;
}






int save_ext_mgr_pr(char slot, unsigned char *parg)
{
    if(print_switch == 0)
    {
        printf("%s:%s#\n", NET_SAVE, parg);
    }

    if(0 == ext_mgr_pr_trpt(slot, parg, &gExtCtx))
    {
        return 1;
    }

    return 0;
}





int save_out_sta(char slot, unsigned char *parg)
{
    int num;

    if(slot >= 'a' && slot <= 'n')
    {
        num = (int)(slot - 'a');
        if((parg[0] == '1') || (parg[0] == '0'))
        {
            rpt_content.slot[num].out_mode = parg[0];
        }
        else
        {
            return 1;
        }

        if((parg[2] == '1') || (parg[2] == '0'))
        {
            rpt_content.slot[num].out_sta = parg[2];
        }
        else
        {
            return 1;
        }
		//
		//OUT32S ADD
		//
		//printf("out32s mod change:%d,%s\n",num,parg);
    }
    else
    {
        if(3 == strlen(parg))
        {
            if(0 == ext_out_mode_crpt(0, slot, parg, &gExtCtx))
            {
                return 1;
            }
        }
        else
        {
            if(0 == ext_out_mode_trpt(slot, parg, &gExtCtx))
            {
                return 1;
            }
        }
    }

    return 0;
}





int save_out_prr(char slot, unsigned char *parg)
{
    if(slot >= 'a' && slot <= 'n')
    {
        return 0;
    }
    else
    {
        if(0 == ext_out_prr_crpt(slot, parg, &gExtCtx))
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
}


int save_ref_prio(char slot, unsigned char *parg)
{
    int num, i, len;
    num = (int)(slot - 'a');
    if(slot_type[num] != 'S')
    {
        //
        send_online_framing(num, 'S');
        slot_type[num] = 'S';
    }
    len = strlen(parg);
    if(len == 9)
    {
        if(parg[0] == ',') //wu prio1
        {
            for(i = 0; i < 8; i++)
            {
                if((parg[i + 1] < 0x30) || (parg[i + 1] > 0x38))
                {
                    return 1;
                }
            }
            memcpy(rpt_content.slot[num].ref_prio2, &parg[1], 8);

        }
        else
        {
            for(i = 0; i < 8; i++)
            {
                if((parg[i] < 0x30) || (parg[i] > 0x38))
                {
                    return 1;
                }
            }
            memcpy(rpt_content.slot[num].ref_prio1, parg, 8);
        }

    }
    else if(len == 17)
    {
        for(i = 0; i < 8; i++)
        {
            if((parg[i + 9] < 0x30) || (parg[i + 9] > 0x38))
            {
                return 1;
            }
            if((parg[i] < 0x30) || (parg[i] > 0x38))
            {
                return 1;
            }
        }
        memcpy(rpt_content.slot[num].ref_prio2, &parg[9], 8);
        memcpy(rpt_content.slot[num].ref_prio1, parg, 8);
    }
    else
    {
        return 1;
    }

    return 0;
}

int save_ref_mod(char slot, unsigned char *parg)
{
    int num, len;
    num = (int)(slot - 'a');
    len = strlen(parg);
    if(slot_type[num] != 'S')
    {
        //
        send_online_framing(num, 'S');
        slot_type[num] = 'S';
    }
    if(len == 5)
    {
        if(parg[0] == ',')
        {
            if(((parg[2] == '0') || (parg[2] == '1')) &&
                    ((parg[4] > 0x2f) && (parg[4] < 0x39)))
            {
                memcpy(rpt_content.slot[num].ref_mod2, &parg[2], 3);
                //	send_ref_lev(num);
                get_out_lev(0, 0);
            }
            else
            {
                return 1;
            }
        }
        else
        {
            if(((parg[0] == '0') || (parg[0] == '1')) &&
                    ((parg[2] > 0x2f) && (parg[2] < 0x39)))
            {
                memcpy(rpt_content.slot[num].ref_mod1, &parg[0], 3);
                //send_ref_lev(num);
                get_out_lev(0, 0);
            }
            else
            {
                return 1;
            }
        }
    }
    else if(len == 7)
    {
        if(((parg[0] == '0') || (parg[0] == '1')) &&
                ((parg[2] > 0x2f) && (parg[2] < 0x39)))
        {}
        else
        {
            return 1;
        }
        if(((parg[4] == '0') || (parg[4] == '1')) &&
                ((parg[6] > 0x2f) && (parg[6] < 0x39)))
        {}
        else
        {
            return 1;
        }

        memcpy(rpt_content.slot[num].ref_mod1, &parg[0], 3);
        memcpy(rpt_content.slot[num].ref_mod2, &parg[4], 3);
        //send_ref_lev(num);
        get_out_lev(0, 0);
    }
    else
    {
        return 1;
    }

    return 0;
}

int save_ref_ph(char slot, unsigned char *parg)
{
    int num, i, cnt, j1;
    num = (int)(slot - 'a');
    cnt = i = j1 = 0;
    if(slot_type[num] != 'S')
    {
        //
        send_online_framing(num, 'S');
        slot_type[num] = 'S';
    }
    while(parg[i])
    {
        if(parg[i] == ',')
        {
            cnt++;
            if(cnt == 1)
            {
                j1 = i + 1;
            }
        }
        i++;
    }
    if(cnt != 8)
    {
        return 1;
    }
    memcpy(rpt_content.slot[num].ref_en, parg, 8);
    memset(rpt_content.slot[num].ref_ph, 0, 128);
    memcpy(rpt_content.slot[num].ref_ph, &parg[9], strlen(parg) - 9);
    return 0;
}

int save_ref_sta(char slot, unsigned char *parg)
{
    int num, i, len;

    num = (int)(slot - 'a');
    if(slot_type[num] != 'S')
    {
        //
        send_online_framing(num, 'S');
        slot_type[num] = 'S';
    }
    len = strlen(parg);
    if(len != 17)
    {
        return 1;
    }
    for(i = 0; i < 8; i++)
    {
        if((parg[i] < '@') && (parg[i] > 'A'))
        {
            return 1;
        }
    }
    for(i = 9; i < 8; i++)
    {
        if((parg[i] < 'a') && (parg[i] > 'g'))
        {
            return 1;
        }
    }

    memcpy(rpt_content.slot[num].ref_sa, parg, 8);
    memcpy(rpt_content.slot[num].ref_tl, &parg[9], 8);
    //send_ref_lev(num);
    get_out_lev(0, 0);

    return 0;
}


int save_ref_tpy(char slot, unsigned char *parg)
{
    int num, i;
    num = (int)(slot - 'a');
    if(slot_type[num] != 'S')
    {
        //
        send_online_framing(num, 'S');
        slot_type[num] = 'S';
    }
    for(i = 0; i < 8; i++)
    {
        if((parg[i] != '0') && (parg[i] != '1') && (parg[i] != '2'))
        {
            return 1;
        }
    }
    for(i = 0; i < 8; i++)
    {
        if((parg[i + 9] != '0') && (parg[i + 9] != '1'))
        {
            return 1;
        }
    }

    memcpy(rpt_content.slot[num].ref_typ1, parg, 8);
    memcpy(rpt_content.slot[num].ref_typ2, &parg[9], 8);
    return 0;
}

int save_ref_alm(char slot, unsigned char *parg)
{
    int num;
    unsigned char  tmp[14];
    memset(tmp, '\0', 14);
    num = (int)(slot - 'a');

    rpt_alm_to_client(slot, parg);
    sprintf((char *)tmp, "\"%c,S,%s\"", slot, parg);
    //printf("<save_ref_alm>:%s\n",tmp);
    if((parg[0] >> 4) == 0x05) 
    {
        if(num == 0) //ref1
        {
            memcpy(alm_sta[18], tmp, strlen(tmp));
        }
        else if(num == 1) //ref2
        {
            memcpy(alm_sta[19], tmp, strlen(tmp));
        }
    }
    else
    {
        memcpy(alm_sta[num], tmp, strlen(tmp));
    }

    return 0;
}

int save_ref_en(char slot, unsigned char *parg)
{
    int num, i;
    num = (int)(slot - 'a');
    if(slot_type[num] != 'S')
    {
        //
        send_online_framing(num, 'S');
        slot_type[num] = 'S';
    }
    for(i = 0; i < 8; i++)
    {
        if((parg[i] != '1') && (parg[i] != '0'))
        {
            return 1;
        }
    }
    memcpy(rpt_content.slot[num].ref_en, parg, 8);

    return 0;
}



int save_ref_ssm_en(char slot, unsigned char *parg)
{
    int num, i;
    num = (int)(slot - 'a');
    if(slot_type[num] != 'S')
    {
        //
        send_online_framing(num, 'S');
        slot_type[num] = 'S';
    }
    for(i = 0; i < 8; i++)
    {
        if(parg[i] < '0' || parg[i] > '2')
        {
            return 1;
        }
    }
    memcpy(rpt_content.slot[num].ref_ssm_en, parg, 8);

    return 0;
}

int save_ref_sa(char slot, unsigned char *parg)
{
    int num, i;
    num = (int)(slot - 'a');
    if(slot_type[num] != 'S')
    {
        //
        send_online_framing(num, 'S');
        slot_type[num] = 'S';
    }
    for(i = 0; i < 8; i++)
    {
        if((parg[i] < '@') && (parg[i] > 'E'))
        {
            return 1;
        }
    }
    memcpy(rpt_content.slot[num].ref_sa, parg, 8);

    return 0;
}

int save_ref_tl(char slot, unsigned char *parg)
{
    int num, i;
    num = (int)(slot - 'a');
    if(slot_type[num] != 'S')
    {
        //
        send_online_framing(num, 'S');
        slot_type[num] = 'S';
    }
    for(i = 0; i < 8; i++)
    {
        if((parg[i] < 'a') && (parg[i] > 'g'))
        {
            return 1;
        }
    }
    memcpy(rpt_content.slot[num].ref_tl, parg, 8);
    //send_ref_lev(num);
    get_out_lev(0, 0);
    return 0;
}

int save_rs_en(char slot, unsigned char *parg)
{
    int num, i;
    num = (int)(slot - 'a');
    /*if(slot_type[num]!='T')
    	{
    		//
    		send_online_framing(num,'T');
    		slot_type[num]='T';
    	}*/
    for(i = 0; i < 4; i++)
    {
        if((parg[i] != '1') && (parg[i] != '0'))
        {
            return 1;
        }
    }

    memcpy(rpt_content.slot[num].rs_en, parg, 4);

    return 0;
}

int save_rs_tzo(char slot, unsigned char *parg)
{
    int num, i;
    num = (int)(slot - 'a');
    /*if(slot_type[num]!='T')
    	{
    		//
    		send_online_framing(num,'T');
    		slot_type[num]='T';
    	}*/
    for(i = 0; i < 4; i++)
    {
        if((parg[i] < 0x40) || (parg[i] > 0x57))
        {
            return 1;
        }
    }

    memcpy(rpt_content.slot[num].rs_tzo, parg, 4);

    return 0;
}

int save_ppx_mod(char slot, unsigned char *parg)
{
    int num, i;
    num = (int)(slot - 'a');
    if(slot_type[num] != 'U')
    {
        //
        send_online_framing(num, 'U');
        slot_type[num] = 'U';
    }
    for(i = 0; i < 16; i++)
    {
        if((parg[i] != '1') && (parg[i] != '2') && (parg[i] != '3'))
        {
            return 1;
        }
    }

    memcpy(rpt_content.slot[num].ppx_mod, parg, 16);

    return 0;
}

int save_tod_en(char slot, unsigned char *parg)
{
    int num, i;
    num = (int)(slot - 'a');
    if(slot_type[num] != 'V')
    {
        //
        send_online_framing(num, 'V');
        slot_type[num] = 'V';
    }
    for(i = 0; i < 16; i++)
    {
        if((parg[i] != '1') && (parg[i] != '0'))
        {
            return 1;
        }
    }
    memcpy(rpt_content.slot[num].tod_en, parg, 16);

    return 0;
}
int save_tod_pro(char slot, unsigned char *parg)
{
    int num;
    num = (int)(slot - 'a');
    if(slot_type[num] != 'V')
    {
        //
        send_online_framing(num, 'V');
        slot_type[num] = 'V';
    }
    if((parg[0] < 0x30) || (parg[0] > 0x37))
    {
        return 1;
    }
    if((parg[2] < 0x40) || (parg[2] > 0x57))
    {
        return 1;
    }
    rpt_content.slot[num].tod_br = parg[0];
    rpt_content.slot[num].tod_tzo = parg[2];

    return 0;
}

int save_igb_en(char slot, unsigned char *parg)
{
    int num, i;
    num = (int)(slot - 'a');
    if(slot_type[num] != 'W')
    {
        //
        send_online_framing(num, 'W');
        slot_type[num] = 'W';
    }
    for(i = 0; i < 4; i++)
    {
        if((parg[i] != '1') && (parg[i] != '0'))
        {
            return 1;
        }
    }
    memcpy(rpt_content.slot[num].igb_en, parg, 4);

    return 0;
}
int save_igb_rat(char slot, unsigned char *parg)
{
    int num, i;
    num = (int)(slot - 'a');
    if(slot_type[num] != 'W')
    {
        //
        send_online_framing(num, 'W');
        slot_type[num] = 'W';
    }
    for(i = 0; i < 4; i++)
    {
        if((parg[i] < 'a') && (parg[i] > 'i'))
        {
            return 1;
        }
    }
    memcpy(rpt_content.slot[num].igb_rat, parg, 4);
    return 0;
}
int save_igb_tzone(char slot, unsigned char *parg)
{
    int num;
    num = (int)(slot - 'a');
    if(slot_type[num] != 'W')
    {
        send_online_framing(num, 'W');
        slot_type[num] = 'W';
    }
    rpt_content.slot[num].igb_tzone = parg[0];
    return 0;
}

int save_igb_max(char slot, unsigned char *parg)
{
    int num, i;
    num = (int)(slot - 'a');
    if(slot_type[num] != 'W')
    {
        //
        send_online_framing(num, 'W');
        slot_type[num] = 'W';
    }
    for(i = 0; i < 4; i++)
    {
        if((parg[i] < 'A') && (parg[i] > 'E'))
        {
            return 1;
        }
    }
    memcpy(rpt_content.slot[num].igb_max, parg, 4);
    return 0;
}
int check_net(char *parg,int len)
{
	int i;
	for(i=0;i<len;i++)
	{
	    if( (parg[i]&0xf0) != 0x40)
	    {
	    	return i;
	    }
	    else
	    {
	    }
	}
	return i;
}

int net_2_str(char h_4bit,char l_4bit)
{
    if((h_4bit & 0xf0) != 0x40 || (l_4bit & 0xf0) != 0x40)
    {
    	return -1;	
    }
    else
    {
    	return ( ((h_4bit&0x0f)<<4)|(l_4bit&0x0f) );
    }
}

void __save_ntp_pnet(unsigned char *dit,char * parg,int len)
{
	int i,res;
	unsigned char str[6];
	for(i=0;i<len;i+=2)
	{
		res = net_2_str(parg[i],parg[i+1]);
		if(res >= 0)
		{
			str[i/2] = (unsigned char)res;
		}
		else
		{
		    return;
		}
	}
	memcpy(dit,str,len/2);
}

int save_ntp_pnet(char slot, unsigned char *parg)
{
	int num, i,port,res;
	int net_sta = 0;
	int len;
	unsigned char net_char[13];
	
	
	num = (int)(slot - 'a');
	len = strlen(parg);
	port = 0;
	for(i=0;i<len;i++)
	{
		memset(net_char,0,13);
		if(parg[i]>='0' && parg[i]<='9')
		{
			net_sta = 0;
		}
		switch (net_sta)
		{
			case 0://port
				
				//(parg[i+1] != ',')?(port = (parg[i++]-'0')*10+(parg[i++]-'0') ):(port = parg[i++]-'0' );
				if(parg[i+1] != ',')
				{
					port = parg[i++]-'0';
					port = port *10 + parg[i++]-'0';
				
				}
				else
				{
					port = parg[i++]-'0';
				}
				port -=1;
				if(port < 0)
				{
					printf("port error\n");
					net_sta = 5;
					break;
				}
				
				net_sta = 1;
				break;
			case 1://mac
				
				res = check_net(parg+i,len-i+1);
				
				if(res == 12)
				{

					__save_ntp_pnet(rpt_content.slot[num].ntp_pmac[port],parg+i,res);
				}
				i = i+res;
				net_sta = 2;
				break;
			case 2:
				
				res = check_net(parg+i,len-i+1);
				
				if(res == 8)
				{
					__save_ntp_pnet(rpt_content.slot[num].ntp_pip[port],parg+i,res);
				}
				i = i+res;
				net_sta = 3;
				
				break;
			case 3:
				res = check_net(parg+i,len-i+1);
				
				if(res == 8)
				{
					__save_ntp_pnet(rpt_content.slot[num].ntp_pgateway[port],parg+i,res);
				}
				i = i+res;
				net_sta = 4;
				
				break;
			case 4:
				res = check_net(parg+i,len-i+1);
				
				if(res == 8)
				{
					__save_ntp_pnet(rpt_content.slot[num].ntp_pnetmask[port],parg+i,res);
				}
				i = i+res;
				net_sta = 1;
				break;
			case 5:
				i = len+1;
				break;
			default:
				break;
		}
		
	}
	return 0;
}

int save_ntp_net(char slot, unsigned char *parg)
{
    int num, i;
    int j, i1, i2, i3, len;
    j = i1 = i2 = i3 = 0;
    num = (int)(slot - 'a');
    len = strlen(parg);
    if(slot_type[num] != 'Z')
    {
        //
        send_online_framing(num, 'Z');
        slot_type[num] = 'Z';
    }


    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
    if(parg[0] != ',')
    {
        memcpy(rpt_content.slot[num].ntp_ip, &parg[0], i1 - 1);
    }
    if(parg[i1] != ',')
    {
        memcpy(rpt_content.slot[num].ntp_mac, &parg[i1], i2 - i1 - 1);
    }
    if(parg[i2] != ',')
    {
        memcpy(rpt_content.slot[num].ntp_gate, &parg[i2], i3 - i2 - 1);
    }
    if(parg[i3] != '\0')
    {
        memcpy(rpt_content.slot[num].ntp_msk, &parg[i3], len - i3);
    }



    return 0;
}

int save_ntp_en(char slot, unsigned char *parg)
{
    int num, i;
    int j, i1, i2, i3, i4, len;
    j = i1 = i2 = i3 = i4 = 0;
    num = (int)(slot - 'a');
    len = strlen(parg);
    if(slot_type[num] != 'Z')
    {
        //
        send_online_framing(num, 'Z');
        slot_type[num] = 'Z';
    }
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
    if((parg[0] == '0') || (parg[0] == '1'))
    {
        rpt_content.slot[num].ntp_bcast_en = parg[0];
    }
    if(parg[i1] != ',')
    {
		memset(rpt_content.slot[num].ntp_interval, 0, sizeof(rpt_content.slot[num].ntp_interval));
        memcpy(rpt_content.slot[num].ntp_interval, &parg[i1], i2 - i1 - 1);
    }
    if((parg[i2] > 0x3f) && (parg[i2] < 0x48))
    {
        rpt_content.slot[num].ntp_tpv_en = parg[i2];
    }
    if((parg[i3] == '0') || (parg[i3] == '1'))
    {
        rpt_content.slot[num].ntp_md5_en = parg[i3];
    }
/*
    if((parg[i4] > 0x3f) && (parg[i4] < 0x48))
    {
        rpt_content.slot[num].ntp_ois_en = parg[i4];
    }
*/
    return 0;
}


int save_r_ntp_key(char slot, unsigned char *parg)
{
    int num, i;
    int len, i1, i2, i3, i4, i5, i6, i7, j;
    len = i1 = i2 = i3 = i4 = i5 = i6 = i7 = j = 0;
    num = (int)(slot - 'a');
    len = strlen(parg);
    if(slot_type[num] != 'Z')
    {
        //
        send_online_framing(num, 'Z');
        slot_type[num] = 'Z';
    }
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
            if(j == 5)
            {
                i5 = i + 1;
            }
            if(j == 6)
            {
                i6 = i + 1;
            }
            if(j == 7)
            {
                i7 = i + 1;
            }
        }
    }
    if(i7 == 0)
    {
        return 1;
    }
    memset(rpt_content.slot[num].ntp_keyid, '\0', 20);
    memset(rpt_content.slot[num].ntp_key, '\0', 68);
    if((parg[0] != ',') && (parg[i1] != ','))
    {
        memcpy(rpt_content.slot[num].ntp_keyid[0], &parg[0], i1 - 1);
        memcpy(rpt_content.slot[num].ntp_key[0], &parg[i1], i2 - i1 - 1);
    }
    if((parg[i2] != ',') && (parg[i3] != ','))
    {
        memcpy(rpt_content.slot[num].ntp_keyid[1], &parg[i2], i3 - i2 - 1);
        memcpy(rpt_content.slot[num].ntp_key[1], &parg[i3], i4 - i3 - 1);
    }
    if((parg[i4] != ',') && (parg[i5] != ','))
    {
        memcpy(rpt_content.slot[num].ntp_keyid[2], &parg[i4], i5 - i4 - 1);
        memcpy(rpt_content.slot[num].ntp_key[2], &parg[i5], i6 - i5 - 1);
    }
    if((parg[i6] != ',') && (parg[i7] != '\0'))
    {
        memcpy(rpt_content.slot[num].ntp_keyid[3], &parg[i6], i7 - i6 - 1);
        memcpy(rpt_content.slot[num].ntp_key[3], &parg[i7], len - i7);
    }

    return 0;
}

int save_ntp_key(char slot, unsigned char *parg)
{
    int num, i;
    unsigned char keyid[5];

    memset(keyid, '\0', 5);
    num = (int)(slot - 'a');
    if(slot_type[num] != 'Z')
    {
        //
        send_online_framing(num, 'Z');
        slot_type[num] = 'Z';
    }

    if((parg[0] != '0') && (parg[0] != '1') && (parg[0] != '2'))
    {
        return 1;
    }
    else if(parg[1] != ',')
    {
        return 1;
    }
    else if(parg[6] != ',')
    {
        return 1;
    }
    memcpy(keyid, &parg[2], 4);
    if(parg[0] == '0')
    {
        for(i = 0; i < 4; i++)
        {
            //if(rpt_content.slot[num].ntp_key_flag[i]!=2)
            if(strncmp1(rpt_content.slot[num].ntp_keyid[i], keyid, 4) == 0)
            {
                return 1;
            }
            else if(rpt_content.slot[num].ntp_keyid[i][0] == '\0')
            {
                //	rpt_content.slot[num].ntp_key_flag[i]=2;
                memcpy(rpt_content.slot[num].ntp_keyid[i], &parg[2], 4);
                memcpy(rpt_content.slot[num].ntp_key[i], &parg[7], 16);
                break;
            }

            else if(i == 3)
            {
                return 1;
            }
        }
    }
    else if(parg[0] == '1')
    {
        for(i = 0; i < 4; i++)
        {
            if(strncmp1(keyid, rpt_content.slot[num].ntp_keyid[i] , 4) == 0)
            {
                //rpt_content.slot[num].ntp_key_flag[i]=1;
                memset(rpt_content.slot[num].ntp_keyid[i], '\0', 5);
                memset(rpt_content.slot[num].ntp_key[i], '\0', 17);
                break;
            }
            else if(i == 3)
            {
                return 1;
            }
        }
    }
    else if(parg[0] == '2')
    {
        for(i = 0; i < 4; i++)
        {
            if(strncmp1(keyid, rpt_content.slot[num].ntp_keyid[i] , 4) == 0)
            {
                memset(rpt_content.slot[num].ntp_key[i], '\0', 17);
                memcpy(rpt_content.slot[num].ntp_key[i], &parg[7], 16);
                break;
            }
            else if(i == 3)
            {
                return 1;
            }
        }
    }

    return 0;
}

int save_ntp_ms(char slot, unsigned char *parg)
{
    int num, len;
    num = (int)(slot - 'a');
    len = strlen(parg);
    if(slot_type[num] != 'Z')
    {
        //
        send_online_framing(num, 'Z');
        slot_type[num] = 'Z';
    }
    if((parg[0] != '0') && (parg[0] != '1'))
    {
        return 1;
    }
    if(len < 4)
    {
        return 1;
    }
    memcpy(rpt_content.slot[num].ntp_ms, parg, len);

    return 0;
}
int save_ntp_sta(char slot, unsigned char *parg)
{
	int num, i;
	int j, i1, i2, i3, len;
	j = i1 = i2 = i3 = 0;
	num = (int)(slot - 'a');
	len = strlen(parg);
	if(0 == len)
	{
		return 1;
	}
	for(i = 0; i < len; i++)
	{
		if(parg[i] == ',')
		{
			j++;
			if(j == 1)
			{
				i1 = i + 1;
			}

		}
	}
	if(i1 == 0)
	{
		memset(rpt_content.slot[num].ntp_leap,0,sizeof(rpt_content.slot[num].ntp_leap));
		memcpy(rpt_content.slot[num].ntp_leap, &parg[0], len);

		
	}
	else
	{
		if(parg[0] != ',')
		{
			memset(rpt_content.slot[num].ntp_leap,0,sizeof(rpt_content.slot[num].ntp_leap));
			memcpy(rpt_content.slot[num].ntp_leap, &parg[0], i1 - 1);
		}
		if(parg[i1] != '\0')
		{
			memset(rpt_content.slot[num].ntp_mcu,0,sizeof(rpt_content.slot[num].ntp_mcu));
			memcpy(rpt_content.slot[num].ntp_mcu, &parg[i1], len - i1);
		}
	}
	return 0;
}

int save_ntp_psta(char slot, unsigned char *parg)
{
	int num;
	num = (int)(slot - 'a');
	if(slot_type[num] != 'Z')
	{
		send_online_framing(num, 'Z');
		slot_type[num] = 'Z';
	}
	memset(rpt_content.slot[num].ntp_portEn,0,sizeof(rpt_content.slot[num].ntp_portEn));
	memset(rpt_content.slot[num].ntp_portStatus,0,sizeof(rpt_content.slot[num].ntp_portStatus));
	
	rpt_content.slot[num].ntp_portEn[0] = parg[0];
	rpt_content.slot[num].ntp_portEn[1] = parg[1];
	rpt_content.slot[num].ntp_portEn[2] = parg[2];
	rpt_content.slot[num].ntp_portEn[3] = parg[3];
	
	rpt_content.slot[num].ntp_portStatus[0] = parg[5];
	rpt_content.slot[num].ntp_portStatus[1] = parg[6];
	rpt_content.slot[num].ntp_portStatus[2] = parg[7];
	rpt_content.slot[num].ntp_portStatus[3] = parg[8];

	return 0;
}



int save_ntp_alm(char slot, unsigned char *parg)
{
    int num;
    unsigned char  tmp[14];
    memset(tmp, '\0', 14);
    rpt_alm_to_client(slot, parg);
    num = (int)(slot - 'a');
    sprintf((char *)tmp, "\"%c,Z,%s\"", slot, parg);
    memcpy(alm_sta[num], tmp, strlen(tmp));
    return 0;
}
int save_2mb_lvlm(char slot, unsigned char *parg)
{
    int num;
    num = (int)(slot - 'a');
    /*if(slot_type[num]!='T')
    	{
    		//
    		send_online_framing(num,'T');
    		slot_type[num]='T';
    	}*/
    //for(i = 0; i < 4; i++)
    //{
    //    if((parg[i] < 0x40) || (parg[i] > 0x57))
    //    {
      if((parg[0] != '1')&&(parg[0] != '0'))
            return 1;
    //   }
    //}

    rpt_content.slot[num].ref_2mb_lvlm = parg[0];

    return 0;
}

int save_rb_sa(char slot, unsigned char *parg)
{
    if((parg[0] < '@') || (parg[0] > 'E'))
    {
        return 1;
    }
    if(slot == RB1_SLOT)
    {
        rpt_content.slot_o.rb_sa = parg[0];
    }
    else if(slot == RB2_SLOT)
    {
        rpt_content.slot_p.rb_sa = parg[0];
    }

    return 0;
}
int save_ok(char slot, unsigned char *parg)
{
    
    return 0;
}

/*
璁剧疆闂扮锛?
鎴愬姛0锛?
澶辫触1锛?
*/
/*
struct StrTime
{
	unsigned int hour;
	unsigned int minute;
	unsigned int second;
};

struct StrDate
{
	unsigned int year;
	unsigned int month;
	unsigned int day;
};


struct StrTime msgTime;
struct StrDate msgDate;
struct StrTime mcpTime;
struct StrDate mcpDate;


int getLeapDate(char * msg)
{
	int i=0;
	int len=0;
	int cnt = 0;
	char tmp[11];

	get_time_fromnet();

	while(mcp_date[i]!='\0' && i<11)
	{
		if('-' == mcp_date[i])
		{
			memset(tmp,0,11);
			if(cnt == 0)
			{
				cnt ++;
				memcpy(tmp,&mcp_date[i-4],4);
				mcpDate.year = atoi(tmp);
			}
			else if(cnt == 1)
			{
				cnt ++;
				memcpy(tmp,&mcp_date[i-2],2);
				mcpDate.month= atoi(tmp);
			}
			else if(cnt == 2)
			{
				cnt ++;
				memcpy(tmp,&mcp_date[i-2],2);
				mcpDate.day= atoi(tmp);
				break;

			}
			else
			{
				printf("%s mcp date error\n",__FUNCTION__);
				return 0;
			}
		}
		i++;
	}

	i=0;
	cnt = 0;

	while(mcp_time[i]!='\0' && i<11)
	{
		if(':' == mcp_time[i])
		{
			memset(tmp,0,11);
			if(cnt == 0)
			{
				cnt ++;
				memcpy(tmp,&mcp_time[i-2],2);
				mcpTime.hour = atoi(tmp);
			}
			else if(cnt == 1)
			{
				cnt ++;
				memcpy(tmp,&mcp_time[i-2],2);
				mcpTime.minute= atoi(tmp);

			}
			else if( 2 == cnt )
			{
				cnt ++;
				memcpy(tmp,&mcp_time[i-2],2);
				mcpTime.second= atoi(tmp);
				break;
			}
			else
			{
				//printf("%s mcp date error\n",__FUNCTION__);
				//return 0;
			}
		}
		i++;
	}

//L,235960,31,12,2012,15,16,01;
	i = 0;
	cnt = 0;

	while(msg[i] != '\0' && i < len)
	{
		if(',' == msg[i])
		{
			memset(tmp,0,11);
			if( cnt == 0)
			{
				cnt ++;

			}
			else if( cnt == 1)
			{
				cnt++;
				memset(tmp,0,11);
				memcpy(tmp,&msg[i-6],2);
				msgTime.hour = atoi(tmp);

				memset(tmp,0,11);
				memcpy(tmp,&msg[i-4],2);
				msgTime.minute= atoi(tmp);

				memset(tmp,0,11);
				memcpy(tmp,&msg[i-2],2);
				msgTime.second= atoi(tmp);
			}
			else if( 2 == cnt)
			{
				cnt ++;
				memset(tmp,0,11);
				memcpy(tmp,&msg[i-2],2);
				msgDate.day= atoi(tmp);
			}
			else if( 3 == cnt )
			{
				cnt ++;
				memset(tmp,0,11);
				memcpy(tmp,&msg[i-2],2);
				msgDate.month= atoi(tmp);
			}
			else if( 4 == cnt)
			{
				cnt ++;
				memset(tmp,0,11);
				memcpy(tmp,&msg[i-4],4);
				msgDate.year= atoi(tmp);
			}
			else
			{
				printf("leap msg error \n");
				return 1;
			}
		}
	}
}
*/
int save_leap_alm(char slot, unsigned char *parg)
{

    int i;
    int cnt = 0;
    int len;

    char msg[64];
    char tmp[11];
    char leapTime[20];
    char leapDate[20];

    len = strlen(parg);
	if(len > 64)
		return 1;
	
    memset(msg, 0, 64);
    memcpy(msg, parg, len);
    printf("@#%s\n", __FUNCTION__);
    if(g_LeapFlag != 0)
    {
        return 1;
    }
    else
    {
        ;
    }

    for(i = 0; i < len; i++)
    {
        if(',' == msg[i])
        {
            cnt++;
        }
        else
        {
            continue;
        }
    }

    if(cnt != 6)
    {
        return 1;
    }
    else
    {
        memset(tmp, 0, 11);
        memset(leapDate, 0, 20);
        memset(leapTime, 0, 20);
        i = 0;
        cnt = 0;
    }

    while(msg[i] != '\0' && i < len)
    {
        if(',' == msg[i])
        {
            memset(tmp, 0, 11);

            if( cnt == 0)
            {
                cnt++;
                memset(tmp, 0, 11);
                memcpy(tmp, &msg[i - 6], 2);
                sprintf((char *)leapTime, "%s", tmp);

                memset(tmp, 0, 11);
                memcpy(tmp, &msg[i - 4], 2);
                sprintf((char *)leapTime, "%s:%s", leapTime, tmp);

                memset(tmp, 0, 11);
                memcpy(tmp, &msg[i - 2], 2);
                sprintf((char *)leapTime, "%s:%s", leapTime, tmp);
            }
            else if( 1 == cnt)
            {
                cnt ++;
                memset(tmp, 0, 11);
                memcpy(tmp, &msg[i - 2], 2);
                sprintf((char *)leapDate, "%s", tmp);
            }
            else if( 2 == cnt )
            {
                cnt ++;
                memset(tmp, 0, 11);
                memcpy(tmp, &msg[i - 2], 2);
                sprintf((char *)tmp, "%s-%s", tmp, leapDate);
                memset(leapDate , 0, 20);
                memcpy(leapDate, tmp, strlen(tmp));
            }
            else if( 3 == cnt)
            {
                cnt ++;
                memset(tmp, 0, 11);
                memcpy(tmp, &msg[i - 4], 4);
                sprintf((char *)tmp, "%s-%s", tmp, leapDate);
                memset(leapDate , 0, 20);
                memcpy(leapDate, tmp, strlen(tmp));
            }
            else if(5 == cnt)
            {
                cnt++;
				
                //R-LEAP-ALM :L,235960,31,12,2012,15,16,01;
                ;
            }
            else if(4 == cnt)
            {
                cnt++;
                memset(tmp, 0, 11);
                memcpy(tmp, &msg[i - 2], 2);
                if( (tmp[0] >= '0' && tmp[0] <= '9') &&
                        (tmp[1] >= '0' && tmp[1] <= '9') )
                {
                    memset(conf_content.slot_u.leap_num, '\0', 3);
                    memcpy(conf_content.slot_u.leap_num, tmp, 2);
                }
                else
                {
                    return 1;
                }
            }
        }
        i++;
    }

	if((msg[len-2] == '0')&&((msg[len-1] == '0')||(msg[len-1] == '1')||(msg[len-1] == '2')))
	{
	   rpt_content.slot_q.leapalm[0] = msg[len-2];
	   rpt_content.slot_q.leapalm[1] = msg[len-1];
	   rpt_content.slot_q.leapalm[2] = '\0';
	}
	else
	{
	   rpt_content.slot_q.leapalm[0] = '0';
	   rpt_content.slot_q.leapalm[1] = '0';
	   rpt_content.slot_q.leapalm[2] = '\0';
	}
#if 0
    get_time_fromnet();
    ret = strncmp1( mcp_date, leapDate, strlen(mcp_date));
    if( ret <= 0)
    {
        g_LeapFlag = 1;
    }
    else
    {
        g_LeapFlag = 0;
        return 1;
    }

    ret = strncmp1( mcp_time, leapTime, strlen(mcp_time));
    if( ret <= 0)
    {
        g_LeapFlag = 1;
    }
    else
    {
        g_LeapFlag = 0;
        return 1;
    }
#endif
    g_LeapFlag = 1;

    printf("###%s\n", conf_content.slot_u.leap_num);
    return 0;

}

void sta1_alm(unsigned char *sendbuf, unsigned char slot)
{
    if( 1 == (alm_sta[20][1] & 0x01) )
    {
        memset(sendbuf, '\0', SENDBUFSIZE);
        sprintf((char *)sendbuf, "SET-STA1:000000::MONT/GGIO1/Alm=1;");
        sendtodown(sendbuf, slot);
    }
    if( 0x02 == (alm_sta[20][1] & 0x02))
    {
        memset(sendbuf, '\0', SENDBUFSIZE);
        sprintf((char *)sendbuf, "SET-STA1:000000::MONT/GGIO1/Alm1=1;");
        sendtodown(sendbuf, slot);
    }
}

void sta1_portnum(unsigned char *sendbuf, unsigned char slot)
{
    unsigned char i;
    unsigned char portNum = 0;
    for(i = 0; i < 14; i++)
    {
        if( 'O' != slot_type[i] )
        {
            portNum++;
        }
    }

    //memset(sendbuf, '\0', SENDBUFSIZE);
    sprintf((char *)sendbuf, "SET-STA1:000000::MONT/LLN0/PortNum=%d;", portNum);
    sendtodown(sendbuf, slot);
}

void sta1_net(unsigned char *sendbuf, unsigned char slot)
{
#if 1

    unsigned char  net_buf[50];

    unsigned int net_ip;

    int s;
    struct ifreq ifr;
    struct sockaddr_in *ptr;
    unsigned char *interface_name = "eth0";
    if((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket");
        return;
    }

    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);

    if(ioctl(s, SIOCGIFADDR, &ifr) < 0)
    {
        perror("Get_McuLocalIp ioctl");
        close(s);
    }
    ptr = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_addr;
    net_ip = (unsigned int)(ptr->sin_addr.s_addr);
    //strcpy(mcp_local_ip,(unsigned char *)inet_ntoa (ptr->sin_addr));
    close(s);


    //IPAddr
    //memset(net_buf,0,50);
    //API_Get_McpIp(net_buf);
    memset(sendbuf, '\0', SENDBUFSIZE);
    sprintf((char *)sendbuf, "SET-STA1:000000::MONT/LLN0/IPAddr=%x;", net_ip);
    sendtodown(sendbuf, slot);
    //MACAddr
    memset(net_buf, 0, 50);
    memset(sendbuf, '\0', SENDBUFSIZE);
    API_Get_McpMacAddr(net_buf);
    sprintf((char *)sendbuf, "SET-STA1:000000::MONT/LLN0/MACAddr=%s;", net_buf);
    sendtodown(sendbuf, slot);
#endif

}


void sta1_cpu(unsigned char *sendbuf, unsigned char slot)
{
    //CpuUsage
    //memset(sendbuf, '\0', SENDBUFSIZE);
    sprintf((char *)sendbuf, "SET-STA1:000000::MONT/LLN0/CpuUsage=67;");
    sendtodown(sendbuf, slot);
}

void sta1_in_out(unsigned char *sendbuf, unsigned char slot)
{
    unsigned char temp = 0;
    unsigned char i = 0;



    //00010001
    //GPS BD
    if('O' != slot_type[16])
    {
        if( (strncmp(rpt_content.slot_q.gb_acmode, "BD", 2) == 0) )
        {
            temp |= 0x01;
        }
        else
        {
            temp |= 0x10;
        }
    }
    if('O' != slot_type[17] )
    {
        if( (strncmp(rpt_content.slot_r.gb_acmode, "BD", 2) == 0) )
        {
            temp |= 0x01;
        }
        else
        {
            temp |= 0x10;
        }
    }
    //memset(sendbuf, '\0', SENDBUFSIZE);
    sprintf((char *)sendbuf, "SET-STA1:000000::MONT/LLN0/InType=");
    if((temp & 0x10) == 0x10)
    {
        sprintf((char *)sendbuf, "%sGPS ", sendbuf);
    }
    if((temp & 0x01) == 0x01)
    {
        sprintf((char *)sendbuf, "%sBD ", sendbuf);
    }

    temp = strlen(sendbuf);
    if(sendbuf[temp - 1] == ' ')
    {
        sendbuf[temp - 1] = ';';
    }

    if(sendbuf[temp - 1] == ';')
    {
        sendtodown(sendbuf, slot);
    }

    temp = 0;
    //111000
    //b鐮?588 pps+tod
    for(i = 0; i < 14; i++)
    {
        if('W' == slot_type[i])
        {
            temp |= 0x80;
        }

        if( (slot_type[i] >= 'A' && slot_type[i] <= 'H')
                || (slot_type[i] >= 'n' && slot_type[i] <= 'q') )
        {
            temp |= 0x40;
        }

        if( 'I' == slot_type[i] )
        {
            temp |= 0x20;
        }
    }
    memset(sendbuf, '\0', SENDBUFSIZE);
    sprintf((char *)sendbuf, "SET-STA1:000000::MONT/LLN0/OutType=");
    if( (temp & 0x80) == 0x80)
    {
        sprintf((char *)sendbuf, "%sB鐮?", sendbuf);
    }

    if( (temp & 0x40) == 0x40 )
    {
        sprintf((char *)sendbuf, "%s1588 ", sendbuf);
    }

    if( (temp & 0x20) == 0x20 )
    {
        sprintf((char *)sendbuf, "%s1PPS+TOD ", sendbuf);
    }

    temp = strlen(sendbuf);
    if(sendbuf[temp - 1] == ' ')
    {
        sendbuf[temp - 1] = ';';
    }

    if(sendbuf[temp - 1] == ';')
    {
        sendtodown(sendbuf, slot);
    }

}



void send_61850_sta(unsigned char slot)
{


    unsigned char sendbuf[125];

    sta1_alm(sendbuf, slot);


    sta1_portnum(sendbuf, slot);


    sta1_net(sendbuf, slot);


    sta1_cpu(sendbuf, slot);

    sta1_in_out(sendbuf, slot);


}

int save_sta1_alm(char slot, unsigned char *parg)
{
    int num;
    unsigned char  tmp[14];

    if(slot < 'a' || slot > 'n')
    {
        return -1;
    }
    else
    {
        num = (int)(slot - 'a');
    }

    memset(tmp, '\0', 14);
    rpt_alm_to_client(slot, parg);
    sprintf((char *)tmp, "\"%c,m,%s\"", slot, parg);
    memcpy(alm_sta[num], tmp, strlen(tmp));

    send_61850_sta(slot);
    return 0;
}
//
//!PGEIN单盘输入100S上报保存网络参数/事件CD1
//
int save_pgein_net(char slot, unsigned char *parg)
{
    int num, port;
    int i, j, i1, i2, i3,len;
    j = i1 = i2 = i3  = 0;
    num = (int)(slot - 'a');
    port = (int)(parg[0] - 0x31);
    len = strlen(parg);
    //
    //!PI NET PRINTF .
    //
	//printf("PI NET %s\n",parg);	
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
        return (1);
    }
	//
	//! ip address
	//
    if(parg[i1] != ',')
    {
        memcpy(rpt_content.slot[num].ptp_ip[port], &parg[i1], i2 - i1 - 1);
    }
    //
    //! gate address
    //
    if(parg[i2] != ',')
    {
        memcpy(rpt_content.slot[num].ptp_gate[port], &parg[i2], i3 - i2 - 1);
    }
    //
    //! mask address
    //
    if(parg[i3] != '\0')
    {
        memcpy(rpt_content.slot[num].ptp_mask[port], &parg[i3], len - i3);
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    printf("PI NET %s:%s,%s,%s#\n", NET_SAVE, rpt_content.slot[num].ptp_ip[port],
 											  rpt_content.slot[num].ptp_gate[port],
                							  rpt_content.slot[num].ptp_mask[port]);
#endif
    return 0; 
        

}

//
//! PGEIN 单盘输入 工作模式100S上报/事件CD2
//
int save_pgein_mod(char slot, unsigned char *parg)
{
   int i, j, i1, i2, i3, i4, i5, i6, len;
   int num, port;
   j = i1 = i2 = i3 = i4 = i5 = i6 = 0;
   num = (int)(slot - 'a');
   port = (int)(parg[0] - 0x31);
   len = strlen(parg);
   //
   //! PGEIN
   //
   //printf("PI MOD %s\n",parg);
   for(i = 0; i < len; i++)
   {
        if(parg[i] == ',')
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
            if(j == 5)
            {
                i5 = i + 1;
            }
            if(j == 6)
            {
                i6 = i + 1;
            }
            
        }
    }
    if(i6 == 0)
    {
        return (1);
    }
    //
    //! E2E/P2P
    //
    if((parg[i1] == '1') || (parg[i1] == '0'))
    {
        
        rpt_content.slot[num].ptp_delaytype[port] = parg[i1];
		
		rpt_content.slot[num].ptp_delaytype[1] = '\0';
		//
		//!PGEIN
		//
		//printf("PI Delaytype %d,%c\n",port,rpt_content.slot[num].ptp_delaytype[port]);
    }
	//
	//! multicast /uniticast
	//
    if((parg[i2] == '1') || (parg[i2] == '0'))
    {
        rpt_content.slot[num].ptp_multicast[port] = parg[i2];
		rpt_content.slot[num].ptp_multicast[1] = '\0';
    }
	//
	//!enq two /three
	//
    if((parg[i3] == '1') || (parg[i3] == '0'))
    {
        rpt_content.slot[num].ptp_enp[port] = parg[i3];
		rpt_content.slot[num].ptp_enp[1] = '\0';
    }
	//
	//! one step/two step
	//
    if((parg[i4] == '1') || (parg[i4] == '0'))
    {
        rpt_content.slot[num].ptp_step[port] = parg[i4];
		rpt_content.slot[num].ptp_step[1] = '\0';
		
    }
    //
    //!delay req fre
    //
    if(parg[i5] != ',')
    {
        memcpy(rpt_content.slot[num].ptp_delayreq[port], &parg[i5], i6 - i5 - 1);
		rpt_content.slot[num].ptp_delayreq[port][2] = '\0';
    }
	//
	//!pdelay req fre
	//
    if(parg[i6] != '\0')
    {
        memcpy(rpt_content.slot[num].ptp_pdelayreq[port], &parg[i6], len - i6);
		rpt_content.slot[num].ptp_pdelayreq[port][2] = '\0';
    }
    
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("PI MOD %s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
        
}

//
//!PGEIN单盘输入参考输入信号质量变化事件CD3
//
int save_pgein_clkclass(char slot, unsigned char *parg)
{
    int num, port;
    int i, j, i1,len;
    j = i1 = 0;
    num = (int)(slot - 'a');
    port = (int)(parg[0] - 0x31);
    len = strlen(parg);
	//
	//! clock class
	//
	//printf("PI clkcls %s\n",parg);
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
        {
            j++;
            if(j == 1)
            {
                i1 = i + 1;
            }
            
           
        }
    }
    if(i1 == 0)
    {
        return (1);
    }
	//
	//! clkclass 
	//
    if(parg[i1] != '\0')
    {
        memcpy(rpt_content.slot[num].ptp_clkcls[port], &parg[i1], len - i1);
		rpt_content.slot[num].ptp_clkcls[port][3] = '\0';
    }
   
#if DEBUG_NET_INFO
    if(print_switch == 0)
    printf("PI CLK %s:%s#\n", NET_SAVE, rpt_content.slot[num].ptp_clkcls[port]);
#endif
    return 0; 
        

}


//
//! 100S上报报警
//
int save_pgein_alm(char slot, unsigned char *parg)
{
    int num;
    unsigned char tmp[14];
    memset(tmp, '\0', 14);
    rpt_alm_to_client(slot, parg);
    num = (int)(slot - 'a');
    sprintf((char *)tmp, "\"%c,%c,%s\"", slot, slot_type[num], parg);
    memcpy(alm_sta[num], tmp, strlen(tmp));
	
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("PGEIN %s:%s#\n", NET_SAVE, parg);
    }
#endif
    return 0;
        

}

//
//!PGE4V2单盘输出100S上报
//
int save_pge4v_net(char slot, unsigned char *parg)
{
    int num, port;
    int i, j, i1, i2, i3, i4,len;
	unsigned char datalen;
	unsigned char errflag;
	datalen = errflag = 0;
    j = i1 = i2 = i3  =  i4 = 0;
    num = (int)(slot - 'a');
    port = (int)(parg[0] - 0x31);
    len = strlen(parg);
    //
    //slot range 0 ~ 13。
    //
	if((num < 0) || (num > 13))
		return 1;
    //
    //port range 0 ~ 3。
    //
	if((port < 0) ||(port > 3))
		return 1;
		
   
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
	//
	//error ,return 1。
    if(i4 == 0)
    {
        return (1);
    }
    //
	//Judge the data len or not correct.
	//
	datalen = i2 - i1 -1; 
	if(datalen >  MAX_MAC_LEN)
		errflag = 1;
			
	datalen = i3 - i2 -1; 
	if(datalen >  MAX_IP_LEN)
		errflag = 1;
			
	datalen = i4 - i3 -1;
	if(datalen >  MAX_IP_LEN)
		errflag = 1;

	datalen = len - i4;
	if(datalen >  MAX_IP_LEN)
		errflag = 1;

	if(1 == errflag)
		return 1;
	//
	//! mac address
	//
    if(parg[i1] != ',')
    {
        memset(rpt_content.slot[num].ptp_mac[port],'\0',18);
        memcpy(rpt_content.slot[num].ptp_mac[port], &parg[i1], i2 - i1 - 1);  
    }
    //
    //! ip address
    //
    if(parg[i2] != ',')
    {
        memset(rpt_content.slot[num].ptp_ip[port],'\0',MAX_IP_LEN + 1);
        memcpy(rpt_content.slot[num].ptp_ip[port],  &parg[i2], i3 - i2 - 1);
    }
    //
    //! mask address
    //
    if(parg[i3] != ',')
    {
        memset(rpt_content.slot[num].ptp_mask[port],'\0',MAX_IP_LEN + 1);
        memcpy(rpt_content.slot[num].ptp_mask[port], &parg[i3], i4 - i3 - 1);
    }
	//
	//! gate address
	//
	if(parg[i4] != '\0')
	{
		memset(rpt_content.slot[num].ptp_gate[port],'\0',MAX_IP_LEN + 1);
		memcpy(rpt_content.slot[num].ptp_gate[port], &parg[i4], len - i4);
	}

#if DEBUG_NET_INFO
    if(print_switch == 0)
    printf("PGE4S NET %s:%s,%s,%s,%s#\n", NET_SAVE, rpt_content.slot[num].ptp_mac[port],
    										  rpt_content.slot[num].ptp_ip[port],
 											  rpt_content.slot[num].ptp_gate[port],
                							  rpt_content.slot[num].ptp_mask[port]);
#endif
    return 0; 
        

}
//
//!PGE4S单盘输出工作模式100S上报
//
int save_pge4v_mod(char slot, unsigned char *parg)
{
    int num, port;
    int i, j, i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13,len;
    unsigned char datalen;
	datalen = 0;
    j = i1 = i2 = i3  =  i4 = i5 = i6 = 0;
	i7 = i8 = i9 = i10 = i11 = i12 = i13 = 0;
	//
	//num , port , len ;
    num = (int)(slot - 'a');
    port = (int)(parg[0] - 0x31);
    len = strlen(parg);
    //
    //slot range 0 ~ 13。
    //
	if((num < 0) || (num > 13))
		return 1;
    //
    //port range 0 ~ 3。
    //
	if((port < 0) ||(port > 3))
		return 1;
		
   
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
			if(j == 5)
			{
			    i5 = i + 1;
			}
			if(j == 6)
			{
			    i6 = i + 1;
			}
			if(j == 7)
            {
                i7 = i + 1;
            }
			if(j == 8)
			{
			    i8 = i + 1;
			}
			if(j == 9)
			{
			    i9 = i + 1;
			}
			if(j == 10)
			{
			    i10 = i + 1;
			}
			if(j == 11)
			{
			    i11 = i + 1;
			}
			if(j == 12)
			{
			    i12 = i + 1;
			}
			if(j == 13)
			{  
			    i13 = i + 1;
			}
			
           
        }
    }
	//
	//error ,return 1。
    if(i12 == 0)
    {
        return (1);
    }
   
	//
	//! enable。
	//
    if((parg[i1] == '0') || (parg[i1] == '1'))
    {
        rpt_content.slot[num].ptp_en[port] = parg[i1];  
    }
    //
    //! esmc enable。
    //
    if((parg[i2] == '0') || (parg[i2] == '1'))
    {
        rpt_ptp_mtc.esmc_en[num][port] = parg[i2];
    }
    //
    //! delaytype 
    //
    if((parg[i3] == '0') || (parg[i3] == '1'))
    {
        rpt_content.slot[num].ptp_delaytype[port] = parg[i3];
    }
	//
	//! multicast
	//
	if((parg[i4] == '0') || (parg[i4] == '1'))
	{
		rpt_content.slot[num].ptp_multicast[port] = parg[i4];
	}
	
	//
	//! enp
	//
	if((parg[i5] == '0') || (parg[i5] == '1'))
	{
		rpt_content.slot[num].ptp_enp[port] = parg[i5];
	}
	//
	//! step
	//
	if((parg[i6] == '0') || (parg[i6] == '1'))
	{
		rpt_content.slot[num].ptp_step[port] = parg[i6];
	}
	//
	//! domain
	//
	if(parg[i7] != ',')
	{  
	    datalen  = i8 - i7 - 1;
	    if(datalen <= MAX_DOM_LEN)
		{ 
			 memset(rpt_ptp_mtc.ptp_dom[num][port],'\0', 4);
			 memcpy(rpt_ptp_mtc.ptp_dom[num][port], &parg[i7], datalen);
		}
	}
	//
	//!sync fre
	//
	if(parg[i8] != ',')
	{
	   datalen = i9 - i8 - 1;
	   if(datalen == FRAME_FRE_LEN)
	   		memcpy(rpt_content.slot[num].ptp_sync[port], &parg[i8], datalen);
	  
	}
	//
	//!announce。
	//
	if(parg[i9] != ',')
	{
	   datalen = i10 - i9 - 1;
	   if(datalen == FRAME_FRE_LEN)
	   		memcpy(rpt_content.slot[num].ptp_announce[port], &parg[i9], datalen);
	   	 
	}
	//
	//!delaycom
	//
	if(parg[i10] != ',')
	{
	   datalen = i11 - i10 - 1;
	   if(datalen  <= DELAY_COM_LEN)
	   {
	       memset(rpt_content.slot[num].ptp_delaycom[port],'\0', 9);
	       memcpy(rpt_content.slot[num].ptp_delaycom[port], &parg[i10], datalen);
	   }
	}
	//
	//!prio 1
	//
	if(parg[i11] != ',')
	{
	   datalen = i12 - i11 - 1;
	   if(datalen  <= MAX_PRIO_LEN)
	   {
	       memset(rpt_ptp_mtc.ptp_prio1[num][port],'\0', 5);
	       memcpy(rpt_ptp_mtc.ptp_prio1[num][port], &parg[i11], datalen);
	   }
	}
	//
	//! prio 2
	//
	if(parg[i12] != ',')
	{
	   datalen = i13 - i12 - 1;
	   if(datalen  <= MAX_PRIO_LEN)
	   {
	       memset(rpt_ptp_mtc.ptp_prio2[num][port],'\0', 5);
	       memcpy(rpt_ptp_mtc.ptp_prio2[num][port], &parg[i12], datalen);
	   }
	}
	//
	//! ptp versionptp_ver
	//
    if(parg[i13] != '\0')
	{
	   datalen = len - i13;
	   if(datalen  <= 9)
	   {
	       memset(rpt_ptp_mtc.ptp_ver[num][port],'\0', 10);
	       memcpy(rpt_ptp_mtc.ptp_ver[num][port], &parg[i13], datalen);
	   }
	}
#if DEBUG_NET_INFO
    if(print_switch == 0)
    printf("PGE4S MOD %s:%s#\n", NET_SAVE, rpt_ptp_mtc.ptp_ver[num][port]);
#endif
    return 0; 
        

}

//
//!PGE4S单盘输出单播对端100S上报
//
int save_pge4v_mtc(char slot, unsigned char *parg)
{
    int num, port;
    int i, j, i1, i2, i3, i4,len;//, i5, i6, i7, i8, i9, i10, i11, i12, i13
    unsigned char datalen;
	datalen = 0;
    j = i1 = i2 = i3  =i4 = 0;
	
	//
	//num , port , len ;
	//
    num = (int)(slot - 'a');
    port = (int)(parg[0] - 0x31);
    len = strlen(parg);
    //
    //slot range 0 ~ 13。
    //
	if((num < 0) || (num > 13))
		return 1;
    //
    //port range 0 ~ 3。
    //
	if((port < 0) ||(port > 3))
		return 1;
		
    
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
	//
	//error ,return 1。
    if(i4 == 0)
    {
        return (1);
    }
	//
	//! mtc ip。
	//
    if(parg[i1] != ',')
    {
        datalen = i2 - i1 - 1;
		if(datalen <= MAX_IP_LEN)
		{
        	memset(rpt_ptp_mtc.ptp_mtc_ip[num][port], '\0', 9);
        	memcpy(rpt_ptp_mtc.ptp_mtc_ip[num][port], &parg[i1], datalen); 
		}
    }
    //
    //! sfp modul type。
    //
    if(parg[i2] != ',')
    {
        datalen = i3 - i2 - 1; 
		if(datalen == FRAME_FRE_LEN)
		{
		    memset(rpt_ptp_mtc.sfp_type[num][port], '\0', 3);
        	memcpy(rpt_ptp_mtc.sfp_type[num][port], &parg[i2], datalen);
		}
    }
    //
    //! op low. 
    //
    if(parg[i3] != ',')
    {
        datalen = i4 - i3 - 1;
		if(datalen <= MAX_OPTHR_LEN)
		{
		    memset(rpt_ptp_mtc.sfp_oplo[num][port], '\0', 7);
        	memcpy(rpt_ptp_mtc.sfp_oplo[num][port], &parg[i3], datalen);
		}
    }
	//
	//! op hi.
	//
	if(parg[i4] != '\0')
	{
	   datalen = len - i4;
	   if(datalen <= MAX_OPTHR_LEN)
	   {
		    memset(rpt_ptp_mtc.sfp_ophi[num][port], '\0', 7);
        	memcpy(rpt_ptp_mtc.sfp_ophi[num][port], &parg[i4], datalen);
	   }
	}
	
#if DEBUG_NET_INFO
    if(print_switch == 0)
    printf("PGE4S%s:%s#\n", NET_SAVE, rpt_ptp_mtc.ptp_mtc_ip[num][port]);
#endif
    return 0; 
        
}
//
//!PGE4S单盘输出PTP状态字节100S上报
//
int save_pge4v_sta(char slot, unsigned char *parg)
{
    int num, port;
    int i, j, i1, i2, i3, i4,len;//, i5, i6, i7, i8, i9, i10, i11, i12, i13
    unsigned char datalen;
	datalen = 0;
    j = i1 = i2 = i3  =i4 = 0;
	
	//
	//num , port , len ;
	//
    num = (int)(slot - 'a');
    port = (int)(parg[0] - 0x31);
    len = strlen(parg);
    //
    //slot range 0 ~ 13。
    //
	if((num < 0) || (num > 13))
		return 1;
    //
    //port range 0 ~ 3。
    //
	if((port < 0) ||(port > 3))
		return 1;
		
    
    for(i = 0; i < len; i++)
    {
        if(parg[i] == ',')
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
	//
	//error ,return 1。
    if(i4 == 0)
    {
        return (1);
    }
	//
	//! time source。
	//
    if(parg[i1] != ',')
    {
        datalen = i2 - i1 - 1;
		if(datalen == MAX_TIMES_LEN)
		{
        	memset(rpt_ptp_mtc.time_source[num][port], '\0', 5);
        	memcpy(rpt_ptp_mtc.time_source[num][port], &parg[i1], datalen); 
		}
    }
    //
    //! clock class。
    //
    if(parg[i2] != ',')
    {
        datalen = i3 - i2 - 1; 
		if(datalen <= MAX_CLKCLS_LEN)
		{
		    memset(rpt_ptp_mtc.clock_class[num][port], '\0', 4);
        	memcpy(rpt_ptp_mtc.clock_class[num][port], &parg[i2], datalen);
		}
    }
    //
    //! time acc. 
    //
    if(parg[i3] != ',')
    {
        datalen = i4 - i3 - 1;
		if(datalen <= MAX_TIMES_LEN)
		{
		    memset(rpt_ptp_mtc.time_acc[num][port], '\0', 5);
        	memcpy(rpt_ptp_mtc.time_acc[num][port], &parg[i3], datalen);
		}
    }
	//
	//! synce ssm.
	//
	if(parg[i4] != '\0')
	{
	   datalen = len - i4;
	   if(datalen <= MAX_SYNCSSM_LEN)
	   {
		    memset(rpt_ptp_mtc.synce_ssm[num][port], '\0', 14);
        	memcpy(rpt_ptp_mtc.synce_ssm[num][port], &parg[i4], datalen);
	   }
	}
	
#if DEBUG_NET_INFO
    if(print_switch == 0)
    printf("PGE4S%s:%s#\n", NET_SAVE, rpt_ptp_mtc.synce_ssm[num][port]);
#endif
    return 0; 
        
}

//
//!PGE4S单盘输出ALM 100S上报
//
int save_pge4v_alm(char slot, unsigned char *parg)
{
    int num;
    unsigned char  tmp[14];
    memset(tmp, '\0', 14);
    num = (int)(slot - 'a');
    //
    //!屏蔽、告警产生、告警消除
    //
    rpt_alm_to_client(slot, parg);
    sprintf((char *)tmp, "\"%c,j,%s\"", slot, parg);
    
    if((parg[0] >> 4) == 0x05) 
   		memcpy(alm_sta[PGE4S_ALM5_START + num], tmp, strlen(tmp));
    else
        memcpy(alm_sta[num], tmp, strlen(tmp));
	//printf("pge4v2 alarm %s\n",parg);
    return 0;
        
}







int send_init_config(char slot, unsigned char *parg)
{
    int num;

	printf("###send_init_config\n");
    if(slot < 'a' || slot > 'r')
    {
        return -1;
    }
    else
    {
        num = (int)(slot - 'a');
    }
	printf("###parg:%s\n",parg);
    printf("###init_pan:%d\n",num);
	
	init_pan(slot);
    return 0;
}



/**********************/
_REVT_NODE REVT_fun[] =
{
    {"C23", save_gb_rcv_mode},
    {"C25", save_gb_work_mode},
    {"C26", save_gb_mask},
    {"C01", save_rb_msmode},
    {"C02", save_rb_src},

    {"CF1", save_gb_rcv_mode},
    {"CF2", save_gb_mask},
    {"CF3", save_gbtp_sta},
    {"CF4", save_gb_tod},
    {"CF5", save_gbtp_delay},
    {"CF6", save_pps_delay},
    {"C24", save_gb_bdzb},

    {"C05", save_rb_prio},
    {"C06", save_rb_mask},
    {"C07", save_rf_tzo},
    {"C08", save_rb_dey_chg},
    {"C09", save_rb_sta_chg},

    {"C03", save_rb_leap_chg},
    {"C04", save_rb_leap_flg},
    {"C31", save_tod_sta},
    {"C41", save_out_lev},
    {"C42", save_out_typ},

    {"C43", save_out_sta},
    {"C44", save_out_prr},
    {"C51", save_ptp_net},
    {"C52", save_ptp_mod},
    {"C14", save_ptp_mtc16},   //ptp mtc ip
    
    {"C53", save_ptp_mtc},
    {"CC7", save_ptp_sta},     /*20170616 add		  */
    {"CC9", save_Leap_sta},    /*Satellite leap change*/
	{"CC8", save_ptp_dom},     /*20170616 add		 */
    {"C61", save_ref_prio},
    {"C62", save_ref_mod},

    {"C63", save_ref_sa},
    {"C64", save_ref_tl},
    {"C65", save_ref_en},
    {"C67", save_ref_ssm_en},
    {"C71", save_rs_en},
    {"C72", save_rs_tzo},

    {"C81", save_ppx_mod},
    {"CA1", save_tod_en},
    {"CA2", save_tod_pro},
    {"CB1", save_igb_en},
    {"CB2", save_igb_rat},

    {"CB3", save_igb_max},
    {"CC1", save_ntp_pnet},
    {"CC2", save_ntp_en},
    {"CC3", save_ntp_key},
    {"CC4", save_ntp_ms},
    {"CC5", save_ntp_sta},
	{"CC6", save_ntp_psta},
	
	{"CD1", save_pgein_net},     
	{"CD2", save_pgein_mod},     
	{"CD3", save_pgein_clkclass},
	
	{"CE1", save_pge4v_net},     
	{"CE2", save_pge4v_mod},     
	{"CE3", save_pge4v_mtc},
	{"CE4", save_pge4v_sta},     
	
    {"C84", save_2mb_lvlm},   /*new add*/
    {"C10", save_rb_sa},
    {"C12", save_ok},
    {"C13", save_ok}
   
};

int NUM_REVT = ((int)(sizeof(REVT_fun) / sizeof(_REVT_NODE)));

_RPT100S_NODE RPT100S_fun[] =
{
    {"R-GB-STA", 8, "0", save_gb_sta}, 
    {"R-GB-ORI", 8, "C28", save_gb_ori},
    {"R-GB-REV", 8, "C27", save_gb_rev},
    {"R-GB-TOD", 8, "C29", save_gb_tod},
    {"R-GB-ALM", 8, "0", save_gb_alm}, 		//GB ALARM
    
    {"R-GBTP-STA",10,"0",save_gbtp_sta},	//GBTPV2 Status.R-GBTP-STA
    {"R-GBTP-PST",10,"0",save_gbtp_pst},	//GBTPV2 Postion.R-GBTP-PST
    {"R-GBTP-INFO",11,"0", save_gbtp_info},	//GBTPV2 Sat info.R-GBTP-INFO
    {"R-GBTP-ALM",10,"0", save_gbtp_alm},	//GBTPV2 ALM.R-GBTP-ALM
    {"R-GBTP-DLY",10,"0", save_gbtp_delay}, //GBTPIII DLY.R-GBTP-DLY 
    {"R-PPS-DLY",9,"0", save_pps_delay},    //GBTPIII DLY.R-PPS-DLY
    
    {"R-GB-MASK", 9, "0", save_gb_mask}, 
    {"R-SY-VER", 8, "0", save_sy_ver},
    {"R-RX-DPH", 8, "C00", save_rx_dph},
    {"R-RF-SRC", 8, "0", save_rf_src},
    {"R-RB-TOD", 8, "0", save_rb_tod},

    {"R-RF-DEY", 8, "0", save_rf_dey},
    {"R-RF-TZO", 8, "0", save_rf_tzo},
    {"R-RX-STA", 8, "0", save_rx_sta},
    {"R-RX-ALM", 8, "0", save_rx_alm},
    {"R-TOD-STA", 9, "0", save_tod_sta},

    {"R-TP-ALM", 8, "0", save_tp_alm},
    {"R-OUT-LEV", 9, "0", save_out_lev},
    {"R-OUT-TYP", 9, "0", save_out_typ},
    {"R-OUT-ALM", 9, "0", save_out_alm},//OUT32 ALARM
    {"R-OUT-STA", 9, "0", save_out_sta},

    {"R-PTP-NET", 9, "0", save_ptp_net},
    {"R-PTP-MOD", 9, "0", save_ptp_mod},
    {"R-PTP-ALM", 9, "0", save_ptp_alm},//R-PTP-ALM
    {"R-REF-PRIO", 10, "0", save_ref_prio},
    {"R-REF-MOD", 9, "0", save_ref_mod},

    {"R-REF-PH", 8, "C66", save_ref_ph},
    {"R-REF-STA", 9, "0", save_ref_sta},
    {"R-REF-ALM", 9, "0", save_ref_alm},//REF ALM
    {"R-REF-TPY", 9, "0", save_ref_tpy},
    {"R-REF-IEN", 9, "0", save_ref_ssm_en},
    {"R-RS-EN", 7, "0", save_rs_en},

    {"R-RS-TZO", 8, "0", save_rs_tzo},
    {"R-PPX-MOD", 9, "0", save_ppx_mod},
    {"R-TOD-EN", 8, "0", save_tod_en},
    {"R-TOD-PRO", 9, "0", save_tod_pro},
    {"R-IGB-EN", 8, "0", save_igb_en},

    {"R-IGB-RAT", 9, "0", save_igb_rat},
    {"R-IGB-MAX", 9, "0", save_igb_max},
    
    {"R-IGB-ZONE", 10, "0", save_igb_tzone},/*igb zone set******/
    
    {"R-NTP-STA", 9, "0", save_ntp_sta},
    {"R-NTP-NET", 9, "0", save_ntp_net},
    {"R-NTP-PNET", 10, "0", save_ntp_pnet},
    
    {"R-NTP-EN", 8, "0", save_ntp_en},
    {"R-NTP-PSTA",10, "0", save_ntp_psta},
    {"R-NTP-KEY", 9, "0", save_r_ntp_key},
    {"R-NTP-MS", 8, "0", save_ntp_ms},
    {"R-NTP-ALM", 9, "0", save_ntp_alm},//NTP ALM
    {"R-OUT-CFG", 9, "0", save_ext_bid},
    {"R-OUT-MGR", 9, "0", save_ext_mgr_pr},

    {"R-LEAP-ALM", 10, "0", save_leap_alm},
    {"R-STA1-ALM", 10, "0", save_sta1_alm},
    
    {"R-PGEIN-NET", 11, "0", save_pgein_net},
    {"R-PGEIN-MOD", 11, "0", save_pgein_mod},
    {"R-PGEIN-ALM", 11, "0", save_pgein_alm},
    
    {"R-PGE4V-NET", 11, "0", save_pge4v_net},
    {"R-PGE4V-MOD", 11, "0", save_pge4v_mod},
    {"R-PGE4V-MTC", 11, "0", save_pge4v_mtc},
    {"R-PGE4V-STA", 11, "0", save_pge4v_sta},
    {"R-PGE4V-ALM", 11, "0", save_pge4v_alm},
	
    
	{"R-CONFIG", 8, "0", send_init_config}   /*20150909 init pan*/
	

};

int _NUM_RPT100S = ((int)(sizeof(RPT100S_fun) / sizeof(_RPT100S_NODE))); //2014.12.5

