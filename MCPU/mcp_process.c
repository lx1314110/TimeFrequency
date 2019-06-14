#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/msg.h>
//#include <sys/select.h>
//#include <sys/time.h>

#include <signal.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <net/route.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>


#include "ext_cmn.h"
#include "ext_global.h"
#include "ext_crpt.h"
#include "ext_trpt.h"
#include "ext_alloc.h"

#include "mcp_process.h"

#include "mcp_def.h"
#include "mcp_set_struct.h"
#include "mcp_save_struct.h"
//#include "memwatch.h"


#define MGE_RALM_NUM  4

int shake_hand_count = 0;
int reset_network_card = 0;
//int _CMD_NODE_MAX = ((int)(sizeof(g_cmd_fun)/sizeof(_CMD_NODE)));


const char *g_cmd_fun_ret[] = { "ENEQ", "IIAC", "ICNV", "IDNV",
                                "IISP", "IITA", "INUP", "SCSN", "IPMS", "PSWD"
                              };

unsigned char ssm_alm_last = 0;//0表示无告警
unsigned char ssm_alm_curr = 0;//0表示无告警


void strncpynew(unsigned char *str1, unsigned char *str2, int n)
{
    strncpy(str1, str2, n);
    *(str1 + n) = '\0';
}
void strncatnew(unsigned char *str1, unsigned char *str2, int n)
{
    Uint16 strlength;
    strlength = strlen(str1);
    strncat(str1, str2, n);
    *(str1 + strlength + n) = '\0';
}

int strncmp1(unsigned char *p1, unsigned char *p2, unsigned char n)
{
    unsigned int  i = 0;
    signed char  j = 0;
    for (i = 0; i < n; i++)
    {
        if (p1[i] == p2[i])
        {
            ;
        }
        else if(p1[i] > p2[2])
        {
            j = 1;
            break;
        }
        else
        {
            j = -1;
            break;
        }
    }
    return j;
}

void timefunc(int sig)
{
    int count, i;
    int  client_num[MAX_CLIENT];

    count = judge_client_online(client_num);

    for(i = 0; i < count; i++)
    {
        if(timeout_cnt[client_num[i]] == 0)
        {
            FD_CLR(client_fd[client_num[i]], &allset);
			close(client_fd[client_num[i]]);
            client_fd[client_num[i]] = -1;
            printf("<timeout>close client<%d>\n", client_num[i]);
        }

        timeout_cnt[client_num[i]] = 0;
    }

    reset_network_card = 1;
}

int init_timer()
{
    struct itimerval value;
    memset(&value, 0, sizeof(struct itimerval));
    value.it_value.tv_sec = 120;
    value.it_interval.tv_sec = 120;
    signal(SIGALRM, timefunc);             /*捕获定时信号 */
    setitimer(ITIMER_REAL, &value, NULL);  /*定时开始     */
    //printf("250s timer startx\n");
    return 0;
}

int msgQ_create( const char *pathname, char proj_id )
{
    key_t key;
    int msgQ_id;
    /* Create unique key via call to ftok() */
    key = ftok( pathname, proj_id );
    if ( -1 == key )
    {
        return(-1);
    }
    /* Open the queue,create if necessary */
    msgQ_id = msgget( key, IPC_CREAT | 0660 );
    if( -1 == msgQ_id )
    {
        return (-1);
    }
#if DEBUG_NET_INFO
    printf("%s:qid=%d\n", NET_INFO, msgQ_id);
#endif

    return (msgQ_id);
}

int almMsk_id(int index, char type)
{
    if(type != conf_content.alm_msk[index].type)
    {
        conf_content.alm_msk[index].type = type;
        return 0;
    }
    else
    {
        return 1;
    }
}

void init_almMsk(void)
{
    if(almMsk_id(0, 'L') == 0 )//GBTP Y0 Y1, 
    {
        memset(conf_content.alm_msk[0].data, 0x4f, 2);
        memset(conf_content.alm_msk[0].data + 2, 0, 22);
        conf_content.alm_msk[0].data[0] = 0x4b;
    }
    else
    {
        memset(conf_content.alm_msk[0].data + 2, 0, 22);
        conf_content.alm_msk[0].data[0] &= ~(1 << 2);  //alrm mask bit 2 ,1pps+tod alarm.
    }

    if(almMsk_id(1, 'g') == 0 )//GBTP2 
    {
        memset(conf_content.alm_msk[1].data, 0x4f, 2);
        memset(conf_content.alm_msk[1].data + 2, 0, 22);
        conf_content.alm_msk[1].data[0] = 0x4b;
    }
    else
    {
        memset(conf_content.alm_msk[1].data + 2, 0, 22);
        conf_content.alm_msk[1].data[0] &= ~(1 << 2); // alrm mask bit 2 ,1pps+tod alarm.
    }

    if(almMsk_id(2, 'P') == 0 )//GTP
    {
        memset(conf_content.alm_msk[2].data, 0x4f, 2);
        memset(conf_content.alm_msk[2].data + 2, 0, 22);
        conf_content.alm_msk[2].data[0] = 0x49;
    }
    else
    {
        memset(conf_content.alm_msk[2].data + 2, 0, 22);
        conf_content.alm_msk[2].data[0] &= ~(3 << 1);
    }

    if(almMsk_id(3, 'e') == 0 )//GGTP
    {
        memset(conf_content.alm_msk[3].data, 0x4f, 2);
        memset(conf_content.alm_msk[3].data + 2, 0, 22);
        conf_content.alm_msk[3].data[0] = 0x4b;
    }
    else
    {
        memset(conf_content.alm_msk[3].data + 2, 0, 22);

        conf_content.alm_msk[3].data[0] &= ~(1 << 2);
    }

	if(almMsk_id(4, 'l') == 0 )//BTP
    {
        memset(conf_content.alm_msk[4].data, 0x4f, 2);
        memset(conf_content.alm_msk[4].data + 2, 0, 22);
        conf_content.alm_msk[4].data[0] = 0x4a;
    }
    else
    {
        memset(conf_content.alm_msk[4].data + 2, 0, 22);
		conf_content.alm_msk[4].data[0] &= ~(1 << 0);
        conf_content.alm_msk[4].data[0] &= ~(1 << 2);
    }

    if(almMsk_id(5, 'K') == 0 )//RB
    {
        memset(conf_content.alm_msk[5].data, 0x4f, 5);
        memset(conf_content.alm_msk[5].data + 5, 0, 19);
        conf_content.alm_msk[5].data[0] = 0x43;        //   Mask 17# 18# 1pp+tod 
        conf_content.alm_msk[5].data[2] = 0x4b;        //
        conf_content.alm_msk[5].data[4] = 0x4e;      
        conf_content.alm_msk[5].data[1] &= ~(0x3 << 2);// mask LOF、AIS
        conf_content.alm_msk[5].data[2] &= ~(0x7 << 0);// mask BPV、CRC
    }
    else
    {
        memset(conf_content.alm_msk[5].data + 5, 0, 19);
        conf_content.alm_msk[5].data[0] &= ~( (1 << 2) | (1 << 3) );//mask 17# 18# 1pps+tod
        //conf_content.alm_msk[4].data[2] &= ~(1 << 2);//modify:2017-07-19, delete the Null mask.
        conf_content.alm_msk[5].data[4] &= ~1;     
        conf_content.alm_msk[5].data[1] &= ~(0x3 << 2);
        conf_content.alm_msk[5].data[2] &= ~(0x7 << 0);

    }

    if(almMsk_id(6, 'R') == 0 )
    {
        memset(conf_content.alm_msk[6].data, 0x4f, 5);
        memset(conf_content.alm_msk[6].data + 5, 0, 19);
        conf_content.alm_msk[6].data[0] = 0x43;
        conf_content.alm_msk[6].data[2] = 0x4b;
        conf_content.alm_msk[6].data[4] = 0x4e;
    }
    else
    {
        memset(conf_content.alm_msk[6].data + 5, 0, 19);
        conf_content.alm_msk[6].data[0] &= ~( (1 << 2) | (1 << 3) );
        //conf_content.alm_msk[5].data[2] &= ~(1 << 2);//modify:2017-07-19, delete the Null mask.
        conf_content.alm_msk[6].data[4] &= ~1;
    }

    if(almMsk_id(7, 'I') == 0 )//TP16
    {
        memset(conf_content.alm_msk[7].data, 0x4f, 6);
        memset(conf_content.alm_msk[7].data + 6, 0, 18);
        conf_content.alm_msk[7].data[0] = 0x40;
    }
    else
    {
        memset(conf_content.alm_msk[7].data + 6, 0, 18);
        //conf_content.alm_msk[7].data[0] = 0x40;
    }

    if(almMsk_id(8, 'J') == 0 )//OUT16
    {
        memset(conf_content.alm_msk[8].data, 0x4f, 6);
        memset(conf_content.alm_msk[8].data + 6, 0, 18);
    }
    else
    {
        memset(conf_content.alm_msk[8].data + 6, 0, 18);
    }

    if(almMsk_id(9, 'a') == 0 )//OUT32
    {
        memset(conf_content.alm_msk[9].data, 0x4f, 6);
        memset(conf_content.alm_msk[9].data + 6, 0, 18);
    }
    else
    {
        memset(conf_content.alm_msk[9].data + 6, 0, 18);
    }


    if(almMsk_id(10, 'b') == 0 )//OUTH32
    {
        memset(conf_content.alm_msk[10].data, 0x4f, 6);
        memset(conf_content.alm_msk[10].data + 6, 0, 18);
    }
    else
    {
        memset(conf_content.alm_msk[10].data + 6, 0, 18);
    }


    if(almMsk_id(11, 'c') == 0 )//OUTE32
    {
        memset(conf_content.alm_msk[11].data, 0x4f, 6);
        memset(conf_content.alm_msk[11].data + 6, 0, 18);
    }
    else
    {
        memset(conf_content.alm_msk[11].data + 6, 0, 18);
    }


    /*if(almMsk_id(11, 'A') == 0 )//PTP4
    {
        memset(conf_content.alm_msk[11].data, 0x4f, 3);
        memset(conf_content.alm_msk[11].data + 3, 0, 21);
        conf_content.alm_msk[11].data[0] = 0x40;
    }
    else
    {
        memset(conf_content.alm_msk[11].data + 3, 0, 21);
        conf_content.alm_msk[11].data[0] = 0x40;
    }
    if(almMsk_id(12, 'B') == 0 )//PF04
    {
        memset(conf_content.alm_msk[12].data, 0x4f, 3);
        memset(conf_content.alm_msk[12].data + 3, 0, 21);
        conf_content.alm_msk[12].data[0] = 0x40;
    }
    else
    {
        memset(conf_content.alm_msk[12].data + 3, 0, 21);
        conf_content.alm_msk[12].data[0] = 0x40;
    }
    if(almMsk_id(13, 'C') == 0 )//PGE4
    {
        memset(conf_content.alm_msk[13].data, 0x4f, 3);
        memset(conf_content.alm_msk[13].data + 3, 0, 21);
        conf_content.alm_msk[13].data[0] = 0x40;
    }
    else
    {
        memset(conf_content.alm_msk[13].data + 3, 0, 21);
        conf_content.alm_msk[13].data[0] = 0x40;
    }
    if(almMsk_id(14, 'D') == 0 )//PGO4
    {
        memset(conf_content.alm_msk[14].data, 0x4f, 3);
        memset(conf_content.alm_msk[14].data + 3, 0, 21);
        conf_content.alm_msk[14].data[0] = 0x40;
    }
    else
    {
        memset(conf_content.alm_msk[14].data + 3, 0, 21);
        conf_content.alm_msk[14].data[0] = 0x40;
    }*/

    if(almMsk_id(12, 'S') == 0 )//REFTF
    {
        memset(conf_content.alm_msk[12].data, 0x4f, 6);
        memset(conf_content.alm_msk[12].data + 6, 0x5f, 6);
        memset(conf_content.alm_msk[12].data + 12, 0, 12);
    }
    else
    {
        memset(conf_content.alm_msk[12].data + 12, 0, 12);
    }

    if(almMsk_id(13, 'U') == 0 )//PPX
    {
        memset(conf_content.alm_msk[13].data, 0x40, 1);
        memset(conf_content.alm_msk[13].data + 1, 0, 23);
    }
    else
    {
        memset(conf_content.alm_msk[13].data + 1, 0, 23);
        //conf_content.alm_msk[13].data[0] = 0x40;
    }

    if(almMsk_id(14, 'V') == 0 )//TOD16
    {
        memset(conf_content.alm_msk[14].data, 0x40, 1);
        memset(conf_content.alm_msk[14].data + 1, 0, 23);
    }
    else
    {
        memset(conf_content.alm_msk[14].data + 1, 0, 23);
        //conf_content.alm_msk[14].data[0] = 0x40;
    }

    if(almMsk_id(15, 'Z') == 0 )//NTP12
    {
        memset(conf_content.alm_msk[15].data, 0x4f, 4);
        memset(conf_content.alm_msk[15].data + 4, 0, 20);//modify:2017-07-19 ,alarm mask,
    }
    else
    {
        memset(conf_content.alm_msk[15].data + 4, 0, 20);//modify:2017-07-19,alarm mask.
    }

    if(almMsk_id(16, 'N') == 0 )//MCP
    {
        memset(conf_content.alm_msk[16].data, 0x4f, 6);
        //memset(conf_content.alm_msk[16].data + 6, 0x5f, 6);
        memset(conf_content.alm_msk[16].data + 6, 0, 18);

		conf_content.alm_msk[16].data[0] &= ~( (1 << 2) | (1 << 3) );
		conf_content.alm_msk[16].data[1] &= ~( 1 << 3) ;
        //conf_content.alm_msk[19].data[2] = 0x48;
		//memset(conf_content.alm_msk[19].data + 3, 0x40, 3);
    }
    else
    {
        memset(conf_content.alm_msk[16].data + 6, 0, 18);
        conf_content.alm_msk[16].data[0] &= ~( (1 << 2) | (1 << 3) );
        conf_content.alm_msk[16].data[1] &= ~(1 << 3);
        //memset(conf_content.alm_msk[19].data + 2, 0x40, 4);
    }

    if(almMsk_id(17, 'Q') == 0 )//FAN
    {
        memset(conf_content.alm_msk[17].data, 0x4f, 1);
        memset(conf_content.alm_msk[17].data + 1, 0, 23);
    }
    else
    {
        memset(conf_content.alm_msk[17].data + 1, 0, 23);
    }
    if(almMsk_id(18, 'h') == 0 )//out32s
    {
        memset(conf_content.alm_msk[18].data, 0x4f, 6);
        memset(conf_content.alm_msk[18].data + 6, 0, 18);
    }
    else
    {
        memset(conf_content.alm_msk[18].data + 6, 0, 18);
    }
    if(almMsk_id(19, 'i') == 0 )//OUT16S
    {
        memset(conf_content.alm_msk[19].data, 0x4f, 6);
        memset(conf_content.alm_msk[19].data + 6, 0, 18);
    }
    else
    {
        memset(conf_content.alm_msk[19].data + 6, 0, 18);
    }

    if(almMsk_id(20, 's') == 0 )//BPGEIN
    {
        memset(conf_content.alm_msk[20].data, 0x4f, 6);
        memset(conf_content.alm_msk[20].data + 6, 0, 18);
        
    }
    else
    {
        memset(conf_content.alm_msk[20].data + 6, 0, 18);
    }

	if(almMsk_id(21, 'j') == 0 )//pge4v2
    {
        memset(conf_content.alm_msk[21].data, 0x4f, 6);
        memset(conf_content.alm_msk[21].data + 6, 0x5f, 6);
        memset(conf_content.alm_msk[21].data + 12, 0, 12);
    }
    else
    {
        memset(conf_content.alm_msk[21].data + 12, 0, 12);
    }

	if(almMsk_id(22, 'v') == 0 )//GBTPIIV2
    {
        memset(conf_content.alm_msk[22].data, 0x4f, 2);
        memset(conf_content.alm_msk[22].data + 2, 0, 22);
    }
    else
    {
        memset(conf_content.alm_msk[22].data + 2, 0, 22);
    }
	if(almMsk_id(23, 'f') == 0 )//MGE
    {
        memset(conf_content.alm_msk[23].data, 0x4f, 4);
        memset(conf_content.alm_msk[23].data + 4, 0, 20);
    }
    else
    {
        memset(conf_content.alm_msk[23].data + 4, 0, 20);
    }

	if(almMsk_id(24, 'd') == 0 )//DRV
    {
        memset(conf_content.alm_msk[24].data, 0x4f, 1);
        memset(conf_content.alm_msk[24].data + 1, 0, 23);
    }
    else
    {
        memset(conf_content.alm_msk[24].data + 1, 0, 23);
    }
	
    memset(&(conf_content.alm_msk[25]), 0, (alm_msk_total - 26)*sizeof(struct ALM_MSK));

}



void init_var(void)
{
    int i;

    memset(slot_type, 'O', sizeof(unsigned char) * 24);
    slot_type[20] = 'N';
	//
	//!clear temp alarm cache.
	//
    memset(&flag_alm, '\0', sizeof(int) * 36 * 14);
    memset(online_sta, 0, sizeof(int) * 24);
	//
	//!clear alarm array.
	//
    memset(&alm_sta, '\0', sizeof(unsigned char) * 37 * 14);
    memset(fpga_ver, '\0', 7);
    memset(timeout_cnt, 0, sizeof(int) * 4);
    //printf("%s\n",slot_type);
    memset(&saved_alms, 0, SAVE_ALM_NUM * 50);
    saved_alms_cnt = 0;

    memcpy(mcp_date, "2011-11-11", 10);
    memcpy(mcp_time, "11:11:11", 8);
    memcpy(password, "password", 8);


    memcpy(alm_sta[20], "\"u,N,@@@\"", 9);

    g_DrvLost = 200;

    i = 0;
    i = atoi(conf_content.slot_u.leap_num);
    if(strncmp1(conf_content.slot_u.leap_num, "00", 2) == 0)
    {}
    else if((i < 1) || (i > 99))
    {
        memcpy(conf_content.slot_u.leap_num, "18", 2);
    }


    /****test**/
    for(i = 0; i < 14; i++)
    {
        rpt_content.slot[i].out_mode = '0';
        rpt_content.slot[i].out_sta = '0';
    }
    /*
    	printf("slot_type=<%s>\n",slot_type);
    	printf("online=<");
    	for(i=0;i<24;i++)
    		printf("%d",online_sta[i]);
    	printf(">;\n");
    */
    atag = 0;
    flag_i_o = 0;
    flag_i_p = 0;
    time_temp = 0;
    current_client = -1;
    g_lev = -1;
    qid = -1;


    rpt_content.slot_o.rb_sa = '@';
    rpt_content.slot_p.rb_sa = '@';
    rpt_content.slot_o.rb_tl = 'g';
    rpt_content.slot_p.rb_tl = 'g';
    rpt_content.slot_o.rb_tl_2mhz = 'g';
    rpt_content.slot_p.rb_tl_2mhz = 'g';
	rpt_content.slot[0].ref_2mb_lvlm = '0';
	rpt_content.slot[1].ref_2mb_lvlm = '0';
	
	//GBTPIIV2 VAR INIT
	rpt_content.slot_q.gb_sta = '0';
    rpt_content.slot_q.staen = '1';
	
    rpt_content.slot_q.gb_tod[0] = '3';
    rpt_content.slot_q.leapalm[0] = '0';
	rpt_content.slot_q.leapalm[1] = '0';
    rpt_content.slot_q.leapnum[0] = '0';
	rpt_content.slot_q.leapnum[1] = '0';

	rpt_content.slot_r.gb_sta = '0';
    rpt_content.slot_r.staen = '1';
	
    rpt_content.slot_r.gb_tod[0] = '3';
    rpt_content.slot_r.leapalm[0] = '0';
	rpt_content.slot_r.leapalm[1] = '0';
    rpt_content.slot_r.leapnum[0] = '0';
	rpt_content.slot_r.leapnum[1] = '0';

    //告警屏蔽初始化；
    init_almMsk();
    memset(event_old, 0, 17 * 128);

    g_LeapFlag = 0;
}

int Create_Thread(bool_t * loop)
{
    pthread_t thread1;
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(pthread_create(&thread1,&attr,(void *)timer_mcp,(void *) loop) != 0 ) { 
		printf("pthread_create1 mcp gf_Usual error.\n");
		return(false);
	}
	pthread_attr_destroy(&attr);
	return (true);
 
}


void init_sta1_ip(unsigned char slot)
{
    unsigned char ip_temp[9];
    unsigned char gate_temp[9];
    unsigned char mask_temp[9];
    if( (( (conf_content3.sta1.ip[0] ) & 0xf0 ) != 0x40) ||
            (((conf_content3.sta1.ip[2] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.ip[4] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.ip[6] ) & 0xf0) != 0x40) )
    {

        memset(conf_content3.sta1.ip, 0, 9);
        memcpy(conf_content3.sta1.ip, "L@JH@AFC", 8);
    }


    if( (((conf_content3.sta1.gate[0] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.gate[2] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.gate[4] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.gate[6] ) & 0xf0) != 0x40) )
    {

        memset(conf_content3.sta1.gate, 0, 9);
        memcpy(conf_content3.sta1.gate, "L@JH@A@A", 8);
    }

    if( (((conf_content3.sta1.mask[0] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.mask[2] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.mask[4] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.mask[6] ) & 0xf0) != 0x40) )
    {


        memset(conf_content3.sta1.mask, 0, 9);
        memcpy(conf_content3.sta1.mask, "OOOOOO@@", 8);
    }

    memset(ip_temp, 0, 9);
    memcpy(ip_temp, conf_content3.sta1.ip, 8);
    set_sta_ip(ip_temp, slot);
    sleep(1);
    memset(gate_temp, 0, 9);
    memcpy(gate_temp, conf_content3.sta1.gate, 8);
    set_sta_gateway(gate_temp, slot);
    sleep(1);
    memset(mask_temp, 0, 9);
    memcpy(mask_temp, conf_content3.sta1.mask, 8);
    set_sta_netmask(mask_temp, slot);
}

void init_sta1(int num, unsigned char *sendbuf)
{

    unsigned char slot;
    //DevType
    slot = 'a' + num;

    init_sta1_ip(slot);

    memset(sendbuf, '\0', SENDBUFSIZE);
    sprintf((char *)sendbuf, "SET-STA1:000000::MONT/LLN0/DevType=GNSS-97-P300;");
    sendtodown(sendbuf, (char)('a' + num));
    sendtodown(sendbuf, (char)('a' + num));
    usleep(500);
    //DevDescr
    memset(sendbuf, '\0', SENDBUFSIZE);
    sprintf((char *)sendbuf, "SET-STA1:000000::MONT/LLN0/DevDescr=P300;");
    sendtodown(sendbuf, (char)('a' + num));
    sendtodown(sendbuf, (char)('a' + num));

#if 1

    //Company
    memset(sendbuf, '\0', SENDBUFSIZE);
    sprintf((char *)sendbuf, "SET-STA1:000000::MONT/LLN0/Company=DTT;");
    sendtodown(sendbuf, (char)('a' + num));
    sendtodown(sendbuf, (char)('a' + num));
    //PortNum 实际为板卡数量
    sta1_portnum(sendbuf, slot);
    //HWVersion
    memset(sendbuf, '\0', SENDBUFSIZE);
    sprintf((char *)sendbuf, "SET-STA1:000000::MONT/LLN0/HWVersion=v01.00;");
    sendtodown(sendbuf, (char)('a' + num));
    sendtodown(sendbuf, (char)('a' + num));
    //FWVersion
    memset(sendbuf, '\0', SENDBUFSIZE);
    sprintf((char *)sendbuf, "SET-STA1:000000::MONT/LLN0/FWVersion=v01.00;");
    sendtodown(sendbuf, (char)('a' + num));
    sendtodown(sendbuf, (char)('a' + num));
    //SWVersion
    memset(sendbuf, '\0', SENDBUFSIZE);
    sprintf((char *)sendbuf, "SET-STA1:000000::MONT/LLN0/SWVersion=v01.00;");
    sendtodown(sendbuf, (char)('a' + num));
    sendtodown(sendbuf, (char)('a' + num));
    //ip mac
    sta1_net(sendbuf, slot);
    //CpuUsage
    sta1_cpu(sendbuf, slot);
    //InType OutType
    sta1_in_out(sendbuf, slot);
    //Alm
    sta1_alm(sendbuf, slot);
#endif
}

void delay_s(int iSec,int iUsec)
{ 
  struct timeval tv;
  tv.tv_sec=iSec;
  tv.tv_usec=iUsec;
  select(0,NULL,NULL,NULL,&tv);
}



void init_pan(char slot)
{
    /* RB1*/
    unsigned char out_ssm_threshold = 0, out_port = 4;
    int j, len, num, flag, len1, len2;
    unsigned char sendbuf[SENDBUFSIZE];
    unsigned char *ctag = "000000";
    unsigned char tmp1[3], tmp2[3], tmp3[3], tmp4[3], tmp5[3], tmp6[3], tmp7[3];
    //int flag_lev=0;
    num = (int)(slot - 'a');
	//printf("###init_pan:%d\n",num);
    if(slot == 20) /*mcp*/
    {
        memset(sendbuf, '\0', SENDBUFSIZE);
        num = atoi(conf_content.slot_u.leap_num);
        if(strncmp1(conf_content.slot_u.leap_num, "00", 2) == 0)
        {
		}
        else if((num < 1) || (num > 99))
        {
            return ;
        }
        sprintf((char *)sendbuf, "SET-LEAP-NUM:%s::%s;", ctag, conf_content.slot_u.leap_num);
        sendtodown(sendbuf, 'u');
    }
    else if(slot == RB1_SLOT)
    {
        if(slot_type[num] == 'Y')
        {
            return;
        }
        if(conf_content.slot_o.msmode[0] != 0xff)
        {
            if(strlen(conf_content.slot_o.msmode) == 4)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-MAIN:%s::%s;", ctag, conf_content.slot_o.msmode);
                sendtodown(sendbuf, 'o');
            }
        }
        if(conf_content.slot_o.sys_ref != 0xff)
        {
            if(((conf_content.slot_o.sys_ref > 0x29) && (conf_content.slot_o.sys_ref < 0x39))||conf_content.slot_o.sys_ref == 0x46)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-SYS-REF:%s::INP-%c;", ctag, conf_content.slot_o.sys_ref);
                sendtodown(sendbuf, 'o');
            }
        }
        if(conf_content.slot_o.priority[0] != 0xff)
        {
            if(strlen(conf_content.slot_o.priority) == 8)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-PRIO-INP:%s::%s;", ctag, conf_content.slot_o.priority);
                sendtodown(sendbuf, 'o');
            }

        }
        if(conf_content.slot_o.mask[0] != 0xff)
        {
            if(strlen(conf_content.slot_o.mask) == 8)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-MASK-INP:%s::%s;", ctag, conf_content.slot_o.mask);
                sendtodown(sendbuf, 'o');
            }
        }
        if(conf_content.slot_o.dely[0][0] != 0xff)
        {
            if(strlen(conf_content.slot_o.dely[0]) < 9)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-DELY:%s::1,%s;", ctag, conf_content.slot_o.dely[0]);
                sendtodown(sendbuf, 'o');
            }
        }
        if(conf_content.slot_o.dely[1][0] != 0xff)
        {
            if(strlen(conf_content.slot_o.dely[1]) < 9)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-DELY:%s::2,%s;", ctag, conf_content.slot_o.dely[1]);
                sendtodown(sendbuf, 'o');
            }
        }
        if(conf_content.slot_o.dely[2][0] != 0xff)
        {
            if(strlen(conf_content.slot_o.dely[2]) < 9)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-DELY:%s::3,%s;", ctag, conf_content.slot_o.dely[2]);
                sendtodown(sendbuf, 'o');
            }
        }
        if(conf_content.slot_o.dely[3][0] != 0xff)
        {
            if(strlen(conf_content.slot_o.dely[3]) < 9)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-DELY:%s::4,%s;", ctag, conf_content.slot_o.dely[3]);
                sendtodown(sendbuf, 'o');
            }
        }
        if(conf_content.slot_o.tzo[0] != 0xff)
        {
            if(strlen(conf_content.slot_o.tzo) == 3)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-TZO:%s::%s;", ctag, conf_content.slot_o.tzo);
                sendtodown(sendbuf, 'o');
				printf("15 set tzo %s\n",conf_content.slot_o.tzo);
            }
        }
        if(conf_content.slot_o.leaptag != 0xff)
        {
            if((conf_content.slot_o.leaptag > 0x29) && (conf_content.slot_o.leaptag < 0x34))
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-LEAP:%s::%c;", ctag, conf_content.slot_o.leaptag);
                sendtodown(sendbuf, 'o');
            }
        }
        if(conf_content.slot_o.leapmask[0] != 0xff)
        {
            if(strlen(conf_content.slot_o.leapmask) == 5)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-LMAK-INP:%s::%s;", ctag, conf_content.slot_o.leapmask);
                sendtodown(sendbuf, 'o');
            }
        }
    }

    /*RB2*/
    else if(slot == RB2_SLOT)
    {
        if(slot_type[num] == 'Y')
        {
            return;
        }
        if(conf_content.slot_p.msmode[0] != 0xff)
        {
            if(strlen(conf_content.slot_p.msmode) == 4)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-MAIN:%s::%s;", ctag, conf_content.slot_p.msmode);
                sendtodown(sendbuf, 'p');
            }
        }
        if(conf_content.slot_p.sys_ref != 0xff)
        {
            if(((conf_content.slot_p.sys_ref > 0x29) && (conf_content.slot_p.sys_ref < 0x39))||conf_content.slot_p.sys_ref == 0x46)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-SYS-REF:%s::INP-%c;", ctag, conf_content.slot_p.sys_ref);
                sendtodown(sendbuf, 'p');
            }
        }
        if(conf_content.slot_p.priority[0] != 0xff)
        {
            if(strlen(conf_content.slot_p.priority) == 8)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-PRIO-INP:%s::%s;", ctag, conf_content.slot_p.priority);
                sendtodown(sendbuf, 'p');
            }

        }
        if(conf_content.slot_p.mask[0] != 0xff)
        {
            if(strlen(conf_content.slot_p.mask) == 8)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-MASK-INP:%s::%s;", ctag, conf_content.slot_p.mask);
                sendtodown(sendbuf, 'p');
            }
        }
        if(conf_content.slot_p.dely[0][0] != 0xff)
        {
            if(strlen(conf_content.slot_p.dely[0]) < 9)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-DELY:%s::1,%s;", ctag, conf_content.slot_p.dely[0]);
                sendtodown(sendbuf, 'p');
            }
        }
        if(conf_content.slot_p.dely[1][0] != 0xff)
        {
            if(strlen(conf_content.slot_p.dely[1]) < 9)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-DELY:%s::2,%s;", ctag, conf_content.slot_p.dely[1]);
                sendtodown(sendbuf, 'p');
            }
        }
        if(conf_content.slot_p.dely[2][0] != 0xff)
        {
            if(strlen(conf_content.slot_p.dely[2]) < 9)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-DELY:%s::3,%s;", ctag, conf_content.slot_p.dely[2]);
                sendtodown(sendbuf, 'p');
            }
        }
        if(conf_content.slot_p.dely[3][0] != 0xff)
        {
            if(strlen(conf_content.slot_p.dely[3]) < 9)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-DELY:%s::4,%s;", ctag, conf_content.slot_p.dely[3]);
                sendtodown(sendbuf, 'p');
            }
        }
        if(conf_content.slot_p.tzo[0] != 0xff)
        {
            if(strlen(conf_content.slot_p.tzo) == 3)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-TZO:%s::%s;", ctag, conf_content.slot_p.tzo);
                sendtodown(sendbuf, 'p');
				printf("16 set tzo %s\n",conf_content.slot_p.tzo);
            }
        }
        if(conf_content.slot_p.leaptag != 0xff)
        {
            if((conf_content.slot_p.leaptag > 0x29) && (conf_content.slot_p.leaptag < 0x34))
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-LEAP:%s::%c;", ctag, conf_content.slot_p.leaptag);
                sendtodown(sendbuf, 'p');
            }
        }
        if(conf_content.slot_p.leapmask[0] != 0xff)
        {
            if(strlen(conf_content.slot_p.leapmask) == 5)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-LMAK-INP:%s::%s;", ctag, conf_content.slot_p.leapmask);
                sendtodown(sendbuf, 'p');
            }
        }
    }
    /*gbtp1*/
    else if(slot == GBTP1_SLOT)
    {
        if(slot_type[num] == 'Y')
        {
            return;
        }
        if(conf_content.slot_q.mode[0] != 0xff)
        {
            if( ('L' == slot_type[16]) ||
                    ('e' == slot_type[16]) ||
                    'l' == slot_type[16] ||
                    'g' == slot_type[16]
              )
            {
                len = strlen(conf_content.slot_q.mode);
                if( (2 == len) || (3 == len) )
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-MODE:%s::%s;", ctag, conf_content.slot_q.mode);
                    sendtodown(sendbuf, 'q');
                }
            }
            else if( 'P' == slot_type[16] )
            {
                if( 0 == strncmp1(conf_content.slot_q.mode, "GPS", 3) )
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-MODE:%s::%s;", ctag, conf_content.slot_q.mode);
                    sendtodown(sendbuf, 'q');
                }
            }
			else if('v' == slot_type[16] || 'x' == slot_type[16] )
			{
			    len = strlen(conf_content.slot_q.mode);
				if((2 == len)||(3 == len))
				{
				  memset(sendbuf, '\0', SENDBUFSIZE);
                  sprintf((char *)sendbuf, "SET-GBTP-MODE:%s::%s;", ctag, conf_content.slot_q.mode);
                  sendtodown(sendbuf, 'q');
				}
			}
        }/*
			if(conf_content.slot_q.pos[0]!=0xff)
				{
					if(strlen(conf_content.slot_q.pos)>35)
						{
							memset(sendbuf,'\0',SENDBUFSIZE);
							sprintf((char *)sendbuf,"SET-POS:%s::%s;",ctag,conf_content.slot_q.pos);
							sendtodown(sendbuf,'q');
						}
				}*/
        if('v' == slot_type[16])//SET-GBTP-SATEN
		{
            if((conf_content.slot_q.mask[0] == '0') ||(conf_content.slot_q.mask[0] == '1')) 
            {
                    memset(sendbuf, '\0', SENDBUFSIZE);
	                sprintf((char *)sendbuf, "SET-GBTP-SATEN:%s::%s;", ctag, conf_content.slot_q.mask);
	                sendtodown(sendbuf, 'q');
               
            }
			
		}
		else if('x' == slot_type[16])
		{
		    if((conf_content.slot_q.mask[0] == '0') ||(conf_content.slot_q.mask[0] == '1')) 
            {
                    memset(sendbuf, '\0', SENDBUFSIZE);
	                sprintf((char *)sendbuf, "SET-GBTP-SATEN:%s::%s;", ctag, conf_content.slot_q.mask);
	                sendtodown(sendbuf, 'q');
               
            }

			if((strlen(conf_content3.gbtp_delay[0]) <=6) && 
				(atoi(conf_content3.gbtp_delay[0]) <= 32767) &&
				(atoi(conf_content3.gbtp_delay[0]) >= -32768))
			{
					memset(sendbuf, '\0', SENDBUFSIZE);
	                sprintf((char *)sendbuf, "SET-GBTP-DELAY:%s::%s;", ctag, conf_content3.gbtp_delay[0]);
	                sendtodown(sendbuf, 'q');
			}

			if((strlen(conf_content3.gbtp_pps_delay[0]) <=10) && 
				(atoi(conf_content3.gbtp_pps_delay[0]) <= 200000000) &&
				(atoi(conf_content3.gbtp_pps_delay[0]) >= -200000000))
			{
					memset(sendbuf, '\0', SENDBUFSIZE);
	                sprintf((char *)sendbuf, "SET-PPS-DELAY:%s::%s;", ctag, conf_content3.gbtp_pps_delay[0]);
	                sendtodown(sendbuf, 'q');
			}
		}
		else
		{
	        if(conf_content.slot_q.mask[0] != 0xff)
	        {
	            if(strlen(conf_content.slot_q.mask) == 2)
	            {
	                memset(sendbuf, '\0', SENDBUFSIZE);
	                sprintf((char *)sendbuf, "SET-MASK:%s::%s;", ctag, conf_content.slot_q.mask);
	                sendtodown(sendbuf, 'q');
	            }
	        }
		}
    }
    /*gbtp2*/
    else if(slot == GBTP2_SLOT)
    {
        if(slot_type[num] == 'Y')
        {
            return;
        }
        if(conf_content.slot_r.mode[0] != 0xff)
        {
            if( ('L' == slot_type[17]) ||
                    ('e' == slot_type[17]) ||
                    'l' == slot_type[17] ||
                    'g' == slot_type[17]
              )
            {
                len = strlen(conf_content.slot_r.mode);
                if( (2 == len) || (3 == len) )
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-MODE:%s::%s;", ctag, conf_content.slot_r.mode);
                    sendtodown(sendbuf, 'r');
                }
            }
            else if( 'P' == slot_type[17])
            {
                if( 0 == strncmp1(conf_content.slot_r.mode, "GPS", 3) )
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-MODE:%s::%s;", ctag, conf_content.slot_r.mode);
                    sendtodown(sendbuf, 'r');
                }
            }
			else if('v' == slot_type[17] || 'x' == slot_type[17])
			{
			    len = strlen(conf_content.slot_r.mode);
				printf("enter init pan 18\r\n");
				if((2 == len)||(3 == len))
				{
				  memset(sendbuf, '\0', SENDBUFSIZE);
                  sprintf((char *)sendbuf, "SET-GBTP-MODE:%s::%s;", ctag, conf_content.slot_r.mode);
                  sendtodown(sendbuf, 'r');
				  printf(sendbuf);
				  
				}
			}
        }/*
			if(conf_content.slot_r.pos[0]!=0xff)
				{
					len=strlen(conf_content.slot_r.pos);
					if((len<35)&&(len>25))
						{
							memset(sendbuf,'\0',SENDBUFSIZE);
							sprintf((char *)sendbuf,"SET-POS:%s::%s;",ctag,conf_content.slot_r.pos);
							sendtodown(sendbuf,'r');
						}
				}*/
		if('v' == slot_type[17] )//SET-GBTP-SATEN
		{
            if((conf_content.slot_r.mask[0] == '0') ||(conf_content.slot_r.mask[0] == '1')) 
            {
                    memset(sendbuf, '\0', SENDBUFSIZE);
	                sprintf((char *)sendbuf, "SET-GBTP-SATEN:%s::%s;", ctag, conf_content.slot_r.mask);
	                sendtodown(sendbuf, 'r');
               
            }
		}
		else if('x' == slot_type[17])
		{
		    if((conf_content.slot_r.mask[0] == '0') ||(conf_content.slot_r.mask[0] == '1')) 
            {
                    memset(sendbuf, '\0', SENDBUFSIZE);
	                sprintf((char *)sendbuf, "SET-GBTP-SATEN:%s::%s;", ctag, conf_content.slot_r.mask);
	                sendtodown(sendbuf, 'r');
               
            }

			if((strlen(conf_content3.gbtp_delay[1]) <=6) && 
				(atoi(conf_content3.gbtp_delay[1]) <= 32767) &&
				(atoi(conf_content3.gbtp_delay[1]) >= -32768))
			{
					memset(sendbuf, '\0', SENDBUFSIZE);
	                sprintf((char *)sendbuf, "SET-GBTP-DELAY:%s::%s;", ctag, conf_content3.gbtp_delay[1]);
	                sendtodown(sendbuf, 'r');
			}

			if((strlen(conf_content3.gbtp_pps_delay[1]) <=10) && 
				(atoi(conf_content3.gbtp_pps_delay[1]) <= 200000000) &&
				(atoi(conf_content3.gbtp_pps_delay[1]) >= -200000000))
			{
					memset(sendbuf, '\0', SENDBUFSIZE);
	                sprintf((char *)sendbuf, "SET-PPS-DELAY:%s::%s;", ctag, conf_content3.gbtp_pps_delay[1]);
	                sendtodown(sendbuf, 'r');
			}
		}
		else
		{
	        if(conf_content.slot_r.mask[0] != 0xff)
	        {
	            if(strlen(conf_content.slot_r.mask) == 2)
	            {
	                memset(sendbuf, '\0', SENDBUFSIZE);
	                sprintf((char *)sendbuf, "SET-MASK:%s::%s;", ctag, conf_content.slot_r.mask);
	                sendtodown(sendbuf, 'r');
	            }
	        }
		}
    }
    /*out*/
    else if((num >= 0) && (num < 14))
    {
        if( (slot_type[num] == 'J') ||
                (slot_type[num] == 'a') ||
                (slot_type[num] == 'b') ||
                (slot_type[num] == 'c') ||
                (slot_type[num] == 'h') ||
                (slot_type[num] == 'i'))
        {
        
            /*if(conf_content.slot[num].out_lev[0]!=0xff)
            	{
            		if(strlen(conf_content.slot[num].out_lev)==2)
            			{
            				memset(sendbuf,'\0',SENDBUFSIZE);
            				sprintf((char *)sendbuf,"SET-OUT-LEV:%s::%s;",ctag,conf_content.slot[num].out_lev);
            				sendtodown(sendbuf,(char)('a'+num));
            			}
            	}*/
            //
            //OUT32S ADD
            //
            //printf("###init_pan:%d\n",num);
            
            get_out_lev(num, 1);
            //printf("test\n");
             
            if(conf_content.slot[num].out_typ[0] != 0xff)
            {
                if(strlen(conf_content.slot[num].out_typ) == 16)
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-OUT-TYP:%s::%s;", ctag, conf_content.slot[num].out_typ);
                    sendtodown(sendbuf, (char)('a' + num));
					//
					//OUT32S ADD.
					//
					//printf("out32s:%s\n",sendbuf);
                }
            }

            if(conf_content.slot[num].out_mod != 0xff)
            {
				if( (conf_content.slot[num].out_mod != '0') && (conf_content.slot[num].out_mod != '1'))
					conf_content.slot[num].out_mod = '0';
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-OUT-MOD:%s::%c;", ctag, conf_content.slot[num].out_mod);
                sendtodown(sendbuf, (char)('a' + num));
				//
				//OUT32S ADD
				//
				//printf("out32s:%s\n",sendbuf);
            }
        }
		//
		//! tp16 initialtion.
		//
        else if(slot_type[num] == 'I')
        {
            
            if(conf_content.slot[num].tp16_en[0] != 0xff)
            {

                if(strlen(conf_content.slot[num].tp16_en) == 16)
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-TP16-EN:%s::%s;", ctag, conf_content.slot[num].tp16_en);
                    sendtodown(sendbuf, (char)('a' + num));
                }
            }


            j = atoi(conf_content.slot_u.leap_num);
            if(strncmp1(conf_content.slot_u.leap_num, "00", 2) == 0)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-TP16-LEAP:%s::%s;", ctag, conf_content.slot_u.leap_num);
                sendtodown(sendbuf, (char)('a' + num));
            }
            else if((j < 1) || (j > 99))
            {}
            else
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-TP16-LEAP:%s::%s;", ctag, conf_content.slot_u.leap_num);
                sendtodown(sendbuf, (char)('a' + num));
				printf("set tp16 leap %d\n",j);
            }

			//2015.9.9 初始化TP16协议
			if( '0' == conf_content3.mcp_protocol ||
				'1' == conf_content3.mcp_protocol ||
				'2' == conf_content3.mcp_protocol   )
			{
				printf("###init protocol ptp --%d\n",num);
				memset(sendbuf, '\0', SENDBUFSIZE);
				sprintf((char *)sendbuf, "SET-PROTOCOL:%s::%c;", ctag, conf_content3.mcp_protocol);
                sendtodown(sendbuf, (char)('a' + num));
			}
			else
			{
				;
			}
			
        }
		//
		//! ppx
		//
        else if(slot_type[num] == 'U')
        {
            if(conf_content.slot[num].ppx_mod[0] != 0xff)
            {

                if(strlen(conf_content.slot[num].ppx_mod) == 16)
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-PPX-MOD:%s::%s;", ctag, conf_content.slot[num].ppx_mod);
                    sendtodown(sendbuf, (char)('a' + num));
                }
            }
        }
		//
		//!TOD16 
		//
        else if(slot_type[num] == 'V')
        {
            if(conf_content.slot[num].tod_en[0] != 0xff)
            {

                if(strlen(conf_content.slot[num].tod_en) == 16)
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-TOD-EN:%s::%s;", ctag, conf_content.slot[num].tod_en);
                    sendtodown(sendbuf, (char)('a' + num));
                }
            }

            flag = 0;
            memset(sendbuf, '\0', SENDBUFSIZE);
            if(conf_content.slot[num].tod_br >= '0' && conf_content.slot[num].tod_br <= '7')
            {
                sprintf((char *)sendbuf, "SET-TOD-PRO:%s::%c", ctag, conf_content.slot[num].tod_br);
            }
            else
            {
                sprintf((char *)sendbuf, "SET-TOD-PRO:%s::", ctag);
            }
            if(conf_content.slot[num].tod_tzo >= 0x40 && conf_content.slot[num].tod_tzo <= 0x57)
            {
                sprintf((char *)sendbuf, "%s,%c", sendbuf, conf_content.slot[num].tod_tzo);
            }
            else
            {
                sprintf((char *)sendbuf, "%s,", sendbuf);
            }
            j = atoi(conf_content.slot_u.leap_num);
            if((j < 1) || (j > 99))
            {
                if(flag == 0)
                {
                    sprintf((char *)sendbuf, "%s,;", sendbuf);
                    sendtodown(sendbuf, (char)('a' + num));
                }
            }
            else
            {
                sprintf((char *)sendbuf, "%s,%s;", sendbuf, conf_content.slot_u.leap_num);
                sendtodown(sendbuf, (char)('a' + num));
            }

        }
		//
		//!IRIGB 
		//
        else if(slot_type[num] == 'W')
        {
            if(conf_content.slot[num].igb_en[0] != 0xff)
            {

                if(strlen(conf_content.slot[num].igb_en) == 4)
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-IGB-EN:%s::%s;", ctag, conf_content.slot[num].igb_en);
                    sendtodown(sendbuf, (char)('a' + num));
                }
            }
            if(conf_content.slot[num].igb_rat[0] != 0xff)
            {

                if(strlen(conf_content.slot[num].igb_rat) == 4)
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-IGB-RAT:%s::%s;", ctag, conf_content.slot[num].igb_rat);
                    sendtodown(sendbuf, (char)('a' + num));
                }
            }
            if(conf_content.slot[num].igb_max[0] != 0xff)
            {

                if(strlen(conf_content.slot[num].igb_max) == 4)
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-IGB-MAX:%s::%s;", ctag, conf_content.slot[num].igb_max);
                    sendtodown(sendbuf, (char)('a' + num));
                }
            }

            j = atoi(conf_content.slot_u.leap_num);
            if((j < 1) || (j > 99))
            {
                ;
            }
            else
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-IGB-LEAP:%s::%s;", ctag, conf_content.slot_u.leap_num);
                sendtodown(sendbuf, (char)('a' + num));
            }

            if(conf_content3.irigb_tzone[num] != 0)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-IGB-TZONE:%s::%c;", ctag, conf_content3.irigb_tzone[num]);
                sendtodown(sendbuf, (char)('a' + num));
            }

        }
		//
		//REFTF 
		//
        else if(slot_type[num] == 'S')
        {
            switch(conf_content.slot_u.out_ssm_oth)
            {
            case 0x02:
                out_ssm_threshold = 'a';
                break;
            case 0x04:
                out_ssm_threshold = 'b';
                break;
            case 0x08:
                out_ssm_threshold = 'c';
                break;
            case 0x0B:
                out_ssm_threshold = 'd';
                break;
            case 0x0F:
                out_ssm_threshold = 'e';
                break;
            case 0x00:
                out_ssm_threshold = 'f';
                break;
            }

            memset(sendbuf, '\0', SENDBUFSIZE);
            sprintf((char *)sendbuf, "SET-SSM-OTH:%s::%c;", ctag, out_ssm_threshold);
            sendtodown(sendbuf, (char)('a' + num));

            len1 = len2 = 0;
            if(conf_content.slot[num].ref_prio1[0] != 0xff)
            {
                len1 = strlen(conf_content.slot[num].ref_prio1);
            }
            if(conf_content.slot[num].ref_prio2[0] != 0xff)
            {
                len2 = strlen(conf_content.slot[num].ref_prio2);
            }
            if(len1 == 8)
            {
                if(len2 == 8)
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-REF-PRIO:%s::%s,%s;", ctag, conf_content.slot[num].ref_prio1,
                            conf_content.slot[num].ref_prio2);
                    sendtodown(sendbuf, (char)('a' + num));
                }
                else
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-REF-PRIO:%s::%s,;", ctag, conf_content.slot[num].ref_prio1);
                    sendtodown(sendbuf, (char)('a' + num));
                }
            }
            else
            {
                if(len2 == 8)
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-REF-PRIO:%s::,%s;", ctag, conf_content.slot[num].ref_prio2);
                    sendtodown(sendbuf, (char)('a' + num));
                }
            }

            len1 = len2 = 0;
            if(conf_content.slot[num].ref_mod1[0] != 0xff)
            {
                len1 = strlen(conf_content.slot[num].ref_mod1);
            }
            if(conf_content.slot[num].ref_mod2[0] != 0xff)
            {
                len2 = strlen(conf_content.slot[num].ref_mod2);
            }
            if(len1 == 3)
            {
                if(len2 == 3)
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-REF-MOD:%s::%s,%s;", ctag, conf_content.slot[num].ref_mod1,
                            conf_content.slot[num].ref_mod1);
                    sendtodown(sendbuf, (char)('a' + num));
                }
                else
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-REF-MOD:%s::%s,,;", ctag, conf_content.slot[num].ref_mod1);
                    sendtodown(sendbuf, (char)('a' + num));
                }
            }
            else
            {
                if(len2 == 3)
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-REF-MOD:%s::,,%s;", ctag, conf_content.slot[num].ref_mod2);
                    sendtodown(sendbuf, (char)('a' + num));
                }
            }

        }
		//
		//!RS422 SF16
		//
        else if(slot_type[num] == 'T' || slot_type[num] == 'X')
        {
            if(conf_content.slot[num].rs_en[0] != 0xff)
            {

                if(strlen(conf_content.slot[num].rs_en) == 4)
                {
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-RS-EN:%s::%s;", ctag, conf_content.slot[num].rs_en);
                    sendtodown(sendbuf, (char)('a' + num));
                }
            }


            j = atoi(conf_content.slot_u.leap_num);
            if(((j < 0) || (j > 99)) && (strncmp1(conf_content.slot_u.leap_num, "00", 2) != 0))
            {
                if(conf_content.slot[num].rs_tzo[0] != 0xff)
                {
                    if(strlen(conf_content.slot[num].rs_tzo) == 4)
                    {
                        memset(sendbuf, '\0', SENDBUFSIZE);
                        sprintf((char *)sendbuf, "SET-RS-TZO:%s::%s,;", ctag, conf_content.slot[num].rs_tzo);
                        sendtodown(sendbuf, (char)('a' + num));
                    }
                }
            }
            else
            {

                if(conf_content.slot[num].rs_tzo[0] != 0xff)
                {
                    if(strlen(conf_content.slot[num].rs_tzo) == 4)
                    {

                        memset(sendbuf, '\0', SENDBUFSIZE);
                        sprintf((char *)sendbuf, "SET-RS-TZO:%s::%s,%s;", ctag,
                                conf_content.slot[num].rs_tzo, conf_content.slot_u.leap_num);
                        sendtodown(sendbuf, (char)('a' + num));
                        return ;
                    }
                }
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-RS-TZO:%s::,%s;", ctag, conf_content.slot_u.leap_num);
                sendtodown(sendbuf, (char)('a' + num));
            }
        }
		//
		//!PTP OUT 
		//
        else if(((slot_type[num] > 0x40) && (slot_type[num] < 0x49)) || ((slot_type[num] > 109) && (slot_type[num] < 114)))
        {
            j = atoi(conf_content.slot_u.leap_num);
            if(strncmp1(conf_content.slot_u.leap_num, "00", 2) == 0)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-PTP-LEAP:%s::%s;", ctag, conf_content.slot_u.leap_num);
                sendtodown(sendbuf, (char)('a' + num));
            }
            else if((j < 1) || (j > 99))
            {}
            else
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-PTP-LEAP:%s::%s;", ctag, conf_content.slot_u.leap_num);
                sendtodown(sendbuf, (char)('a' + num));
            }

            //						if((slot_type[num] == 110) || (slot_type[num] == 111)) out_port = 1;
            //						else if((slot_type[num] == 112) || (slot_type[num] == 113)) out_port = 2;
            //						else out_port = 4;
            
			//2015.9.9  初始化ptp的协议
			if( '0' == conf_content3.mcp_protocol ||
				'1' == conf_content3.mcp_protocol ||
				'2' == conf_content3.mcp_protocol   )
			{
				printf("###init protocol ptp --%d\n",num);
				memset(sendbuf, '\0', SENDBUFSIZE);
				sprintf((char *)sendbuf, "SET-PROTOCOL:%s::%c;", ctag, conf_content3.mcp_protocol);
                sendtodown(sendbuf, (char)('a' + num));
				
			}
			else
			{
				;
			}
			
            for(j = 0; j < out_port; j++)
            {
                if(conf_content.slot[num].ptp_type == slot_type[num])
                {
                    
                   
                    /*memset(sendbuf, 0, SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-PTP-MTC:%c,%d,%s;",
                            conf_content.slot[num].ptp_type, j + 1, conf_content3.ptp_mtc1_ip[num][j]);
					printf("###run sendbuf mtc --%s\n",sendbuf);
                    sendtodown(sendbuf, (char)('a' + num));
                    usleep(200000);*/
					//conf_content3.ptp_mtc1_ip[num][port]

                    /*MODE*/
                    flag = 0;
                    if((conf_content.slot[num].ptp_en[j] != '0') && (conf_content.slot[num].ptp_en[j] != '1'))
                    {
                        memcpy(tmp1, ",", 1);
						tmp1[1] = '\0';
                    }
                    else
                    {
                        sprintf((char *)tmp1, "%c,", conf_content.slot[num].ptp_en[j]);
                        tmp1[2] = '\0';
                        flag = 1;
                    }
                    if((conf_content.slot[num].ptp_delaytype[j] != '0') && (conf_content.slot[num].ptp_delaytype[j] != '1'))
                    {
                        memcpy(tmp2, ",", 1);
						tmp2[1] = '\0';
                    }
                    else
                    {
                        sprintf((char *)tmp2, "%c,", conf_content.slot[num].ptp_delaytype[j]);
                        tmp2[2] = '\0';
                        flag = 1;
                    }
                    if((conf_content.slot[num].ptp_multicast[j] != '0') && (conf_content.slot[num].ptp_multicast[j] != '1'))
                    {
                        memcpy(tmp3, ",", 1);
						tmp3[1] = '\0';
                    }
                    else
                    {
                        sprintf((char *)tmp3, "%c,", conf_content.slot[num].ptp_multicast[j]);
                        tmp3[2] = '\0';
                        flag = 1;
                    }
                    if((conf_content.slot[num].ptp_enp[j] != '0') && (conf_content.slot[num].ptp_enp[j] != '1'))
                    {
                        memcpy(tmp4, ",", 1);
						tmp4[1] = '\0';
                    }
                    else
                    {
                        sprintf((char *)tmp4, "%c,", conf_content.slot[num].ptp_enp[j]);
                        tmp4[2] = '\0';
                        flag = 1;
                    }
                    if((conf_content.slot[num].ptp_step[j] != '0') && (conf_content.slot[num].ptp_step[j] != '1'))
                    {
                        memcpy(tmp5, ",", 1);
						tmp5[1] = '\0';
                    }
                    else
                    {
                        sprintf((char *)tmp5, "%c,", conf_content.slot[num].ptp_step[j]);
                        tmp5[2] = '\0';
                        flag = 1;
                    }
                    if(conf_content.slot[num].ptp_sync[j][0] != 0xff)
                    {
                        if(strlen(conf_content.slot[num].ptp_sync[j]) == 2)
                        {
                            flag = 1;
                        }
                        else
                        {
                            memset(conf_content.slot[num].ptp_sync[j], 0, 3);
                        }
                    }
                    else
                    {
                        memset(conf_content.slot[num].ptp_sync[j], 0, 3);
                    }
                    if(conf_content.slot[num].ptp_announce[j][0] != 0xff)
                    {
                        if(strlen(conf_content.slot[num].ptp_announce[j]) == 2)
                        {
                            flag = 1;
                        }
                        else
                        {
                            memset(conf_content.slot[num].ptp_announce[j], 0, 3);
                        }
                    }
                    else
                    {
                        memset(conf_content.slot[num].ptp_announce[j], 0, 3);
                    }
                    if(conf_content.slot[num].ptp_delayreq[j][0] != 0xff)
                    {
                        if(strlen(conf_content.slot[num].ptp_delayreq[j]) == 2)
                        {
                            flag = 1;
                        }
                        else
                        {
                            memset(conf_content.slot[num].ptp_delayreq[j], 0, 3);
                        }
                    }
                    else
                    {
                        memset(conf_content.slot[num].ptp_delayreq[j], 0, 3);
                    }

                    if(conf_content.slot[num].ptp_pdelayreq[j][0] != 0xff)
                    {
                        if(strlen(conf_content.slot[num].ptp_pdelayreq[j]) == 2)
                        {
                            flag = 1;
                        }
                        else
                        {
                            memset(conf_content.slot[num].ptp_pdelayreq[j], 0, 3);
                        }
                    }
                    else
                    {
                        memset(conf_content.slot[num].ptp_pdelayreq[j], 0, 3);
                    }

                    if(conf_content.slot[num].ptp_delaycom[j][0] != 0xff)
                    {
                        len = strlen(conf_content.slot[num].ptp_delaycom[j]);
                        if((len > 0) && (len < 9))
                        {
                            flag = 1;
                        }
                        else
                        {
                            memset(conf_content.slot[num].ptp_delaycom[j], 0, 9);
                        }
                    }
                    else
                    {
                        memset(conf_content.slot[num].ptp_delaycom[j], 0, 9);
                    }
                    if((conf_content.slot[num].ptp_linkmode[j] != '0') && (conf_content.slot[num].ptp_linkmode[j] != '1'))
                    {
                        memcpy(tmp6, ",", 1);
						tmp6[1] = '\0';
						tmp6[2] = '\0';	
                    }
                    else
                    {
                        sprintf((char *)tmp6, "%c,", conf_content.slot[num].ptp_linkmode[j]);
                        tmp6[2] = '\0';
                        flag = 1;
                    }
                    if((conf_content.slot[num].ptp_out[j] != '0') && (conf_content.slot[num].ptp_out[j] != '1'))
                    {
                        //memcpy(tmp7, ";", 1);
                        memcpy(tmp7, ",", 1); //2014.12.3
                        tmp7[1] = '\0';
						tmp7[2] = '\0';	
                    }
                    else
                    {
                        sprintf((char *)tmp7, "%c,", conf_content.slot[num].ptp_out[j]); //2014.12.3
                        tmp7[2] = '\0';
                        flag = 1;
                    }
                    //2014.12.3
                    if(conf_content3.ptp_prio1[num][j][0] != 'A')
                    {
                        memset(&conf_content3.ptp_prio1[num][j], 0, 5);
                        memcpy(&conf_content3.ptp_prio1[num][j], "A128", 4);
                    }
                    else
                    {
                        ;
                    }

                    if(conf_content3.ptp_prio2[num][j][0] != 'B')
                    {
                        memset(&conf_content3.ptp_prio2[num][j], 0, 5);
                        memcpy(&conf_content3.ptp_prio2[num][j], "B128", 4);
                    }
                    else
                    {
                        ;
                    }
                    if(flag == 1)
                    {
                        //2014.12.3
                        //ptp配置一次低于8个选项最为安全
                        //为保险起见,将命令分两次发送
						memset(sendbuf, '\0', SENDBUFSIZE);
                        //SET-PTP-MOD:%c,%d, , , , , ,  ,  ,  ,,,,,%s;
                        //<num>,<eable>,<delaytype>,<multicast>,<enp>,<step>,<sync>,<announce>,<delayreq>,<pdelayreq>,<delaycom>,<linkmode>，<outtype>,[< prio >]
                        sprintf((char *)sendbuf, "SET-PTP-MOD:%c,%d,,,,,,,,,,,,,%s;",
                                conf_content.slot[num].ptp_type,
                                (j + 1),
                                conf_content3.ptp_prio2[num][j]
                               );     
                        sendtodown(sendbuf, (char)('a' + num));
                        usleep(200000);
						
                        memset(sendbuf, '\0', SENDBUFSIZE);
                        //SET-PTP-MOD:%c,%d, , , , , ,  ,  ,  ,,,,,%s;
                        //<num>,<eable>,<delaytype>,<multicast>,<enp>,<step>,<sync>,<announce>,<delayreq>,<pdelayreq>,<delaycom>,<linkmode>，<outtype>,[< prio >]
                        sprintf((char *)sendbuf, "SET-PTP-MOD:%c,%d,%s%s%s%s%s%s,%s,%s,%s,%s,%s%s%s;",
                                conf_content.slot[num].ptp_type,
                                (j + 1),
                                tmp1, tmp2, tmp3,tmp4, tmp5,
                                conf_content.slot[num].ptp_sync[j],
                                conf_content.slot[num].ptp_announce[j],
                                conf_content.slot[num].ptp_delayreq[j],
								conf_content.slot[num].ptp_pdelayreq[j],
								conf_content.slot[num].ptp_delaycom[j],
								tmp6, tmp7,
                                conf_content3.ptp_prio1[num][j]
                               );/*%s%s%s,%s  tmp4, tmp5,
                                conf_content.slot[num].ptp_sync[j],
                                conf_content.slot[num].ptp_announce[j],*/
                        //printf("ptp init send1 %s\n",sendbuf);      
                        sendtodown(sendbuf, (char)('a' + num));
						delay_s(7,0);

						

                        /*memset(sendbuf, 0, SENDBUFSIZE);
                        sprintf((char *)sendbuf, "SET-PTP-MOD:%c,%d,,,,,,,,%s,%s,%s,,%s;",
                                conf_content.slot[num].ptp_type,
                                (j + 1),
                                conf_content.slot[num].ptp_delayreq[j],
                                conf_content.slot[num].ptp_pdelayreq[j],
                                conf_content.slot[num].ptp_delaycom[j],
                                
                                conf_content3.ptp_prio2[num][j]
                               );
                        //printf("ptp init send3 %s\n",sendbuf);
                        sendtodown(sendbuf, (char)('a' + num));
                        usleep(200000);

						memset(sendbuf, 0, SENDBUFSIZE);
                        sprintf((char *)sendbuf, "SET-PTP-MOD:%c,%d,,,,,,,,,,,%s%s;",
                                conf_content.slot[num].ptp_type,
                                (j + 1),
                                tmp6, tmp7
                               );
                        //printf("ptp init send4 %s\n",sendbuf);
                        sendtodown(sendbuf, (char)('a' + num));
                        usleep(200000);

						memset(sendbuf, '\0', SENDBUFSIZE);
                        //SET-PTP-MOD:%c,%d, , , , , ,  ,  ,  ,,,,,%s;
                        sprintf((char *)sendbuf, "SET-PTP-MOD:%c,%d,,,,%s%s%s,%s,,,,,,;",
                                conf_content.slot[num].ptp_type,
                                (j + 1),
                                 tmp4, tmp5,
                                conf_content.slot[num].ptp_sync[j],
                                conf_content.slot[num].ptp_announce[j]
                                
                               );
						//printf("ptp init send2 %s\n",sendbuf);
                        sendtodown(sendbuf, (char)('a' + num));
                        delay_s(5,0);*/

                    }
					memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-PTP-NET:%c,%d,%s,%s,%s,%s,%s,%s;",
                            conf_content.slot[num].ptp_type, (j + 1),
                            conf_content.slot[num].ptp_ip[j],
                            conf_content.slot[num].ptp_mac[j],
                            conf_content.slot[num].ptp_gate[j],
                            conf_content.slot[num].ptp_mask[j],
                            conf_content.slot[num].ptp_dns1[j],
                            conf_content.slot[num].ptp_dns2[j]);
					//printf("###run sendbuf net --%s\n",sendbuf);
                    sendtodown(sendbuf, (char)('a' + num));
                    delay_s(3,0);

                }
            }
        }
		else if('j' == slot_type[num])
		{
		    //
		    //!闰秒值下发
		    //
		    j = atoi(conf_content.slot_u.leap_num);
            if(strncmp1(conf_content.slot_u.leap_num, "00", 2) == 0)
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-PTP-LEAP:%s::%s;", ctag, conf_content.slot_u.leap_num);
                sendtodown(sendbuf, (char)('a' + num));
            }
            else if((j < 1) || (j > 99))
            {
               ;
            }
            else
            {
                memset(sendbuf, '\0', SENDBUFSIZE);
                sprintf((char *)sendbuf, "SET-PTP-LEAP:%s::%s;", ctag, conf_content.slot_u.leap_num);
                sendtodown(sendbuf, (char)('a' + num));
            }
			//
			//!协议下发(联通/移动/工信)
			//
			if( '0' == conf_content3.mcp_protocol ||
				'1' == conf_content3.mcp_protocol ||
				'2' == conf_content3.mcp_protocol   )
			{
				printf("###init protocol ptp --%d\n",num);
				memset(sendbuf, '\0', SENDBUFSIZE);
				sprintf((char *)sendbuf, "SET-PROTOCOL:%s::%c;", ctag, conf_content3.mcp_protocol);
                sendtodown(sendbuf, (char)('a' + num));
				printf("SET-PROTOCOL %c\n",conf_content3.mcp_protocol);
			}
			else
			{
				;
			}
			//
			//!设置网络参数，工作模式，单播对端IP地址，光模块配置；
			//
			for(j = 0; j < out_port; j++)
            {
			    if(conf_content.slot[num].ptp_type == slot_type[num]) 
			    {
			        //
			        //!Initial the net parameters。
			        //
			        memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-PGE4V-NET:%c,%d,,%s,%s,%s;",
                            conf_content.slot[num].ptp_type, (j + 1),
                           // conf_content.slot[num].ptp_mac[j],
                            conf_content.slot[num].ptp_ip[j],
                            conf_content.slot[num].ptp_mask[j],
                            conf_content.slot[num].ptp_gate[j]
                            );
                    sendtodown(sendbuf, (char)('a' + num));
                    usleep(200000);
                    //
                    //!Set the unicast dest parameters。
                    //
					memset(sendbuf, 0, SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-PGE4V-MTC:%c,%d,%s;",
                            conf_content.slot[num].ptp_type, j + 1, conf_content3.ptp_mtc_ip[num][j]);
                    sendtodown(sendbuf, (char)('a' + num));
                    usleep(200000);

					//
					//!Check the parameters corrected and Set Mode parameters。
					//
					flag = 0;
					if((conf_content.slot[num].ptp_en[j] != '0') && (conf_content.slot[num].ptp_en[j] != '1'))
                        conf_content.slot[num].ptp_en[j] = '\0';
					else
						flag = 1;
					
					if((conf_content3.ptp_esmcen[num][j] != '0') && (conf_content3.ptp_esmcen[num][j] != '1'))
					    conf_content3.ptp_esmcen[num][j] = '\0';
				    else
						flag = 1;
					
					if((conf_content.slot[num].ptp_delaytype[j] != '0') && (conf_content.slot[num].ptp_delaytype[j] != '1'))
						conf_content.slot[num].ptp_delaytype[j] = '\0';
					else 
						flag = 1;

					if((conf_content.slot[num].ptp_multicast[j] != '0') && (conf_content.slot[num].ptp_multicast[j] != '1'))
						conf_content.slot[num].ptp_multicast[j] = '\0';
					else
						flag = 1;

					if((conf_content.slot[num].ptp_enp[j] != '0') && (conf_content.slot[num].ptp_enp[j] != '1') )
						conf_content.slot[num].ptp_enp[j] = '\0';
					else
						flag = 1;

					if((conf_content.slot[num].ptp_step[j] != '0') && (conf_content.slot[num].ptp_step[j] != '1'))
						conf_content.slot[num].ptp_step[j] = '\0';
					else
						flag = 1; 

					//
                    //!Set the ptp mode。
                    //

					if(flag == 1)
					{
						memset(sendbuf, 0, SENDBUFSIZE);
	                    sprintf((char *)sendbuf, "SET-PGE4V-MOD:%c,%d,%c,%c,%c,%c,%c,%c;",
	                            conf_content.slot[num].ptp_type, j + 1, conf_content.slot[num].ptp_en[j],
	                                                                    conf_content3.ptp_esmcen[num][j],
	                                                                    conf_content.slot[num].ptp_delaytype[j],
	                                                                    conf_content.slot[num].ptp_multicast[j],
	                                                                    conf_content.slot[num].ptp_enp[j],
	                                                                    conf_content.slot[num].ptp_step[j]
	                                                                    );
	                    sendtodown(sendbuf, (char)('a' + num));
	                    usleep(200000);
					}
					//
					//!Set the ptp par。
					//
					flag = 0;
					if((strlen(conf_content3.ptp_dom[num][j]) > 0) && (strlen(conf_content3.ptp_dom[num][j]) <= MAX_DOM_LEN))
				    	flag = 1;
					else
						memset(conf_content3.ptp_dom[num][j],'\0', 4);

					if(strlen(conf_content.slot[num].ptp_sync[j]) == FRAME_FRE_LEN)
						flag = 1;
					else
						memset(conf_content.slot[num].ptp_sync[j],'\0',3);

					if(strlen(conf_content.slot[num].ptp_announce[j]) == FRAME_FRE_LEN)
						flag = 1;
					else
						memset(conf_content.slot[num].ptp_announce[j],'\0',3);

					if((strlen(conf_content.slot[num].ptp_delaycom[j]) > 0) &&(strlen(conf_content.slot[num].ptp_delaycom[j]) <= DELAY_COM_LEN))
						flag = 1;
					else
						memset(conf_content.slot[num].ptp_delaycom[j],'\0',9);
					

					if(conf_content3.ptp_prio1[num][j][0] != 'A')
                    {
                        memset(&conf_content3.ptp_prio1[num][j], 0, 5);
                        memcpy(&conf_content3.ptp_prio1[num][j], "A128", 4);
                    }
                    else
                    {
                        ;
                    }

                    if(conf_content3.ptp_prio2[num][j][0] != 'B')
                    {
                        memset(&conf_content3.ptp_prio2[num][j], 0, 5);
                        memcpy(&conf_content3.ptp_prio2[num][j], "B128", 4);
                    }
                    else
                    {
                        ;
                    }
					if(flag == 1)
					{
					    memset(sendbuf, 0, SENDBUFSIZE);
	                    sprintf((char *)sendbuf, "SET-PGE4V-PAR:%c,%d,%s,%s,%s,%s,%s;",
	                            conf_content.slot[num].ptp_type, j + 1, conf_content3.ptp_dom[num][j],
	                                                                    conf_content.slot[num].ptp_sync[j],
	                                                                    conf_content.slot[num].ptp_announce[j],
	                                                                    conf_content.slot[num].ptp_delaycom[j],
	                                                                    conf_content3.ptp_prio1[num][j]
	                                                                    );
	                    sendtodown(sendbuf, (char)('a' + num));
	                    usleep(200000);
						memset(sendbuf, 0, SENDBUFSIZE);
	                    sprintf((char *)sendbuf, "SET-PGE4V-PAR:%c,%d,,,,,%s;",
	                            conf_content.slot[num].ptp_type, j + 1, conf_content3.ptp_prio2[num][j]
	                                                                    );
	                    sendtodown(sendbuf, (char)('a' + num));
	                    usleep(200000);
					}
					//
					//!sfp set
					//
					flag = 0;
					if(strlen(conf_content3.ptp_sfp[num][j]) == 2)
						flag = 1;
					else
						memset(conf_content3.ptp_sfp[num][j],'\0',3);

					if((strlen(conf_content3.ptp_oplo[num][j]) > 0) && (strlen(conf_content3.ptp_oplo[num][j]) <= MAX_OPTHR_LEN))
						flag = 1;
					else
						memset(conf_content3.ptp_oplo[num][j],'\0',6);

					if((strlen(conf_content3.ptp_ophi[num][j]) > 0) && (strlen(conf_content3.ptp_ophi[num][j]) <= MAX_OPTHR_LEN))
						flag = 1;
					else
						memset(conf_content3.ptp_ophi[num][j],'\0',6);

					if(flag == 1)
					{
					     memset(sendbuf, 0, SENDBUFSIZE);
	                     sprintf((char *)sendbuf, "SET-PGE4V-SFP:%c,%d,%s,%s,%s;",
	                            conf_content.slot[num].ptp_type, j + 1, conf_content3.ptp_sfp[num][j],
	                                                                    conf_content3.ptp_oplo[num][j],
	                                                                    conf_content3.ptp_ophi[num][j]   
	                                                                    );
	                    sendtodown(sendbuf, (char)('a' + num));
	                    usleep(200000);
					}
						
			    }
			}
		}
		else if('s' == slot_type[num])
		{
		    if(conf_content.slot[num].ptp_type == slot_type[num])
			{
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-PGEIN-NET:%c,%d,%s,%s,%s;",conf_content.slot[num].ptp_type, 1,
                                                                                                                conf_content.slot[num].ptp_ip[0],
                                                                                                                conf_content.slot[num].ptp_gate[0],
                                                                                                                conf_content.slot[num].ptp_mask[0]);
                    sendtodown(sendbuf, (char)('a' + num));
                    usleep(200000);
                    memset(sendbuf, '\0', SENDBUFSIZE);
                    sprintf((char *)sendbuf, "SET-PGEIN-MOD:%c,%d,%c,%c,%c,%c,%s,%s;",conf_content.slot[num].ptp_type,1,
                                                                                                                 conf_content.slot[num].ptp_delaytype[0],
                                                                                                                 conf_content.slot[num].ptp_multicast[0], 
                                                                                                                 conf_content.slot[num].ptp_enp[0],
                                                                                                                 conf_content.slot[num].ptp_step[0],
                                                                                                                 conf_content.slot[num].ptp_delayreq[0],
                                                                                                                 conf_content.slot[num].ptp_pdelayreq[0]);
					sendtodown(sendbuf, (char)('a' + num));
					
                               
                    
  			}  
		}
        else if( 'Z' == slot_type[num])
        {
            memset(sendbuf, '\0', SENDBUFSIZE);
            sprintf((char *)sendbuf, "SET-NTP-LEAP:%s::%s;", ctag, conf_content.slot_u.leap_num);
            sendtodown(sendbuf, (char)('a' + num));
        }
        else if( 'm' == slot_type[num])
        {
            init_sta1(num, sendbuf);
        }
    }
}




int read_config3(FILE *pf)
{
    if( pf == NULL )
    {
        return -1;
    }

    if(0 != fseek(pf, sizeof(file_content) + sizeof(struct extsave), SEEK_SET))
    {
        return 0;
    }

    memset(&conf_content3, 0, sizeof(file_content3));
    if(1 != fread((void *)&conf_content3, sizeof(file_content3), 1, pf))
    {
        return 0;
    }
    if('T' != conf_content3.flashMask)
    {
        memset(&conf_content3, 0, sizeof(file_content3));
		if(0 != fseek(pf, sizeof(file_content) + sizeof(struct extsave), SEEK_SET))
        {
           return 0;
        }
        fwrite((void *)&conf_content3, sizeof(file_content3), 1, pf);
		fflush(pf);
		sync();
        system("fw -ul -f /var/p300file -o 0x1f0000 /dev/rom1");
    }
    else
    {
    }
    return 1;

}

void Init_filesystem()
{
    FILE  *file_fd;
    int rc;
    //int i;
    //Uint16 value;
    rc = system("dd if=/dev/rom1 of=/var/p300file bs=65536 count=1 skip=31");
    if(rc != 0)
    {
        printf("dd failed\n");
        perror("system");
    }
	sync();
    if((file_fd = fopen(FILE_DEV, "rb+")) == NULL) /*打开文件失败*/
    {
        printf("only read Open FILE_DEV Error:\n");
        perror("fopen");

    }
    else
    {
        //printf("\nopen ok!\n");
        memset(&conf_content, '\0', sizeof(file_content));
        rc = fread((void *)&conf_content, sizeof(file_content), 1, file_fd);
        printf("p300-0 flashmask=%c\n", conf_content.flashMask);
        if('T' != conf_content.flashMask)
        {
            memset(&conf_content, '\0', sizeof(file_content));
			if(0 != fseek(file_fd, 0, SEEK_SET))
    		{
        		printf("only SEEK_SET FILE_DEV Error:\n");
                perror("SEEK_SET");
    		}
            fwrite((void *)&conf_content, sizeof(file_content), 1, file_fd);
			fflush(file_fd);
			sync();
            system("fw -ul -f /var/p300file -o 0x1f0000 /dev/rom1");
        }
        read_config3(file_fd);
        fclose(file_fd);
		sync();
#if 0
        if((file_fd = fopen(FILE_DEV, "wb+")) < 0)
        {
            printf("only write open  FILE_DEV Error:\n");
            perror("fopen");
        }


        printf("\nbefor fwrite\n");
        rc = fwrite((void *)&conf_content, sizeof(file_content), 1, file_fd);
        fclose(file_fd);
        printf("\nbefor fw\n");
        system("fw -ul -f /var/p210file -o 0x1f0000 /dev/rom1");

        //config_enet();
#endif
      if(conf_content3.mcp_protocol <'0' ||conf_content3.mcp_protocol >'2')

	    conf_content3.mcp_protocol = '1';
    }
}






void save_config()
{
    FILE  *file_fd;
    if((file_fd = fopen(FILE_DEV, "rb+")) == NULL)
    {
        printf("Open FILE_DEV Error:\n");

    }
    else
    {
        conf_content.flashMask = 'T';
        fwrite((void *)&conf_content, sizeof(file_content), 1, file_fd);


        conf_content3.flashMask = 'T';

        //2014.12.2
        if(0 != fseek(file_fd, sizeof(file_content) + sizeof(struct extsave), SEEK_SET))
        {
            //return 0;
        }
        else
        {
            fwrite((void *)&conf_content3, sizeof(file_content3), 1, file_fd);
        }

        fflush(file_fd);
        fclose(file_fd);
		sync();
        system("fw -ul -f /var/p300file -o 0x1f0000 /dev/rom1");
        sync();

    }
}


/***********************************************************************************
函数名:	send_arppacket
功能:		构建arp包，以garp包告知网段内ip更新
参数:		无
返回值:	无
************************************************************************************/
void send_arppacket()
{
    struct sockaddr sa;
    int sock;
    unsigned int ip_addr;

    struct arp_packet
    {
        unsigned char targ_hw_addr[6];
        unsigned char src_hw_addr[6];
        unsigned short frame_type;
        unsigned short hw_type;
        unsigned short prot_type;
        unsigned char hw_addr_size;
        unsigned char prot_addr_size;
        unsigned short op;
        unsigned char sndr_hw_addr[6];
        unsigned char sndr_ip_addr[4];
        unsigned char rcpt_hw_addr[6];
        unsigned char rcpt_ip_addr[4];
        unsigned char padding[18];
    } arphead;

    void get_hw_addr(char *buf, char *str);

    sock = socket(AF_INET, SOCK_PACKET, htons(0x0003));
    arphead.frame_type = htons(0x0806);
    arphead.hw_type = htons(1);
    arphead.prot_type = htons(0x0800);
    arphead.hw_addr_size = 6;
    arphead.prot_addr_size = 4;
    arphead.op = htons(1);
    bzero(arphead.padding, 18);

    get_hw_addr(arphead.targ_hw_addr, "ff:ff:ff:ff:ff:ff");
    get_hw_addr(arphead.rcpt_hw_addr, "ff:ff:ff:ff:ff:ff");

    get_hw_addr(arphead.src_hw_addr, conf_content.slot_u.mac);
    get_hw_addr(arphead.sndr_hw_addr, conf_content.slot_u.mac);

    ip_addr = inet_addr(conf_content.slot_u.ip);

    memcpy(arphead.sndr_ip_addr, &ip_addr, 4);
    memcpy(arphead.rcpt_ip_addr, &ip_addr, 4);
    strcpy(sa.sa_data, "eth0");

    if (sendto(sock, &arphead, sizeof(arphead), 0, &sa, sizeof(sa)) < 0)
    {
        perror("sendto");
        return ;
    }
    close(sock);
    printf("Garp send ok\n");
    return ;
}

/***********************************************************************************
函数名:	get_hw_addr
功能:		将mac地址转换为16进制
参数:		char类型的mac地址
返回值:	16进制数据，存放于字节中
************************************************************************************/

void get_hw_addr(char *buf, char *str)
{
    int i;
    char c, val;
    for (i = 0; i < 6; i++)
    {
        c = tolower(*str++);
        if (isdigit(c))
        {
            val = c - '0';
        }
        else
        {
            val = c - 'a' + 10;
        }
        *buf = val << 4;

        c = tolower(*str++);
        if (isdigit(c))
        {
            val = c - '0';
        }
        else
        {
            val = c - 'a' + 10;
        }
        *buf++ |= val;

        if (*str == ':')
        {
            str++;
        }
    }
}


void config_enet()
{
    int len;
    unsigned char  mcp_ip[16];
    unsigned char  mcp_gateway[16];
    unsigned char  mcp_mask[16];
    unsigned char  mcp_mac[18];

    memset( mcp_ip, '\0', sizeof(unsigned char) * 16);
    memset( mcp_gateway, '\0', sizeof(unsigned char) * 16);
    memset( mcp_mask, '\0', sizeof(unsigned char) * 16);
    memset( mcp_mac, '\0', sizeof(unsigned char) * 18);

    API_Get_McpIp(mcp_ip);
    API_Get_McpGateWay(mcp_gateway);
    len = strlen(mcp_gateway);
    if(len < 7)
    {
        memset( mcp_gateway, '\0', sizeof(unsigned char) * 16);
    }
    API_Get_McpMask(mcp_mask);
    API_Get_McpMacAddr(mcp_mac);
    if(strncmp1(conf_content.slot_u.tid, "LT", 2) != 0)
    {
        memcpy(conf_content.slot_u.tid, "LT00000000000005", 16);
        printf("tid=%s\n", conf_content.slot_u.tid);

        //mcp_init
        memcpy(conf_content.slot_u.ip, mcp_ip, strlen(mcp_ip));
        memcpy(conf_content.slot_u.mask, mcp_mask, strlen(mcp_mask));
        memcpy(conf_content.slot_u.gate, "192.168.1.1", 11);
        API_Set_McpGateWay("192.168.1.1");
        //memcpy(conf_content.slot_u.dns1,"61.139.2.69",11);
        //memcpy(conf_content.slot_u.dns2,"61.139.2.69",11);
        memcpy(conf_content.slot_u.mac, mcp_mac, strlen(mcp_mac));

        memcpy(conf_content.slot_u.msmode, "MAIN", 4);
        conf_content.slot_u.time_source = '0';
        conf_content.slot_u.fb = '0';

        conf_content.slot_u.out_ssm_en = '1';
        conf_content.slot_u.out_ssm_oth = 0x04;
        //memcpy(conf_content.slot_u.leap_num,"00",2);
    }
    else
    {
        printf("tid=%s\n", conf_content.slot_u.tid);
		
        if(strcmp(conf_content.slot_u.ip, mcp_ip) != 0)
        {
            API_Set_McpIp(conf_content.slot_u.ip);
        }
       // if(strcmp(conf_content.slot_u.mask, mcp_mask) != 0)
        {
            API_Set_McpMask(conf_content.slot_u.mask);
			printf("mask=%s\n", conf_content.slot_u.mask);
        }
        if(strcmp(conf_content.slot_u.gate, mcp_gateway) != 0)
            if(if_a_string_is_a_valid_ipv4_address(conf_content.slot_u.gate) == 0)
            {
                API_Set_McpGateWay(conf_content.slot_u.gate);
            }
        if(strcmp(conf_content.slot_u.mac, mcp_mac) != 0)
        {
            API_Set_McpMacAddr(conf_content.slot_u.mac);
        }

        if((conf_content.slot_u.fb == '0') || (conf_content.slot_u.fb == '1'))
        {}
        else
        {
            conf_content.slot_u.fb = '0';
        }
		if( '0'!=conf_content.slot_u.time_source && '1'!=conf_content.slot_u.time_source )
		{
        	conf_content.slot_u.time_source = '0';
		}
    }
#if DEBUG_NET_INFO
    printf("%s:<ip>:<%s>\n", NET_INFO, conf_content.slot_u.ip);
    printf("%s:<gate>:<%s>\n", NET_INFO, conf_content.slot_u.gate);
    printf("%s:<mask>:<%s>\n", NET_INFO, conf_content.slot_u.mask);
    printf("%s:<mac>:<%s>\n", NET_INFO, conf_content.slot_u.mac);
#endif
    send_arppacket();//2014-5-19发送arp包，告知所有pc端mcp的ip与mac地址
}

int API_Set_McpIp(unsigned char  *mcp_local_ip)
{
    struct sockaddr_in sin;
    struct ifreq ifr;


    int s;
    //int ret;
    //unsigned char  *ptr;
    unsigned char  *eth_name = "eth0";
    memset(&ifr, '\0', sizeof(struct ifreq));
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if(s == -1)
    {
        //perror("Not create network socket connection\\n");
        return (-1);
    }
    strncpy(ifr.ifr_name, eth_name, IFNAMSIZ);
    ifr.ifr_name[IFNAMSIZ - 1] = 0;
    memset(&sin, '\0', sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = inet_addr(mcp_local_ip);

    memmove(&ifr.ifr_addr, &sin, sizeof(sin));
    if(ioctl(s, SIOCSIFADDR, &ifr) < 0)
    {
        return (-1);
    }
    ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
    if(ioctl(s, SIOCSIFFLAGS, &ifr) < 0)
    {
        return (-1);
    }
    close(s);

    return 0;
}


int API_Set_McpMask(unsigned char  *mcp_local_mask)
{
    int s;
    unsigned char  *interface_name = "eth0";
    struct ifreq ifr;
    struct sockaddr_in netmask_addr;

    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket");
        return (-1);
    }

    strcpy(ifr.ifr_name, interface_name);
    memset(&netmask_addr, '\0', sizeof(struct sockaddr_in));
    netmask_addr.sin_family = PF_INET;
    netmask_addr.sin_addr.s_addr = inet_addr(mcp_local_mask);

    memmove(&ifr.ifr_ifru.ifru_netmask, &netmask_addr, sizeof(struct sockaddr_in));

    if(ioctl(s, SIOCSIFNETMASK, &ifr) < 0)
    {
        perror("ioctl");
        return (-1);
    }
    close(s);

    return 0;
}

int API_Set_McpGateWay(unsigned char *mcp_local_gateway)
{
    unsigned char cmd[50] = "route add default gw ";
    struct sockaddr_in ip_gateway;

    ip_gateway.sin_addr.s_addr = inet_addr(mcp_local_gateway); //inet_addr("202.194.8.156");
    system("route del default");
    usleep(20);

    strcat(cmd, mcp_local_gateway);
    printf("%s\n", cmd);
    system(cmd);
    return 0;
}

int API_Set_McpMacAddr(unsigned char *mcp_local_mac)
{
    unsigned char  mac_add[60];
    memset(mac_add, '\0', 60);
    sprintf((char *)mac_add, "ifconfig eth0 hw ether %s", mcp_local_mac );
    system(mac_add);
    return 0;
}

int API_Get_McpIp(unsigned char  *mcp_local_ip)
{
    int s;
    struct ifreq ifr;
    struct sockaddr_in *ptr;
    unsigned char *interface_name = "eth0";
    if((s = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket");
        return (-1);
    }

    strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);

    if(ioctl(s, SIOCGIFADDR, &ifr) < 0)
    {
        perror("Get_McuLocalIp ioctl");
        return (-1);

    }
    ptr = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_addr;
    strcpy(mcp_local_ip, (unsigned char *)inet_ntoa (ptr->sin_addr)); // 获取IP地址
    close(s);

    //printf("get ip =%s\n",mcp_local_ip);
    return 0;
}

int API_Get_McpMask(unsigned char  *mcp_local_mask)
{
    int s;
    unsigned char *interface_name = "eth0";
    struct ifreq ifr;
    struct sockaddr_in *ptr;
    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket");
        return (-1);
    }

    strcpy(ifr.ifr_name, interface_name);

    if(ioctl(s, SIOCGIFNETMASK, &ifr) < 0)
    {
        perror("ioctl");
        return (-1);
    }


    ptr = (struct sockaddr_in *)&ifr.ifr_ifru.ifru_netmask;
    strcpy(mcp_local_mask , (unsigned char *)inet_ntoa(ptr->sin_addr));
    close(s);

    //printf("get mask =%s\n",mcp_local_mask);
    return 0;
}

int API_Get_McpGateWay(unsigned char  *mcp_local_gateway)
{
    int pl = 0, ph = 0;
    int i;
    FILE *rtreadfp;
    unsigned char  buf[1024];
    memset(buf, '\0', 1024);
    if((rtreadfp = popen( "route -n", "r" )) == NULL)
    {
    }
    fread( buf, sizeof(char), sizeof(buf),  rtreadfp);
    pclose( rtreadfp );

    for (i = 0; i < 1024; i++)
    {
        if (buf[i] == 'U'  && buf[i + 1] == 'G' && (i + 1) < 1024)
        {
            pl = i;
            break;
        }
    }
    while (buf[pl] > '9' || buf[pl] < '0')
    {
        pl--;
    }
    while (('0' <= buf[pl] && buf[pl] <= '9') || buf[pl] == '.')
    {
        pl--;
    }
    while (buf[pl] > '9' || buf[pl] < '0')
    {
        pl--;
    }
    buf[pl + 1] = '\0';
    for (i = pl; ('0' <= buf[i] && buf[i] <= '9') || buf[i] == '.'; i--);
    ph = i + 1;
    strcpy(mcp_local_gateway, &buf[ph]);
    return 0;
}

int API_Get_McpMacAddr(unsigned char  *mcp_local_mac)
{
    int s;
    struct ifreq ifr;
    unsigned char *ptr;
    unsigned char *interface_name = "eth0";
    unsigned char  buf[6];
    unsigned char  tmp[20];
    memset(buf, '\0', 6);
    memset(tmp, '\0', 20);
    if((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket");
    }

    strcpy(ifr.ifr_name, interface_name);

    if(ioctl(s, SIOCGIFHWADDR, &ifr) != 0)
    {
        perror("ioctl");
    }
    ptr = (unsigned char *)&ifr.ifr_ifru.ifru_hwaddr.sa_data[0];
    buf[0] = *ptr;
    buf[1] = *(ptr + 1);
    buf[2] = *(ptr + 2);
    buf[3] = *(ptr + 3);
    buf[4] = *(ptr + 4);
    buf[5] = *(ptr + 5);

    sprintf((char *)tmp, "%02x:%02x:%02x:%02x:%02x:%02x",
            buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
    strcpy(mcp_local_mac, tmp);
    //printf("mac:%s\n",mcp_local_mac);

    close(s);
    return 0;
}

int if_a_string_is_a_valid_ipv4_address(const char *str)
{
    struct in_addr addr;
    int ret;
    ret = inet_pton(AF_INET, str, &addr);
    if (ret > 0)
    {
        return 0;
    }
    else
    {
        return (-1);
    }
}


int judge_length(unsigned char *buf)
{
    int len;

    len = strlen_r(buf, ';') + 1;
    if(len > 2)
    {
        if(buf[1] == (len - 2))
        {
            return 0;
        }
        else
        {
            return (-1);
        }
    }
    else
    {
        return (-1);
    }
}

int judge_client_online(int *clt_num)
{
    int i, count;
    count = 0;
    for(i = 1; i < MAX_CLIENT; i++)
    {
        if(client_fd[i] != (-1))
        {
            clt_num[count] = i;
            count++;
        }
    }
    return count;
}




void judge_data(unsigned char *recvbuf, int len , int client)
{
    unsigned char ctag[7];
    unsigned char temp[20];
    int ret = 0;
    int len_t = 0;

    memset(ctag, 0, 7);
    memset(temp, 0, 20);

    if(len == 10)
    {
        if(strncmp1("YE", recvbuf, 2) == 0)
        {
            memcpy(ctag, &recvbuf[3], 6);
            respond_success(current_client, ctag);
        }
        else if(strncmp1("NO", recvbuf, 2) == 0)
        {
            memcpy(ctag, &recvbuf[3], 6);
            respond_fail(current_client, ctag, 2);
        }
        else if(strncmp1("BEGINNING", recvbuf, 9) == 0)
        {
            process_config();
        }
    }
    else if(strncmp1("MCP", recvbuf, 3) == 0)
    {
        len_t = strlen(recvbuf);
        strncpy(temp, &recvbuf[8], len_t - 9);
        //printf("%s\n",temp);
        if(strncmp1("COM", &recvbuf[4], 3) == 0)
        {
            if(recvbuf[9] == '0')
            {
                print_switch = 0;
                sendtodown("open", 'u');
            }
            else if(recvbuf[9] == '1')
            {
                print_switch = 1;
                sendtodown("clse", 'u');
            }
            else
            {
                return;
            }
        }
        else if(strncmp1("IPP", &recvbuf[4], 3) == 0)
        {

            if(if_a_string_is_a_valid_ipv4_address(temp) == 0)
            {
                ret = API_Set_McpIp(temp);
                if(ret == 0)
                {
                    memset(conf_content.slot_u.ip, '\0', 16);
                    memcpy(conf_content.slot_u.ip, temp, strlen(temp));
                }
                else
                {
                    return;
                }
            }
            else
            {
                return;
            }
        }
        else if(strncmp1("GAT", &recvbuf[4], 3) == 0)
        {
            if(if_a_string_is_a_valid_ipv4_address(temp) == 0)
            {
                ret = API_Set_McpGateWay(temp);
                if(ret == 0)
                {
                    memset(conf_content.slot_u.gate, '\0', 16);
                    memcpy(conf_content.slot_u.gate, temp, strlen(temp));
                }
                else
                {
                    return;
                }
            }
            else
            {
                return;
            }
        }
        else if(strncmp1("MSK", &recvbuf[4], 3) == 0)
        {
            if(if_a_string_is_a_valid_ipv4_address(temp) == 0)
            {
                ret = API_Set_McpMask(temp);
				//printf("MSK=");
				//printf("MSK=%s\n", temp);
                if(ret == 0)
                {
                    memset(conf_content.slot_u.mask, '\0', 16);
                    memcpy(conf_content.slot_u.mask, temp, strlen(temp));
                }
                else
                {
                    return;
                }
            }
            else
            {
                return;
            }
        }
        else if(strncmp1("MAC", &recvbuf[4], 3) == 0)
        {
            if(strlen(temp) == 17)
            {
                ret = API_Set_McpMacAddr(temp);
                if(ret == 0)
                {
                    memset(conf_content.slot_u.mac, '\0', 18);
                    memcpy(conf_content.slot_u.mac, temp, 17);
                }
                else
                {
                    return;
                }
            }
            else
            {
                return;
            }
        }
        else if(strncmp1("TID", &recvbuf[4], 3) == 0)
        {
            if(strlen(temp) == 16)
            {
                memset(conf_content.slot_u.tid, '\0', 18);
                memcpy(conf_content.slot_u.tid, temp, 16);
            }
            else
            {
                return;
            }
        }
        else
        {
            return;
        }
        save_config();
    }
    else if(strncmp1("TIME", recvbuf, 4) == 0)
    {
        get_time(recvbuf);
    }
    else if(strncmp1("FPGA", recvbuf, 4) == 0)
    {
        get_fpga_ver(recvbuf);
    }
    else if(strncmp1("R-", &recvbuf[2], 2) == 0) //判断为100s上报信息
    {
        if((-1) == judge_length(recvbuf))
        {
            printf("rpt_len error1\n");
        }
        else
        {
            process_report(recvbuf, len);
        }
    }
    else if(strncmp1("RALM", &recvbuf[2], 4) == 0)
    {
        if((-1) == judge_length(recvbuf))
        {
            printf("rpt_len error2\n");
        }
        else
        {
            process_ralm(recvbuf, len);
        }
    }
    else if(strncmp1("REVT", &recvbuf[2], 4) == 0)
    {
        if((-1) == judge_length(recvbuf))
        {
            printf("rpt_len error3\n");
        }
        else
        {
            process_revt(recvbuf, len);
        }
    }
    else
    {
        //printf("<%s>:'recvbuf':%s\n",__FUNCTION__,recvbuf);
        process_client(client, recvbuf, len); //接收到客户数据，开始处理
    }
}

int judge_buff(unsigned char *buf)
{
    int i;
    unsigned char data[MAXDATASIZE];
    unsigned char temp[20];
    unsigned char passwd[9];
    _MSG_NODE msg_config;
    int len_data;
    char slot;
    memset(data, 0, MAXDATASIZE);
    memset(temp, 0, 20);
    memset(passwd, 0, 9);
    for(i = 0; i < _CMD_NODE_MAX; i++)
    {
        if(strncmp1(g_cmd_fun[i].cmd , buf, g_cmd_fun[i].cmd_len) == 0)
        {
            //sscanf(s,"%[^:]:%[^:]:%[^;];",buf1,buf2,buf3);
            sscanf(buf, "%[^:]::%c::%[^:]:%[^;];", temp, &slot, passwd, data);
            memset(&msg_config, '\0', sizeof(_MSG_NODE));
            //memcpy(msg_config.cmd,&recvbuf[0],i1);
            //memcpy(msg_config.ctag,&recvbuf[i3],6);
            len_data = strlen(data);
            if(len_data > 0)
            {
                memcpy(msg_config.data, data, len_data);
            }
            else
            {
                return 0;
            }
            //msg_net.cmd_len=i1;
            msg_config.solt = slot;
            memcpy(msg_config.ctag, "000000", 6);
            g_cmd_fun[i].cmd_fun(&msg_config, 0);
            break;
        }

    }
    return 0;
}


int process_config()
{
    FILE *fp;
    unsigned char buf[100];
    char slot;
    if((fp = fopen(CONFIG_FILE, "rt")) == NULL)
    {
        printf("cannot open config \n");
		return 1;
    }
    else
    {
        printf(" open CONFIG \n");
    }

    printf("CONFIG start\n");
    while(!feof(fp))
    {
        fgets(buf, sizeof(buf), fp);
        if((buf[0] == '\r') || (buf[0] == '\0') ||  (buf[0] == '\n') )
        {
            continue;
        }
        else if((buf[0] == '#') || (buf[2] == '#'))
        {
            slot = buf[1];
        }
        else
        {
            judge_buff(buf);
            /*		p = buf+strlen("trap2sink ");
            		for(i=0; *p!='\t'&&*p!=' '; i++,p++)
            			str2[i]=*p;
            		str2[i]='\0';
            */
        }
    }
    fclose(fp);
    return 0;
}


int process_client(int client, char *recvbuf, int len)
{
    unsigned char ctag[7];
    unsigned char *noctag = "noctag";
    int i, j, i1, i2, i3, i4, i5, len_buf;
    _MSG_NODE msg_net;

    memset(ctag, '\0', sizeof(unsigned char) * 7);
    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
    i5 = 0;
    for(i = 2; i < len; i++)
    {
        if(recvbuf[i] == ':')
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
    if((j < 5) || (recvbuf[len - 1] != ';'))
    {
        //出错响应
        if((i4 - i3) == 6)
        {
            memcpy(ctag, &recvbuf[i3], i4 - i3);
        }
        else if((i4 - i3) == 0)
        {
            memcpy(ctag, noctag, 6);
        }

        respond_fail(client, ctag, 6);
        return 1;
    }
    else
    {
        //如果tid相同，赋值到结构体(密码功能暂无)
        if(strncmp1(&recvbuf[i1], conf_content.slot_u.tid, 16) == 0)
        {

#if DEBUG_NET_INFO
            if(print_switch == 0)
            {
                printf("%s:<from net>:%s\n", NET_INFO, recvbuf);
            }
#endif
            //printf("%s:<from net>:%s\n", NET_INFO, recvbuf);
            memset(&msg_net, '\0', sizeof(_MSG_NODE));
            memcpy(msg_net.cmd, &recvbuf[0], i1);
            memcpy(msg_net.ctag, &recvbuf[i3], 6);
            len_buf = len - i5 - 1;
            if(len_buf > 0)
            {
                memcpy(msg_net.data, &recvbuf[i5], len_buf);
            }
            msg_net.cmd_len = i1;
            msg_net.solt = recvbuf[i2];
            cmd_process(client, &msg_net);
        }
        else
        {
            //失败响应
            memcpy(ctag, &recvbuf[i3], 6);
            respond_fail(client, ctag, 5);

            return 1;
        }
    }

    return 0;
}




/***********************************************************************************
函数名:	event_filter
功能:		屏蔽掉多余事件
参数:		槽位号；
返回值:	无
************************************************************************************/

int event_filter(char *cmd_num)
{
    char *event_id = "C23,C26,C24,C27,C28,C29,C05,C06,C07,C08,C10,C31,C42,C43,C51,C52,C61,C62,C63,C64,C67,C71,C72,C81,CB1,CB2,CB3,CC1,CC2,CC3,CC4,";
    if( (NULL == cmd_num) || (cmd_num[0] != 'C') )
    {
        printf("event_filter para error\n");
        return 0;
    }
    else
    {
        if(strstr(event_id, cmd_num) != NULL)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
}

/***********************************************************************************
函数名:	event_filter
功能:		屏蔽掉多余事件
参数:		槽位号；
返回值:	无
************************************************************************************/

int event_filter_another(char *cmd_num)
{
    char *event_id = "C03,C31,C51,C52,C53,CC7,CC5,CD3,";
    if( (NULL == cmd_num) || (cmd_num[0] != 'C') )
    {
        printf("event_filter para error\n");
        return 0;
    }
    else
    {
        if(strstr(event_id, cmd_num) != NULL)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
}



/***********************************************************************************
函数名:	is_change
功能:		是否为重复上报
参数:		槽位号,上报参数
返回值:	无
************************************************************************************/
int is_change(int num, char *str)
{
    int ret = 1;
    int len = 0;
    if(num < 0)
    {
        printf("is_change error pare_num %d\n", num);
        return 0;
    }
    if(str == NULL)
    {
        printf("is_change error pare_str NULL\n");
        return 0;
    }

    len = strlen(str);
    ret = strncmp(event_old[num], str, len);
    if(ret == 0)
    {
        return 0;
    }
    else
    {
        memset(event_old[num], 0, 128);
        memcpy(event_old[num], str, 128);
        return 1;
    }
}



int process_report(unsigned char *recv, int len)
{
    int i, j, i1, i2, num;
    unsigned char  rpt_head[10];
    unsigned char  rpt_data[128];
    unsigned char  rpt_type;
    unsigned char  *buf_auto = "AUTO";

    //int i_temp,j_temp;
    memset(rpt_head, '\0', 10);
    memset(rpt_data, '\0', 128);
    j = 0;
    i1 = 0;
    i2 = 0;

    for(i = 2; i < len; i++)
    {
        if(recv[i] == ':')
        {
            j++;
            if(j == 1)
            {
                i1 = i + 1;
            }
        }
        else if(recv[i] == ',')
        {
            j++;
            if(j == 2)
            {
                i2 = i + 1;
            }
            break;
        }
    }
    if((i1 == 0) || (i2 == 0))
    {
        return 1;
    }

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:<from pan>:>%s<\n", NET_INFO, &recv[2]);
    }
#endif

    memcpy(rpt_head, &recv[2], i1 - 3);
    memcpy(rpt_data, &recv[i2], len - i2 - 1);
    rpt_type = recv[i1];

    num = (int)recv[0] - 0x01;
    if((num < 0) || (num > 17))
    {
        return 1;
    }

    for(i = 0; i < _NUM_RPT100S; i++)
    {
        if(strncmp1(RPT100S_fun[i].cmd , rpt_head , RPT100S_fun[i].cmd_len) == 0)
        {
            if((0 == memcmp(rpt_head, "R-OUT-CFG", 9)) || (0 == memcmp(rpt_head, "R-OUT-MGR", 9)))
            {
                if( (EXT_DRV != slot_type[num]) &&
                        ((12 == num) || (13 == num)) )
                {
                    slot_type[num] = EXT_DRV;
                    send_online_framing(num, EXT_DRV);
                }
            }

            if(	(EXT_DRV == slot_type[num]) &&
                    ((12 == num) || (13 == num)) )
            {
                RPT100S_fun[i].rpt_fun(rpt_type, rpt_data);
            }
            else
            {
                //保存槽位类型
                if(slot_type[num] != rpt_type)
                {
                    slot_type[num] = rpt_type;
                    /*send 插盘事件(针对输出盘)*/
                    send_online_framing(num, rpt_type);
                    init_pan((char)(num + 'a'));
                }

                if(RPT100S_fun[i].cmd_num[0] == 'C')
                {
                    if( event_filter(RPT100S_fun[i].cmd_num) )
                    {
                        //过滤不上报事件.
                    }
                    else
                    {
                        //if( is_change(num,rpt_data) )
                        //{
                        //有需要及时上报，组针上报
                        rpt_event_framing(RPT100S_fun[i].cmd_num, (char)(num + 'a'),
                                          1, rpt_data, 0, buf_auto);
                        //}
                        //else
                        //{
                        //;
                        //}
                    }
                }

                RPT100S_fun[i].rpt_fun((char)(num + 'a'), rpt_data);
            }

            break;
        }
    }

    return 0;
}


int almMsk_ExtRALM(u8_t ext_sid, unsigned char *rpt_alm,struct extctx *ctx)
{
    int i,j,len;
	int index = -1;
    unsigned char type_flag = 0;
	unsigned char num = 0;
	len = strlen(rpt_alm);
    if((ext_sid >= EXT_1_OUT_LOWER) && (ext_sid <= EXT_1_OUT_UPPER))
    {
       num = (ext_sid - EXT_1_OUT_OFFSET)%10;
       type_flag = ctx->extBid[0][num];
    }
	else if((ext_sid >= EXT_2_OUT_LOWER) && (ext_sid <= EXT_2_OUT_UPPER))
	{
	   num = (ext_sid - EXT_2_OUT_OFFSET)%10;
	   type_flag = ctx->extBid[1][num];
	}
	else if((ext_sid >= EXT_3_OUT_LOWER) && (ext_sid <= EXT_3_OUT_UPPER))
	{
	   num = (ext_sid < 58) ? (ext_sid - EXT_3_OUT_OFFSET):(ext_sid - EXT_3_OUT_OFFSET - 2);//except ':;'
	   type_flag = ctx->extBid[2][num];
	}

    else if(EXT_1_MGR_PRI == ext_sid)
    {
       type_flag = ctx->extBid[0][10];
    }
	else if(EXT_1_MGR_RSV == ext_sid)
    {
       type_flag = ctx->extBid[0][11];
    }	
	else if(EXT_2_MGR_PRI == ext_sid)
	{
	   type_flag = ctx->extBid[1][10];
	}
	else if(EXT_2_MGR_RSV == ext_sid)
	{
	   type_flag = ctx->extBid[1][11];
	}	
	else if(EXT_3_MGR_PRI == ext_sid)
	{
	   type_flag = ctx->extBid[2][10];
	}
	else if(EXT_3_MGR_RSV == ext_sid)	
	{
	   type_flag = ctx->extBid[2][11];
	}
	else
		return -1;
	for(j = 0; j < alm_msk_total; j++)
    {
        if(type_flag == conf_content.alm_msk[j].type)
        {
            index = j;
            //printf("slot_type[num] = %c\n",slot_type[num]);
            break;
        }
    }
	if(j == alm_msk_total)
    {
        //printf("slottype[%d]=%c  alm_msk no record\n",num,slot_type[num]);
        return -1;
    }
	if( (rpt_alm[0] >> 4) == 0x4 && -1 != index)
    {

        for(i = 0; i < len; i++)
        {

            rpt_alm[i] &= conf_content.alm_msk[index].data[i];

        }
    }
	
	return 0;
}



int almMsk_cmdRALM(int num, unsigned char *rpt_alm, int len)
{
    int i, j;
    int index = -1;
    if(num < 0)
    {
        printf("%s num is error\n", __FUNCTION__);
        return -1;
    }
    if('O' == slot_type[num])
    {
        return 0;
    }
    if(NULL == rpt_alm)
    {
        printf("rpt_alm is NULL\n");
        return -1;
    }
    //20140926 drv在线告警处理
    if( (12 == num || 13 == num)
            && ('d' == slot_type[num]) )
    {
        g_DrvLost = 200;//20141017
        rpt_alm[0] = rpt_alm[0] & gExtCtx.save.onlineSta;
        return 0;
    }
    else
    {
    }

    for(j = 0; j < alm_msk_total; j++)
    {
        if(slot_type[num] == conf_content.alm_msk[j].type)
        {
            index = j;
            //printf("slot_type[num] = %c\n",slot_type[num]);
            break;
        }
    }
    if(j == alm_msk_total)
    {
        //printf("slottype[%d]=%c  alm_msk no record\n",num,slot_type[num]);
        return -1;
    }
    if( (rpt_alm[0] >> 4) == 0x4 && -1 != index)
    {

        for(i = 0; i < len; i++)
        {

            rpt_alm[i] &= conf_content.alm_msk[index].data[i];

        }
    }
    else if((rpt_alm[0] >> 4) == 0x5 && -1 != index)
    {

        for(i = 0; i < len; i++)
        {

            rpt_alm[i] &= conf_content.alm_msk[index].data[i + 6];
        }
    }
    else
    {

    }
    return 0;
}

/*
RB是否锁定
未锁定，对MCP中的第0位第1位    置1
*/
void is_RB_unlocked(char *rpt_alm)
{
	unsigned char temp;
	
	if(rpt_alm == NULL)
	{
		return ;
	}

	temp = rpt_alm[0];
	if(slot_type[14] != 'O') //15槽位RB 盘
	{
		if(strncmp(rpt_content.slot_o.rb_trmode,"TRCK",4) == 0)
		{
			temp &= (~(1<<1));
		}
		else
		{
			temp |= (1<<1);
		}
	}

	if(slot_type[15] != 'O') //16槽位RB 盘
	{
		if(strncmp(rpt_content.slot_p.rb_trmode,"TRCK",4) == 0)
		{
			temp &= (~(1<<0));
		}
		else
		{
			temp |= (1<<0);
		}
	}
	
	rpt_alm[0] = temp;
}

//
// 改变上报的告警处理
//
int process_ralm(unsigned char *recv, int len)
{
    /*<slot><len>RALM:<slotid>:<alarm>;*/
    unsigned char  rpt_alm[7];
    unsigned char  savealm[14];
    unsigned char  ext_alm[32];
    int num;
    int flag_ref = 0;
    unsigned char  buf[4];

    memset(rpt_alm, '\0', 7);
    memset(savealm, '\0', 14);

#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:<from pan>:%s\n", NET_INFO, recv);
    }
#endif

    if(recv[7] >= 'a' && recv[7] <= 'w')
    {
        num = (int)(recv[7] - 'a');
        if((num <= 0) || (num > 23))
        {
            return 1;
        }
        if(slot_type[num] == 'O')
        {
            return 1;
        }
        memcpy(rpt_alm, &recv[9], len - 10);


        if(num == 14)
        {
            strncpynew(buf, rpt_content.slot_o.tod_buf, 3);
            send_tod(buf, 'o');
        }
        else if(num == 15)
        {
            strncpynew(buf, rpt_content.slot_p.tod_buf, 3);
            send_tod(buf, 'p');
        }
		else if(20 == num)
		{
			is_RB_unlocked(rpt_alm);
		}
        /*告警帧*/
        //printf("<%s>\n",rpt_alm);
        rpt_alm_to_client(recv[7], rpt_alm);

        /*保存告警*/
        /*"<aid>,<type>,<alarm>"*/
        if((rpt_alm[0] >> 4) == 0x05)
        {
            if(slot_type[num] == 'S')
            {
	            if((num != 1) || (num != 0))
	                return 1;
				else
					flag_ref = 1;
            }
			else if(slot_type[num] == 'j')
			{
			        flag_ref = 2;
			}
				
            
        }
        sprintf((char *)savealm, "\"%c,%c,%s\"", recv[7], slot_type[num], rpt_alm);
        save_alm(recv[7], savealm, flag_ref);

        //ext
        if( ('m' == recv[7]) && (EXT_DRV == slot_type[12]) )
        {
            gExtCtx.drv[0].drvAlm = recv[9];
        }
        if( ('n' == recv[7]) && (EXT_DRV == slot_type[13]) )
        {
            gExtCtx.drv[1].drvAlm = recv[9];
        }
        if('u' == recv[7])
        {
            if(recv[13]&BIT(2))
            {
                gExtCtx.extMcpDrvAlm[0] = 1;
            }
            else
            {
                gExtCtx.extMcpDrvAlm[0] = 0;
            }

            if(recv[13]&BIT(3))
            {
                gExtCtx.extMcpDrvAlm[1] = 1;
            }
            else
            {
                gExtCtx.extMcpDrvAlm[1] = 0;
            }
        }

        ext_drv2_mgr6_pr(&gExtCtx, slot_type[12], slot_type[13]);
        ext_drv_mgr_evt(&gExtCtx);
    }
    else
    {
        memset(ext_alm, 0, 32);
        memcpy(ext_alm, &recv[9], len - 10);
		almMsk_ExtRALM(recv[7], ext_alm,&gExtCtx);
        if(0 == ext_alm_cmp(recv[7], ext_alm, &gExtCtx))
        {
            return 1;
        }

        if(0 == ext_out_alm_crpt(recv[7], ext_alm, &gExtCtx))
        {
            return 1;
        }
    }

    return 0;
}



int process_revt(unsigned char *recv, int len)
{
    int i, j, i1, i2, i3, i4, ret;
    unsigned char  rpt_event[4];
    unsigned char  rpt_data[SENDBUFSIZE];
    unsigned char  rpt_reason[5];
    char  rpt_slot;
    char tmp_buf[64];
    char tp_1588_flag = 0;

    memset(rpt_event, '\0', 4);
    memset(rpt_data, '\0', SENDBUFSIZE);
    memset(rpt_reason, '\0', 5);

    j = 0;
    i1 = 0;
    i2 = 0;
    i3 = 0;
    i4 = 0;
    for(i = 2; i < len; i++)
    {
        if(recv[i] == ':')
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
    rpt_slot = recv[i1];
    memcpy(rpt_event, &recv[i2], 3);
    memcpy(rpt_data, &recv[i3], i4 - i3 - 1);
    memcpy(rpt_reason, &recv[i4], len - i4 - 1);
    if((strncmp1(rpt_reason, "LMC", 3) != 0) && (strncmp1(rpt_reason, "AUTO", 4) != 0))
    {
        return 1;
    }
    //modified by zhanghui 2013-9-11 15:52:38
    if('I' == slot_type[recv[0] - 1])
    {
        //printf("revt recv :%s\n",recv);
        //printf("revt :%s\n",rpt_data);
        memset(tmp_buf, 0, 64);
        memcpy(tmp_buf, rpt_data, strlen(rpt_data));
        tp_1588_flag = 1;

        //pps state switch
        if('0' == tmp_buf[0])
        {
            tmp_buf[0] = '1';
        }
        else if('1' == tmp_buf[0])
        {
            tmp_buf[0] = '2';
        }
        else if('2' == tmp_buf[0])
        {
            tmp_buf[0] = '3';
        }
        else if('5' == tmp_buf[0])
        {
            tmp_buf[0] = '4';
        }
        else
        {
            tmp_buf[0] = '3';
        }

        //source type switch
        if('2' == tmp_buf[2])
        {
            tmp_buf[2] = '3';
        }
        else if('3' == tmp_buf[2])
        {
            tmp_buf[2] = '2';
        }
        else
        {
            //do nothing
        }
    }

    for(i = 0; i < NUM_REVT; i++)
    {
        if(strncmp1(REVT_fun[i].cmd_num, rpt_event, 3) == 0)
        {
            //保存信息
            ret = REVT_fun[i].rpt_fun(rpt_slot, rpt_data);
            if(( 0 == ret ) && ( !event_filter_another(REVT_fun[i].cmd_num )))
            {
                if(1 == tp_1588_flag)
                {

                    //printf("<process_revt>:%s\n",tmp_buf);
                    rpt_event_framing(rpt_event, (char)(recv[0] + 'a' - 1),
                                      1, tmp_buf, 0, rpt_reason);
                }
                else
                {
                    //组针上报
                    //rpt_event_framing(rpt_event, (char)(recv[0] + 'a' - 1),
                    //                  1, rpt_data, 0, rpt_reason);
                    rpt_event_framing(rpt_event, (char)rpt_slot,
                                      1, rpt_data, 0, rpt_reason);
                }
            }
            break;
        }
    }

    return 0;

}



int cmd_process(int client, _MSG_NODE *msg)
{
    int i, ret;

    for(i = 0; i < _CMD_NODE_MAX; i++)
    {
        if(strncmp1(g_cmd_fun[i].cmd, msg->cmd, g_cmd_fun[i].cmd_len) == 0)
        {
            current_client = client;
            if(g_cmd_fun[i].cmd_fun != NULL)
            {
                ret = g_cmd_fun[i].cmd_fun(msg, 1);
                if(ret == 0)
                {
                    //返回成功响应
                    respond_success(client, msg->ctag);
                    return 0;
                }
                else
                {
                    //返回错误响应
                    //printf("ret=%d\n",ret);
                    respond_fail(client, msg->ctag, 7);
                    return 0;
                }
            }
            else
            {
                ret = g_cmd_fun[i].rpt_fun(client, msg);
                //printf("RTRV\n");
                if(ret == 0)
                {
                    respond_success(client, msg->ctag);
                }
                else
                {
                    respond_fail(client, msg->ctag, 8);
                }
                return 0;
            }

        }
    }

    respond_fail(client, msg->ctag, 3);
    return 0;
}
int get_fpga_ver(unsigned char *data)
{
    memcpy(fpga_ver, &data[5], 6);
    return 0;
}

//12#drv在在没插入mge时可能不会上报告警信息，特此检查12#扩展框是否没插mge
void CheckExtLost()
{
    if(slot_type[12] != 'd')
    {
        g_DrvLost = 200;
        return;
    }
    else
    {
        if(g_DrvLost <= 0 )
        {
            g_DrvLost = 200;
            alm_sta[12][5] = 0x4f;
            alm_sta[12][5] = alm_sta[12][5] & gExtCtx.save.onlineSta;
        }
        else
        {
            g_DrvLost--;
        }
    }
}


void autoLeap(void)
{
    static int count = 0;
    _MSG_NODE *msg = NULL;

    extern int cmd_set_leap_num(_MSG_NODE * msg, int flag);
    if(g_LeapFlag != 1)
    {
        return ;
    }
    else
    {
        count++;
    }

    if(count >= 60)
    {
        msg = (_MSG_NODE *)malloc(sizeof(_MSG_NODE));
		if(NULL == msg)
        {
            printf("autoLeap Malloc is failed\n");
            return;
        }
        memset(msg, 0, sizeof(_MSG_NODE));
        sprintf((char *)msg->cmd, "SET-LEAP-NUM:");
        msg->cmd_len = 13;
        msg->solt = 'u';
        memcpy(msg->ctag, "R00000", 6);
        memcpy(msg->data, conf_content.slot_u.leap_num, 2);
        cmd_set_leap_num(msg, 1);
        cmd_set_leap_num(msg, 1);
        cmd_set_leap_num(msg, 1);

        count = 0;
        g_LeapFlag = 0;
        free(msg);

    }
    else
    {
        ;
    }
}

int get_time(unsigned char *data) /*来自RB的时间*/
{
    /*<TIME YYYY-MM-DD HH:MM:DD;>*/
    int len;
    unsigned char  year_rb[5];
    int year;
    len = strlen(data);

    //20141017 判断mge是否全丢失
    CheckExtLost();

	//20141210自动设置闰秒
    if(1 == g_LeapFlag)
    {
        autoLeap();
    }
    else
    {
        ;
    }

    if(len == 25)
    {
        if((data[9] == '-') && (data[12] == '-') && (data[18] == ':') && (data[21] == ':'))
        {

            memcpy(year_rb, &data[5], 4);
            year = atoi(year_rb);
            if(year < 2011)
            {
                return 1;    /*如果从FPGA时间未获得，使用默认时间*/
            }

            memcpy(mcp_date, &data[5], 10);
            memcpy(mcp_time, &data[16], 8);

        }
    }
    else
    {
#if DEBUG_NET_INFO
        if(print_switch == 0)
        {
            printf("%s:error_time!\n", NET_INFO);
            printf("%s:<%s>\n", NET_INFO, data);
        }
#endif
    }
    return 0;
}

void get_time_fromnet()/*根据网管的配置确定时间*/
{
    unsigned char year_rb[5];
    unsigned char month_rb[3];
    unsigned char day_rb[3];
    unsigned char hour_rb[3];
    unsigned char minute_rb[3];
    unsigned char second_rb[3];
    int year, month, day, hour, minute, second;
    int tmp_hour, tmp_minute, tmp_second;

    if((conf_content.slot_u.time_source == '1') && (time_temp != 0))
    {
        memcpy(year_rb, mcp_date, 4);
        memcpy(month_rb, &mcp_date[5], 2);
        memcpy(day_rb, &mcp_date[8], 2);

        memcpy(hour_rb, mcp_time, 2);
        memcpy(minute_rb, &mcp_time[3], 2);
        memcpy(second_rb, &mcp_time[6], 2);


        year = atoi(year_rb);
        month = atoi(month_rb);
        day = atoi(day_rb);

        hour = atoi(hour_rb);
        minute = atoi(minute_rb);
        second = atoi(second_rb);


        if(time_temp < 0)
        {
            tmp_hour = abs(time_temp) / 3600;
            tmp_minute = (abs(time_temp) % 3600) / 60;
            tmp_second = abs(time_temp) % 60;

            second -= tmp_second;
            minute -= tmp_minute;
            hour -= tmp_hour;

            if(second < 0)
            {
                second += 60;
                minute--;
            }
            if(minute < 0)
            {
                minute += 60;
                hour--;
            }
            if(hour < 0)
            {
                hour += 24;
                day--;
            }
            if(day == 0)
            {
                if(month == 1)
                {
                    day = 31;
                    month = 12;
                    year--;
                }
                else if((month == 2) || (month == 4) || (month == 6) || (month == 8) || (month == 9) || (month == 11))
                {
                    day = 31;
                    month--;
                }
                else if((month == 5) || (month == 7) || (month == 10) || (month == 12))
                {
                    day = 30;
                    month--;
                }
                else if(month == 3)
                {
                    if(IsLeapYear(year))
                    {
                        day = 29;
                    }
                    else
                    {
                        day = 28;
                    }
                    month--;
                }
            }
        }
        else
        {
            tmp_hour = time_temp / 3600;
            tmp_minute = (time_temp % 3600) / 60;
            tmp_second = time_temp % 60;

            second += tmp_second;
            minute += tmp_minute;
            hour += tmp_hour;
            if(second > 59)
            {
                second = second % 60;
                minute++;
            }
            if(minute > 59)
            {
                minute = minute % 60;
                hour++;
            }
            if(hour > 23)
            {
                hour = hour % 24;
                day++;
            }
            if((day == 29) && (month == 2) && (IsLeapYear(year) != 1))
            {
                day = 1;
                month++;
            }
            else if((day == 30) && (month == 2) && (IsLeapYear(year) == 1))
            {
                day = 1;
                month++;
            }
            else if(day == 31)
            {
                if((month == 4) || (month == 6) || (month == 9) || (month == 11))
                {
                    day = 1;
                    month++;
                }
            }
            else if(day == 32)
            {
                if((month == 1) || (month == 3) || (month == 5) || (month == 7) || (month == 8) || (month == 10))
                {
                    day = 1;
                    month++;
                }
                else if(month == 12)
                {
                    day = 1;
                    month = 1;
                    year++;
                }
            }
        }

        /*<TIME YYYY-MM-DD HH:MM:DD;>*/
        memset(mcp_date, '\0', 11);
        memset(mcp_time, '\0', 9);
        sprintf((char *)mcp_date, "%04d-%02d-%02d", year, month, day);
        sprintf((char *)mcp_time, "%02d:%02d:%02d", hour, minute, second);
    }
}




int IsLeapYear(int year)
{
    int LeapYear;
    LeapYear = (((year % 4) == 0) && (year % 100 != 0)) || (year % 400 == 0);
    return LeapYear;
}


void get_data(char slot, unsigned char *data)
{
    int i, num;
    num = (int)(slot - 'a');
    sprintf((char *)data, "%s%c%c%c\"", data, SP, SP, SP);
	//
	//!
	//
    if((slot == ':') || (slot == ' '))
    {
        for(i = 0; i < 22; i++)
        {
            sprintf((char *)data, "%s%c,%c", data, (char)(0x61 + i), slot_type[i]);
            if((slot_type[i] != 'O') && (i < 18))
            {
                if(i < 14)
                {
                    if((EXT_DRV == slot_type[i]))
                    {
                        sprintf((char *)data, "%s,%s,,",
                                data,
                                ((0 == strlen(gExtCtx.drv[i - 12].drvVer)) ? (char *)"," : (char *)(gExtCtx.drv[i - 12].drvVer)));
                    }
                    else
                    {
                        sprintf((char *)data, "%s,%s", data,
                                (strlen(rpt_content.slot[i].sy_ver) == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot[i].sy_ver));
                    }
                }
                else if(i == 14)
                    sprintf((char *)data, "%s,%s", data,
                            (strlen(rpt_content.slot_o.sy_ver) == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot_o.sy_ver));
                else if(i == 15)
                    sprintf((char *)data, "%s,%s", data,
                            (strlen(rpt_content.slot_p.sy_ver) == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot_p.sy_ver));
                else if(i == 16)
                    sprintf((char *)data, "%s,%s", data,
                            (strlen(rpt_content.slot_q.sy_ver) == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot_q.sy_ver));
                else if(i == 17)
                    sprintf((char *)data, "%s,%s", data,
                            (strlen(rpt_content.slot_r.sy_ver) == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot_r.sy_ver));
            }
			else if((i == 18 )||(i == 19 ))
			{
			    if(slot_type[i] != 'O')
			    	sprintf((char *)data, "%s,,,,", data);
				else
					sprintf((char *)data, "%s", data);
			}
            else if(i == 20)
            {
                sprintf((char *)data, "%s,%s,%s,,", data, (unsigned char *)MCP_VER, fpga_ver);
            }

            sprintf((char *)data, "%s:", data);
        }
		if(slot_type[i] != 'O')
            sprintf((char *)data, "%sw,%c,,,,", data, slot_type[22]);
		else
			sprintf((char *)data, "%sw,%c", data, slot_type[22]);
    }
    else if((slot >= 'a') && (slot < 'o'))
    {
        if(slot_type[num] == 'Y')
        {
            rpt_test(slot, data);
        }
        else if('d' == slot_type[num])
        {
            rpt_drv(slot, data);
        }
        else
        {
            rpt_out(slot, data);
        }
    }
    else if((slot == 'o') || (slot == 'p'))
    {
        if(slot_type[num] == 'Y')
        {
            rpt_test(slot, data);
        }
        else
        {
            rpt_rb(slot, data);
        }
    }
    else if((slot == 'q') || (slot == 'r'))
    {
        if(slot_type[num] == 'Y')
        {
            rpt_test(slot, data);
        }
        else
        {
            rpt_gbtp(slot, data);
        }
    }
    else if((slot == 's') || (slot == 't'))
    {
        if(slot_type[num] != 'O')
        	sprintf((char *)data, "%s%c,%c,,,,", data, slot, slot_type[num]);
		else
			sprintf((char *)data, "%s%c,%c", data, slot, slot_type[num]);
    }
    else if(slot == 'u')
    {
        rpt_mcp(data);
    }
    else if(slot == 'v')
    {
        /*NULL*/
    }
    else if(slot == 'w')
    {
        if(slot_type[num] != 'O')
            sprintf((char *)data, "%s%c,%c,,,,", data, slot, slot_type[num]);
		else
			sprintf((char *)data, "%s%c,%c", data, slot, slot_type[num]);
    }

    sprintf((char *)data, "%s\"%c%c;", data, CR, LF);
}




void get_ext1_eqt(char slot, unsigned char *data)
{
    int i, num;

    num = (int)(slot - 'a');
    sprintf((char *)data, "%s%c%c%c\"", data, SP, SP, SP);
    if((slot == ':') || (slot == ' '))
    {
        for(i = 0; i < 10; i++)
        {
            sprintf((char *)data, "%s%c,%c", data, (char)('a' + i), gExtCtx.extBid[0][i]);
            if(gExtCtx.extBid[0][i] != 'O')
            {
                sprintf((char *)data, "%s,%s,,",
                        data,
                        ((0 == strlen(gExtCtx.out[i].outVer)) ? (char *)(",") : (char *)(gExtCtx.out[i].outVer)));
                sprintf((char *)data, "%s,%s",
                        data,
                        ((0 == strlen(gExtCtx.out[i].outPR)) ? (char *)(",") : (char *)(gExtCtx.out[i].outPR)));


            }

            strcat(data, ":");
        }

        sprintf((char *)data, "%s%c,%c", data, 'k', gExtCtx.extBid[0][10]);
        if(gExtCtx.extBid[0][10] != 'O')
        {
            sprintf((char *)data, "%s,%s,,",
                    data,
                    ((0 == strlen(gExtCtx.mgr[0].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[0].mgrVer)));
        }
        strcat(data, ":");

        sprintf((char *)data, "%s%c,%c", data, 'l', gExtCtx.extBid[0][11]);
        if(gExtCtx.extBid[0][11] != 'O')
        {
            sprintf((char *)data, "%s,%s,,",
                    data,
                    ((0 == strlen(gExtCtx.mgr[1].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[1].mgrVer)));
        }
        strcat(data, ":");
		
        if(gExtCtx.extBid[0][12] == 'O')
        	sprintf((char *)data, "%s%c,%c", data, 'm', gExtCtx.extBid[0][12]);
		else
			sprintf((char *)data, "%s%c,%c,,,,", data, 'm', gExtCtx.extBid[0][12]);
        strcat(data, ":");
		if(gExtCtx.extBid[0][13] == 'O')
        	sprintf((char *)data, "%s%c,%c", data, 'n', gExtCtx.extBid[0][13]);
		else
			sprintf((char *)data, "%s%c,%c,,,,", data, 'n', gExtCtx.extBid[0][13]);
    }
    else
    {
        if(slot >= 'a' && slot <= 'j')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[0][slot - 'a']);
            if(gExtCtx.extBid[0][slot - 'a'] != 'O')
            {
                sprintf((char *)data, "%s,%s,,",
                        data,
                        ((0 == strlen(gExtCtx.out[slot - 'a'].outVer)) ? (char *)(",") : (char *)(gExtCtx.out[slot - 'a'].outVer)));
                sprintf((char *)data, "%s,%s,%s",
                        data,
                        gExtCtx.out[slot - 'a'].outSsm,
                        gExtCtx.out[slot - 'a'].outSignal);
                sprintf((char *)data, "%s,%s",
                        data,
                        ((0 == strlen(gExtCtx.out[slot - 'a'].outPR)) ? (char *)(",") : (char *)(gExtCtx.out[slot - 'a'].outPR)));
            }
        }
        else if(slot == 'k' || slot == 'l')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[0][slot - 'a']);
            if(gExtCtx.extBid[0][slot - 'a'] != 'O')
            {
                sprintf((char *)data, "%s,%s,,,%d",
                        data,
                        ((0 == strlen(gExtCtx.mgr[slot - 'k'].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[slot - 'k'].mgrVer)),
                        gExtCtx.mgr[slot - 'k'].mgrPR);
            }
        }
        else if(slot == 'm' || slot == 'n')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[0][slot - 'a']);
        }
        else
        {

        }
    }

    sprintf((char *)data, "%s\"%c%c;", data, CR, LF);
}

void get_exta1_eqt(char slot, unsigned char *data)
{
    int i, num;

    num = (int)(slot - 'a');
    sprintf((char *)data, "%s%c%c%c\"", data, SP, SP, SP);
    if((slot == ':') || (slot == ' '))
    {
        for(i = 0; i < 10; i++)
        {
            sprintf((char *)data, "%s%c,%c", data, (char)('a' + i), gExtCtx.extBid[0][i]);
            if(gExtCtx.extBid[0][i] != 'O')
            {
                sprintf((char *)data, "%s,%s,,",
                        data,
                        ((0 == strlen(gExtCtx.out[i].outVer)) ? (char *)(",") : (char *)(gExtCtx.out[i].outVer)));
                sprintf((char *)data, "%s,%s",
                        data,
                        ((0 == strlen(gExtCtx.out[i].outPR)) ? (char *)(",") : (char *)(gExtCtx.out[i].outPR)));


            }

            strcat(data, ":");
        }

        sprintf((char *)data, "%s%c,%c", data, 'k', gExtCtx.extBid[0][10]);
        if(gExtCtx.extBid[0][10] != 'O')
        {
            sprintf((char *)data, "%s,%s,,",
                    data,
                    ((0 == strlen(gExtCtx.mgr[0].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[0].mgrVer)));
        }
        strcat(data, ":");

        sprintf((char *)data, "%s%c,%c", data, 'l', gExtCtx.extBid[0][11]);
        if(gExtCtx.extBid[0][11] != 'O')
        {
            sprintf((char *)data, "%s,%s,,",
                    data,
                    ((0 == strlen(gExtCtx.mgr[1].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[1].mgrVer)));
        }
        strcat(data, ":");
        if(gExtCtx.extBid[0][12] == 'O')
        	sprintf((char *)data, "%s%c,%c", data, 'm', gExtCtx.extBid[0][12]);
		else
			sprintf((char *)data, "%s%c,%c,,,,", data, 'm', gExtCtx.extBid[0][12]);
        strcat(data, ":");

		if(gExtCtx.extBid[0][13] == 'O')
        	sprintf((char *)data, "%s%c,%c", data, 'n', gExtCtx.extBid[0][13]);
		else
			sprintf((char *)data, "%s%c,%c,,,,", data, 'n', gExtCtx.extBid[0][13]);
    }
    else
    {
        if(slot >= 'a' && slot <= 'j')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[0][slot - 'a']);
            if(gExtCtx.extBid[0][slot - 'a'] != 'O')
            {
                sprintf((char *)data, "%s,%s,,",
                        data,
                        ((0 == strlen(gExtCtx.out[slot - 'a'].outVer)) ? (char *)(",") : (char *)(gExtCtx.out[slot - 'a'].outVer)));
                sprintf((char *)data, "%s,%s,%s",
                        data,
                        gExtCtx.out[slot - 'a'].outSsm,
                        gExtCtx.out[slot - 'a'].outSignal);
                sprintf((char *)data, "%s,%s",
                        data,
                        ((0 == strlen(gExtCtx.out[slot - 'a'].outPR)) ? (char *)(",") : (char *)(gExtCtx.out[slot - 'a'].outPR)));
            }
        }
        else if(slot == 'k' || slot == 'l')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[0][slot - 'a']);
            if(gExtCtx.extBid[0][slot - 'a'] != 'O')
            {
                sprintf((char *)data, "%s,%s,,,%d",
                        data,
                        ((0 == strlen(gExtCtx.mgr[slot - 'k'].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[slot - 'k'].mgrVer)),
                        gExtCtx.mgr[slot - 'k'].mgrPR);
            }
        }
        else if(slot == 'm' || slot == 'n')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[0][slot - 'a']);
        }
        else
        {

        }
    }

    sprintf((char *)data, "%s\"%c%c/%c%c", data, CR, LF, CR, LF);
}






void get_ext2_eqt(char slot, unsigned char *data)
{
    int i, num;

    num = (int)(slot - 'a');
    sprintf((char *)data, "%s%c%c%c\"", data, SP, SP, SP);
    if((slot == ':') || (slot == ' '))
    {
        for(i = 0; i < 10; i++)
        {
            sprintf((char *)data, "%s%c,%c", data, (char)('a' + i), gExtCtx.extBid[1][i]);
            if(gExtCtx.extBid[1][i] != 'O')
            {
                sprintf((char *)data, "%s,%s,,",
                        data,
                        ((0 == strlen(gExtCtx.out[10 + i].outVer)) ? (char *)(",") : (char *)(gExtCtx.out[10 + i].outVer)));
                sprintf((char *)data, "%s,%s",
                        data,
                        ((0 == strlen(gExtCtx.out[10 + i].outPR)) ? (char *)(",") : (char *)(gExtCtx.out[10 + i].outPR)));
            }

            strcat(data, ":");
        }

        sprintf((char *)data, "%s%c,%c", data, 'k', gExtCtx.extBid[1][10]);
        if(gExtCtx.extBid[1][10] != 'O')
        {
            sprintf((char *)data, "%s,%s,,",
                    data,
                    ((0 == strlen(gExtCtx.mgr[2].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[2].mgrVer)));
        }
        strcat(data, ":");

        sprintf((char *)data, "%s%c,%c", data, 'l', gExtCtx.extBid[1][11]);
        if(gExtCtx.extBid[1][11] != 'O')
        {
            sprintf((char *)data, "%s,%s,,",
                    data,
                    ((0 == strlen(gExtCtx.mgr[3].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[3].mgrVer)));
        }
        strcat(data, ":");
        if(gExtCtx.extBid[1][12] == 'O')
        	sprintf((char *)data, "%s%c,%c", data, 'm', gExtCtx.extBid[1][12]);
		else
			sprintf((char *)data, "%s%c,%c,,,,", data, 'm', gExtCtx.extBid[1][12]);
		
        strcat(data, ":");
		if(gExtCtx.extBid[1][13] == 'O')
        	sprintf((char *)data, "%s%c,%c", data, 'n', gExtCtx.extBid[1][13]);
		else
			sprintf((char *)data, "%s%c,%c,,,,", data, 'n', gExtCtx.extBid[1][13]);
    }
    else
    {
        if(slot >= 'a' && slot <= 'j')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[1][slot - 'a']);
            if(gExtCtx.extBid[1][slot - 'a'] != 'O')
            {
                sprintf((char *)data, "%s,%s,,",
                        data,
                        ((0 == strlen(gExtCtx.out[10 + slot - 'a'].outVer)) ? (char *)(",") : (char *)(gExtCtx.out[10 + slot - 'a'].outVer)));
                sprintf((char *)data, "%s,%s,%s",
                        data,
                        gExtCtx.out[10 + slot - 'a'].outSsm,
                        gExtCtx.out[10 + slot - 'a'].outSignal);
                sprintf((char *)data, "%s,%s",
                        data,
                        ((0 == strlen(gExtCtx.out[10 + slot - 'a'].outPR)) ? (char *)(",") : (char *)(gExtCtx.out[10 + slot - 'a'].outPR)));
            }
        }
        else if(slot == 'k' || slot == 'l')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[1][slot - 'a']);
            if(gExtCtx.extBid[1][slot - 'a'] != 'O')
            {
                sprintf((char *)data, "%s,%s,,,%d",
                        data,
                        ((0 == strlen(gExtCtx.mgr[2 + slot - 'k'].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[2 + slot - 'k'].mgrVer)),
                        gExtCtx.mgr[2 + slot - 'k'].mgrPR);
            }
        }
        else if(slot == 'm' || slot == 'n')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[1][slot - 'a']);
        }
        else
        {

        }
    }

    sprintf((char *)data, "%s\"%c%c;", data, CR, LF);
}



void get_exta2_eqt(char slot, unsigned char *data)
{
    int i, num;
    num = (int)(slot - 'a');
    sprintf((char *)data, "%s%c%c%c\"", data, SP, SP, SP);
    if((slot == ':') || (slot == ' '))
    {
        for(i = 0; i < 10; i++)
        {
            sprintf((char *)data, "%s%c,%c", data, (char)('a' + i), gExtCtx.extBid[1][i]);
            if(gExtCtx.extBid[1][i] != 'O')
            {
                sprintf((char *)data, "%s,%s,,",
                        data,
                        ((0 == strlen(gExtCtx.out[10 + i].outVer)) ? (char *)(",") : (char *)(gExtCtx.out[10 + i].outVer)));
                sprintf((char *)data, "%s,%s",
                        data,
                        ((0 == strlen(gExtCtx.out[10 + i].outPR)) ? (char *)(",") : (char *)(gExtCtx.out[10 + i].outPR)));
            }

            strcat(data, ":");
        }

        sprintf((char *)data, "%s%c,%c", data, 'k', gExtCtx.extBid[1][10]);
        if(gExtCtx.extBid[1][10] != 'O')
        {
            sprintf((char *)data, "%s,%s,,",
                    data,
                    ((0 == strlen(gExtCtx.mgr[2].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[2].mgrVer)));
        }
        strcat(data, ":");

        sprintf((char *)data, "%s%c,%c", data, 'l', gExtCtx.extBid[1][11]);
        if(gExtCtx.extBid[1][11] != 'O')
        {
            sprintf((char *)data, "%s,%s,,",
                    data,
                    ((0 == strlen(gExtCtx.mgr[3].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[3].mgrVer)));
        }
        strcat(data, ":");
		
        if(gExtCtx.extBid[1][12] == 'O')
        	sprintf((char *)data, "%s%c,%c", data, 'm', gExtCtx.extBid[1][12]);
		else
			sprintf((char *)data, "%s%c,%c,,,,", data, 'm', gExtCtx.extBid[1][12]);
		
        strcat(data, ":");
		if(gExtCtx.extBid[1][13] == 'O')
        	sprintf((char *)data, "%s%c,%c", data, 'n', gExtCtx.extBid[1][13]);
		else
			sprintf((char *)data, "%s%c,%c,,,,", data, 'n', gExtCtx.extBid[1][13]);
    }
    else
    {
        if(slot >= 'a' && slot <= 'j')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[1][slot - 'a']);
            if(gExtCtx.extBid[1][slot - 'a'] != 'O')
            {
                sprintf((char *)data, "%s,%s,,",
                        data,
                        ((0 == strlen(gExtCtx.out[10 + slot - 'a'].outVer)) ? (char *)(",") : (char *)(gExtCtx.out[10 + slot - 'a'].outVer)));
                sprintf((char *)data, "%s,%s,%s",
                        data,
                        gExtCtx.out[10 + slot - 'a'].outSsm,
                        gExtCtx.out[10 + slot - 'a'].outSignal);
                sprintf((char *)data, "%s,%s",
                        data,
                        ((0 == strlen(gExtCtx.out[10 + slot - 'a'].outPR)) ? (char *)(",") : (char *)(gExtCtx.out[10 + slot - 'a'].outPR)));
            }
        }
        else if(slot == 'k' || slot == 'l')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[1][slot - 'a']);
            if(gExtCtx.extBid[1][slot - 'a'] != 'O')
            {
                sprintf((char *)data, "%s,%s,,,%d",
                        data,
                        ((0 == strlen(gExtCtx.mgr[2 + slot - 'k'].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[2 + slot - 'k'].mgrVer)),
                        gExtCtx.mgr[2 + slot - 'k'].mgrPR);
            }
        }
        else if(slot == 'm' || slot == 'n')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[1][slot - 'a']);
        }
        else
        {

        }
    }

    sprintf((char *)data, "%s\"%c%c/%c%c", data, CR, LF, CR, LF);
}




void get_ext3_eqt(char slot, unsigned char *data)
{
    int i, num;

    num = (int)(slot - 'a');
	sprintf((char *)data, "%s%c%c%c\"", data, SP, SP, SP);
		
    if((slot == ':') || (slot == ' '))
    {
        for(i = 0; i < 10; i++)
        {
            //
            //! 盘类型
            //
            sprintf((char *)data, "%s%c,%c", data, (char)('a' + i), gExtCtx.extBid[2][i]);
            //
            //! 盘在线，上报
            //
			if(gExtCtx.extBid[2][i] != 'O')
            {
                //
                //!版本信息
                //
                sprintf((char *)data, "%s,%s,,",
                        data,
                        ((0 == strlen(gExtCtx.out[20 + i].outVer)) ? (char *)(",") : (char *)(gExtCtx.out[20 + i].outVer)));
                //
                //!模式和输出状态
                //
				sprintf((char *)data, "%s,%s",
                        data,
                        ((0 == strlen(gExtCtx.out[20 + i].outPR)) ? (char *)(",") : (char *)(gExtCtx.out[20 + i].outPR)));
            }

            strcat(data, ":");
        }

		//
		//!MGE单盘在线状态和版本
		//
        sprintf((char *)data, "%s%c,%c", data, 'k', gExtCtx.extBid[2][10]);
        if(gExtCtx.extBid[2][10] != 'O')
        {
            sprintf((char *)data, "%s,%s,,",
                    data,
                    ((0 == strlen(gExtCtx.mgr[4].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[4].mgrVer)));
        }
        strcat(data, ":");

        sprintf((char *)data, "%s%c,%c", data, 'l', gExtCtx.extBid[2][11]);
        if(gExtCtx.extBid[2][11] != 'O')
        {
            sprintf((char *)data, "%s,%s,,",
                    data,
                    ((0 == strlen(gExtCtx.mgr[5].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[5].mgrVer)));
        }
        strcat(data, ":");
        //
        //!电源单盘在线状态
        //
        if(gExtCtx.extBid[2][12] == 'O')
        	sprintf((char *)data, "%s%c,%c", data, 'm', gExtCtx.extBid[2][12]);
		else
			sprintf((char *)data, "%s%c,%c,,,,", data, 'm', gExtCtx.extBid[2][12]);
		
        strcat(data, ":");

		if(gExtCtx.extBid[2][13] == 'O')
        	sprintf((char *)data, "%s%c,%c", data, 'n', gExtCtx.extBid[2][13]);
		else
			sprintf((char *)data, "%s%c,%c,,,,", data, 'n', gExtCtx.extBid[2][13]);
    }
    else
    {
        //
        //!slot 
        //
        if(slot >= 'a' && slot <= 'j')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[2][slot - 'a']);
            if(gExtCtx.extBid[2][slot - 'a'] != 'O')
            {
                sprintf((char *)data, "%s,%s,,",
                        data,
                        ((0 == strlen(gExtCtx.out[20 + slot - 'a'].outVer)) ? (char *)(",") : (char *)(gExtCtx.out[20 + slot - 'a'].outVer)));
                sprintf((char *)data, "%s,%s,%s",
                        data,
                        gExtCtx.out[20 + slot - 'a'].outSsm,
                        gExtCtx.out[20 + slot - 'a'].outSignal);
                sprintf((char *)data, "%s,%s",
                        data,
                        ((0 == strlen(gExtCtx.out[20 + slot - 'a'].outPR)) ? (char *)(",") : (char *)(gExtCtx.out[20 + slot - 'a'].outPR)));
            }
        }
        else if(slot == 'k' || slot == 'l')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[2][slot - 'a']);
            if(gExtCtx.extBid[2][slot - 'a'] != 'O')
            {
                sprintf((char *)data, "%s,%s,,,%d",
                        data,
                        ((0 == strlen(gExtCtx.mgr[4 + slot - 'k'].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[4 + slot - 'k'].mgrVer)),
                        gExtCtx.mgr[4 + slot - 'k'].mgrPR);
            }
        }
        else if(slot == 'm' || slot == 'n')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[2][slot - 'a']);
        }
        else
        {

        }
    }

    sprintf((char *)data, "%s\"%c%c;", data, CR, LF);
}

void get_exta3_eqt(char slot, unsigned char *data)
{
    int i, num;

    num = (int)(slot - 'a');
    sprintf((char *)data, "%s%c%c%c\"", data, SP, SP, SP);
    if((slot == ':') || (slot == ' '))
    {
        for(i = 0; i < 10; i++)
        {
            //
            //! 盘类型
            //
            sprintf((char *)data, "%s%c,%c", data, (char)('a' + i), gExtCtx.extBid[2][i]);
            //
            //! 盘在线，上报
            //
			if(gExtCtx.extBid[2][i] != 'O')
            {
                //
                //!版本信息
                //
                sprintf((char *)data, "%s,%s,,",
                        data,
                        ((0 == strlen(gExtCtx.out[20 + i].outVer)) ? (char *)(",") : (char *)(gExtCtx.out[20 + i].outVer)));
                //
                //!模式和输出状态
                //
				sprintf((char *)data, "%s,%s",
                        data,
                        ((0 == strlen(gExtCtx.out[20 + i].outPR)) ? (char *)(",") : (char *)(gExtCtx.out[20 + i].outPR)));
            }

            strcat(data, ":");
        }

		//
		//!MGE单盘在线状态和版本
		//
        sprintf((char *)data, "%s%c,%c", data, 'k', gExtCtx.extBid[2][10]);
        if(gExtCtx.extBid[2][10] != 'O')
        {
            sprintf((char *)data, "%s,%s,,",
                    data,
                    ((0 == strlen(gExtCtx.mgr[4].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[4].mgrVer)));
        }
        strcat(data, ":");

        sprintf((char *)data, "%s%c,%c", data, 'l', gExtCtx.extBid[2][11]);
        if(gExtCtx.extBid[2][11] != 'O')
        {
            sprintf((char *)data, "%s,%s,,",
                    data,
                    ((0 == strlen(gExtCtx.mgr[5].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[5].mgrVer)));
        }
        strcat(data, ":");
        //
        //!电源单盘在线状态
        //
        if(gExtCtx.extBid[2][12] == 'O')
        	sprintf((char *)data, "%s%c,%c", data, 'm', gExtCtx.extBid[2][12]);
		else
			sprintf((char *)data, "%s%c,%c,,,,", data, 'm', gExtCtx.extBid[2][12]);
        strcat(data, ":");
		if(gExtCtx.extBid[2][13] == 'O')
        	sprintf((char *)data, "%s%c,%c", data, 'n', gExtCtx.extBid[2][13]);
		else
			sprintf((char *)data, "%s%c,%c,,,,", data, 'n', gExtCtx.extBid[2][13]);
    }
    else
    {
        //
        //!slot 
        //
        if(slot >= 'a' && slot <= 'j')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[2][slot - 'a']);
            if(gExtCtx.extBid[2][slot - 'a'] != 'O')
            {
                sprintf((char *)data, "%s,%s,,",
                        data,
                        ((0 == strlen(gExtCtx.out[20 + slot - 'a'].outVer)) ? (char *)(",") : (char *)(gExtCtx.out[20 + slot - 'a'].outVer)));
                sprintf((char *)data, "%s,%s,%s",
                        data,
                        gExtCtx.out[20 + slot - 'a'].outSsm,
                        gExtCtx.out[20 + slot - 'a'].outSignal);
                sprintf((char *)data, "%s,%s",
                        data,
                        ((0 == strlen(gExtCtx.out[20 + slot - 'a'].outPR)) ? (char *)(",") : (char *)(gExtCtx.out[20 + slot - 'a'].outPR)));
            }
        }
        else if(slot == 'k' || slot == 'l')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[2][slot - 'a']);
            if(gExtCtx.extBid[2][slot - 'a'] != 'O')
            {
                sprintf((char *)data, "%s,%s,,,%d",
                        data,
                        ((0 == strlen(gExtCtx.mgr[4 + slot - 'k'].mgrVer)) ? (char *)(",") : (char *)(gExtCtx.mgr[4 + slot - 'k'].mgrVer)),
                        gExtCtx.mgr[4 + slot - 'k'].mgrPR);
            }
        }
        else if(slot == 'm' || slot == 'n')
        {
            sprintf((char *)data, "%s%c,%c", data, slot, gExtCtx.extBid[2][slot - 'a']);
        }
        else
        {

        }
    }

    sprintf((char *)data, "%s\"%c%c;", data, CR, LF);
}









void rpt_rb(char slot, unsigned char *data)
{
    /*“<aid>,<type>,[<version-MCU>],[<version-FPGA>], [<version-CPLD>],[<version-PCB>],
    <ref>,<priority>,<mask>,<leapmask>,<leap>,<leaptag>,
    [<delay>],[<delay>],[<delay>],[<delay>],[<delay>],[<delay>],[<delay>],[<delay>],
    <we>,<out>,<type-work>,<msmode>,<trmode>”*/
    int i, flag_1, flag_2;
    unsigned char tmp[3];
	unsigned char sel_ref_mod;
    memset(tmp, 0, 3);
	
    if(slot == RB1_SLOT)
    {
        if(slot_type[14] == 'K' || slot_type[14] == 'R')
        {
            sel_ref_mod = (conf_content.slot_o.sys_ref == '0')? '0':'1';
            flag_1 = strlen(rpt_content.slot_o.sy_ver);
            flag_2 = strlen(rpt_content.slot_o.rf_tzo);

            if(rpt_content.slot_o.rb_ref == '\0' || rpt_content.slot_o.rb_ref == 0xff)
            {
                tmp[0] = ',';
                tmp[1] = '\0';
            }
            else
            {
                tmp[0] = rpt_content.slot_o.rb_ref;
                tmp[1] = ',';
                tmp[2] = '\0';
            }
            sprintf((char *)data, "%s%c,%c,%s,%s",
                    data, slot, slot_type[14], flag_1 == 0 ? (unsigned char *)COMMA_3 : rpt_content.slot_o.sy_ver,
                    tmp);


            sprintf((char *)data, "%s%s,%s,%s,%s,1,",
                    data, rpt_content.slot_o.rb_prio, rpt_content.slot_o.rb_mask,
                    rpt_content.slot_o.rb_leapmask, rpt_content.slot_o.rb_leap
                   );
            for(i = 0; i < 8; i++)
            {
                sprintf((char *)data, "%s%s,", data, rpt_content.slot_o.rf_dey[i]);
            }

            //20140915
            if(flag_alm[14][4] == 0)
            {
                if('g' == rpt_content.slot_o.rb_tl_2mhz)
                {
                    rpt_content.slot_o.rb_tl_2mhz = 'f';
                }
                else
                {
                    ;
                }
            }
            else
            {
                rpt_content.slot_o.rb_tl_2mhz = 'g';
            }

            sprintf((char *)data, "%s%s,%s,%s,%s,%c,%c,%c,%c,%s,%s,%s,%s,%c,0",
                    data, (flag_2 == 0) ? (unsigned char *)COMMA_1 : rpt_content.slot_o.rf_tzo,
                    rpt_content.slot_o.rb_typework,
                    rpt_content.slot_o.rb_msmode,
                    rpt_content.slot_o.rb_trmode,
                    conf_content.slot_u.fb,
                    rpt_content.slot_o.rb_sa,
                    rpt_content.slot_o.rb_tl,
                    rpt_content.slot_o.rb_tl_2mhz,
                    conf_content.slot_o.thresh[0],
                    conf_content.slot_o.thresh[1],
                    rpt_content.slot_o.rb_phase[0],
                    rpt_content.slot_o.rb_phase[1],
                    sel_ref_mod
                    );
        }
    }
    else if(slot == RB2_SLOT)
    {
        if(slot_type[15] == 'K' || slot_type[15] == 'R')
        {
           sel_ref_mod = (conf_content.slot_p.sys_ref == '0')? '0':'1';

            /*	<responsemessage>::=“<aid>,<type>,[<version-MCU>],[<version-FPGA>], [<version-CPLD>],[<version-PCB>],
            <ref>,<priority>,<mask>,<leapmask>,<leap>,<leaptag>,
            [<delay>],[<delay>],[<delay>],[<delay>],[<delay>],[<delay>],[<delay>],[<delay>],
            <we>,<out>,<type-work>,<msmode>,<trmode>”*/
            flag_1 = strlen(rpt_content.slot_p.sy_ver);
            flag_2 = strlen(rpt_content.slot_p.rf_tzo);
            if(rpt_content.slot_p.rb_ref == '\0' || rpt_content.slot_p.rb_ref == 0xff)
            {
                tmp[0] = ',';
                tmp[1] = '\0';
            }
            else
            {
                tmp[0] = rpt_content.slot_p.rb_ref;
                tmp[1] = ',';
                tmp[2] = '\0';
            }
            sprintf((char *)data, "%s%c,%c,%s,%s",
                    data, slot, slot_type[15], (flag_1 == 0) ? (unsigned char *)COMMA_3 : rpt_content.slot_p.sy_ver,
                    tmp);
            /*	if(rpt_content.slot_p.rb_leaptag=='\0'||rpt_content.slot_p.rb_leaptag==0xff)
            		{
            			tmp[0]=',';
            			tmp[1]='\0';
            		}
            	else
            		{
            			tmp[0]=rpt_content.slot_o.rb_leaptag;
            			tmp[1]=',';
            			tmp[2]='\0';
            		}*/
            sprintf((char *)data, "%s%s,%s,%s,%s,1,",
                    data, rpt_content.slot_p.rb_prio, rpt_content.slot_p.rb_mask,
                    rpt_content.slot_p.rb_leapmask, rpt_content.slot_p.rb_leap);
            for(i = 0; i < 8; i++)
            {
                sprintf((char *)data, "%s%s,", data, rpt_content.slot_p.rf_dey[i]);
            }

            //20140915
            if(flag_alm[15][4] == 0)
            {
                if('g' == rpt_content.slot_p.rb_tl_2mhz)
                {
                    rpt_content.slot_p.rb_tl_2mhz = 'f';
                }
                else
                {
                    ;
                }
            }
            else
            {
                rpt_content.slot_p.rb_tl_2mhz = 'g';
            }
            sprintf((char *)data, "%s%s,%s,%s,%s,%c,%c,%c,%c,%s,%s,%s,%s,%c,0",
                    data, (flag_2 == 0) ? (unsigned char *)COMMA_1 : rpt_content.slot_p.rf_tzo,
                    rpt_content.slot_p.rb_typework,
                    rpt_content.slot_p.rb_msmode,
                    rpt_content.slot_p.rb_trmode,
                    conf_content.slot_u.fb,
                    rpt_content.slot_p.rb_sa,
                    rpt_content.slot_p.rb_tl,
                    rpt_content.slot_p.rb_tl_2mhz,
                    conf_content.slot_p.thresh[0],
                    conf_content.slot_p.thresh[1],
                    rpt_content.slot_p.rb_phase[0],
                    rpt_content.slot_p.rb_phase[1],
                    sel_ref_mod);
        }
    }

}


void rpt_gbtp(char slot, unsigned char *data)
{
    /* “<aid>,<type>,[<version-MCU>],[<version-FPGA>], [<version-CPLD>],[<version-PCB>],
    <ppssta>,<sourcetype>,<pos_mode>,<elevation>, <format>, <acmode>,
    <mask>, <qq-fix>,<hhmmss.ss>,<ddmm.mmmmmm>,<ns>,<dddmm.mmmmmm>,<we>,
    <saaaaa.aa>,
    <vvvv>,<tttttttttt>[,<sat>,<qq>,<pp>,<ss>,<h>]*”*/
    int flag_1, flag_2, flag_3, flag_4, flag_5, flag_6, flag_7, flag_8, flag_9, flag_10, flag_11;

    if(slot == GBTP1_SLOT)
    {
        if(slot_type[16] == 'L')
        {
            flag_1 = strlen(rpt_content.slot_q.sy_ver);
            flag_2 = strlen(rpt_content.slot_q.gb_tod);
            flag_3 = strlen(rpt_content.slot_q.gb_ori);
            flag_4 = strlen(rpt_content.slot_q.gb_rev_g);
            flag_5 = strlen(rpt_content.slot_q.gb_rev_b);
            sprintf((char *) data, "%s%c,L,%s,%s,%s,%s,", data, slot,
                     (flag_1 == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot_q.sy_ver),
                     rpt_content.slot_q.bdzb,
                     (flag_2 == 0) ? (unsigned char *)COMMA_1 : (rpt_content.slot_q.gb_tod),
                     rpt_content.slot_q.gb_pos_mode);
            sprintf((char *)data, "%s%s,%s,%s,", data, rpt_content.slot_q.gb_elevation,
                    rpt_content.slot_q.gb_format, rpt_content.slot_q.gb_acmode);
            sprintf((char *)data, "%s%s,%s,%c,%s,%s", data, rpt_content.slot_q.gb_mask,
                    (flag_3 == 0) ? (unsigned char *)COMMA_8 : (rpt_content.slot_q.gb_ori),
                    conf_content.slot_u.fb,
                    (flag_4 == 0) ? (unsigned char *)GBTP_G : (rpt_content.slot_q.gb_rev_g),
                    (flag_5 == 0) ? (unsigned char *)GBTP_B : (rpt_content.slot_q.gb_rev_b));
        }
        else if (slot_type[16] == 'P')
        {
            flag_1 = strlen(rpt_content.slot_q.sy_ver);
            flag_2 = strlen(rpt_content.slot_q.gb_tod);
            flag_3 = strlen(rpt_content.slot_q.gb_ori);
            flag_4 = strlen(rpt_content.slot_q.gb_rev_g);
            sprintf((char *) data, "%s%c,P,%s,,%s,%s,", data, slot,
                     (flag_1 == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot_q.sy_ver),
                     (flag_2 == 0) ? (unsigned char *)COMMA_1 : (rpt_content.slot_q.gb_tod),
                     rpt_content.slot_q.gb_pos_mode);
            sprintf((char *) data, "%s%s,%s,%s,", data, rpt_content.slot_q.gb_elevation,
                     rpt_content.slot_q.gb_format, rpt_content.slot_q.gb_acmode);
            sprintf((char *)data, "%s%s,%s,%c,%s", data, rpt_content.slot_q.gb_mask,
                    (flag_3 == 0) ? (unsigned char *)COMMA_8 : (rpt_content.slot_q.gb_ori),
                    conf_content.slot_u.fb,
                    (flag_4 == 0) ? (unsigned char *)GBTP_G : (rpt_content.slot_q.gb_rev_g));
        }
        else if(slot_type[16] == 'e')
        {
            flag_1 = strlen(rpt_content.slot_q.sy_ver);
            flag_2 = strlen(rpt_content.slot_q.gb_tod);
            flag_3 = strlen(rpt_content.slot_q.gb_ori);
            flag_4 = strlen(rpt_content.slot_q.gb_rev_g);
            flag_5 = strlen(rpt_content.slot_q.gb_rev_b);
            sprintf((char *)data, "%s%c,e,%s,%s,%s,%s,", data, slot,
                    (flag_1 == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot_q.sy_ver),
                    rpt_content.slot_q.bdzb,
                    (flag_2 == 0) ? (unsigned char *)COMMA_1 : (rpt_content.slot_q.gb_tod),
                    rpt_content.slot_q.gb_pos_mode);
            sprintf((char *)data, "%s%s,%s,%s,", data, rpt_content.slot_q.gb_elevation,
                    rpt_content.slot_q.gb_format, rpt_content.slot_q.gb_acmode);
            sprintf((char *)data, "%s%s,%s,%c,%s,%s", data, rpt_content.slot_q.gb_mask,
                    (flag_3 == 0) ? (unsigned char *)COMMA_8 : (rpt_content.slot_q.gb_ori),
                    conf_content.slot_u.fb,
                    (flag_4 == 0) ? (unsigned char *)GBTP_G : (rpt_content.slot_q.gb_rev_g),
                    (flag_5 == 0) ? (unsigned char *)GBTP_B : (rpt_content.slot_q.gb_rev_b));
        }
        else if(slot_type[16] == 'g')
        {
            flag_1 = strlen(rpt_content.slot_q.sy_ver);
            flag_2 = strlen(rpt_content.slot_q.gb_tod);
            flag_3 = strlen(rpt_content.slot_q.gb_ori);
            flag_4 = strlen(rpt_content.slot_q.gb_rev_g);
            flag_5 = strlen(rpt_content.slot_q.gb_rev_b);
            sprintf((char *)data, "%s%c,g,%s,%s,%s,%s,", data, slot,
                    (flag_1 == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot_q.sy_ver),
                    rpt_content.slot_q.bdzb,
                    (flag_2 == 0) ? (unsigned char *)COMMA_1 : (rpt_content.slot_q.gb_tod),
                    rpt_content.slot_q.gb_pos_mode);
            sprintf((char *)data, "%s%s,%s,%s,", data, rpt_content.slot_q.gb_elevation,
                    rpt_content.slot_q.gb_format, rpt_content.slot_q.gb_acmode);
            sprintf((char *)data, "%s%s,%s,%c,%s,%s", data, rpt_content.slot_q.gb_mask,
                    (flag_3 == 0) ? (unsigned char *)COMMA_8 : (rpt_content.slot_q.gb_ori),
                    conf_content.slot_u.fb,
                    (flag_4 == 0) ? (unsigned char *)GBTP_G : (rpt_content.slot_q.gb_rev_g),
                    (flag_5 == 0) ? (unsigned char *)GBTP_B : (rpt_content.slot_q.gb_rev_b));
        }

        else if(slot_type[16] == 'l')
        {
            flag_1 = strlen(rpt_content.slot_q.sy_ver);
            flag_2 = strlen(rpt_content.slot_q.gb_tod);
            flag_3 = strlen(rpt_content.slot_q.gb_ori);
            flag_4 = strlen(rpt_content.slot_q.gb_rev_g);
            flag_5 = strlen(rpt_content.slot_q.gb_rev_b);
            sprintf((char *)data, "%s%c,l,%s,%s,%s,%s,", data, slot,
                    (flag_1 == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot_q.sy_ver),
                    rpt_content.slot_q.bdzb,
                    (flag_2 == 0) ? (unsigned char *)COMMA_1 : (rpt_content.slot_q.gb_tod),
                    rpt_content.slot_q.gb_pos_mode);
            sprintf((char *)data, "%s%s,%s,%s,", data, rpt_content.slot_q.gb_elevation,
                    rpt_content.slot_q.gb_format, rpt_content.slot_q.gb_acmode);
            sprintf((char *)data, "%s%s,%s,%c,%s,%s", data, rpt_content.slot_q.gb_mask,
                    (flag_3 == 0) ? (unsigned char *)COMMA_8 : (rpt_content.slot_q.gb_ori),
                    conf_content.slot_u.fb,
                    (flag_4 == 0) ? (unsigned char *)GBTP_G : (rpt_content.slot_q.gb_rev_g),
                    (flag_5 == 0) ? (unsigned char *)GBTP_B : (rpt_content.slot_q.gb_rev_b));
        }
		else if(slot_type[16] == 'v')
		{
			flag_1 = strlen(rpt_content.slot_q.sy_ver);
			flag_2 = (rpt_content.slot_q.gb_sta == '\0') ? 0:1;
			flag_3 = (rpt_content.slot_q.staen == '\0') ? 0:1;
            //flag_4 =  strlen(rpt_content.slot_q.gb_pos_mode);
            //flag_5 =  strlen(rpt_content.slot_q.gb_acmode);
			flag_6 =  rpt_content.slot_q.gb_tod[0] == '\0' ? 0:1;
			flag_7 =  strlen(rpt_content.slot_q.leapalm);
			flag_8 =  strlen(rpt_content.slot_q.leapnum);
			
			flag_9 = strlen(rpt_content.slot_q.gb_ori);
			flag_10 = strlen(rpt_content.slot_q.gb_rev_g);
            flag_11 = strlen(rpt_content.slot_q.gb_rev_b);
			sprintf((char *)data, "%s%c,v,%s,%c,%c,%s,%s,%c,%s,%s,", data, slot,
                    (flag_1 == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot_q.sy_ver),
                    (flag_2 == 0) ? '2' : rpt_content.slot_q.gb_sta, 
                    (flag_3 == 0) ? '1' : rpt_content.slot_q.staen,
                    rpt_content.slot_q.gb_pos_mode,
                    rpt_content.slot_q.gb_acmode,
                    (flag_6 == 0) ? '3' : rpt_content.slot_q.gb_tod[0],
                    (flag_7 == 0) ? "00": (char *)rpt_content.slot_q.leapalm,
                    (flag_8 == 0) ? "00": (char *)rpt_content.slot_q.leapnum);
			
			sprintf((char *)data, "%s%s,%s,%s,%s,%s", data, 
				    ((flag_9 == 0) ? (unsigned char *)COMMA_9 : rpt_content.slot_q.gb_ori),
                    rpt_content.slot_q.rec_type,
                    rpt_content.slot_q.rec_ver,
                    ((flag_10 == 0) ? (unsigned char *)GBTP_GPS : rpt_content.slot_q.gb_rev_g),
                    ((flag_11 == 0) ? (unsigned char *)GBTP_BD : rpt_content.slot_q.gb_rev_b));
		         
		}
		else if(slot_type[16] == 'x')
		{
			flag_1 = strlen(rpt_content.slot_q.sy_ver);
			flag_2 = (rpt_content.slot_q.gb_sta == '\0') ? 0:1;
			flag_3 = (rpt_content.slot_q.staen == '\0') ? 0:1;
            //flag_4 =  strlen(rpt_content.slot_q.gb_pos_mode);
            //flag_5 =  strlen(rpt_content.slot_q.gb_acmode);
            //flag_4 =  strlen(rpt_content.slot_q.delay);
			
			flag_6 =  rpt_content.slot_q.gb_tod[0] == '\0' ? 0:1;
			flag_7 =  strlen(rpt_content.slot_q.leapalm);
			flag_8 =  strlen(rpt_content.slot_q.leapnum);
			
			flag_9 = strlen(rpt_content.slot_q.gb_ori);
			flag_10 = strlen(rpt_content.slot_q.gb_rev_g);
            flag_11 = strlen(rpt_content.slot_q.gb_rev_b);
			flag_4 = strlen(rpt_content.slot_q.gb_rev_glo);
            flag_5 = strlen(rpt_content.slot_q.gb_rev_gal);
			sprintf((char *)data, "%s%c,x,%s,%c,%c,%s,%s,%s,%s,%c,%s,%s,", data, slot,
                    (flag_1 == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot_q.sy_ver),
                    (flag_2 == 0) ? '2' : rpt_content.slot_q.gb_sta, 
                    (flag_3 == 0) ? '1' : rpt_content.slot_q.staen,
                    rpt_content.slot_q.gb_pos_mode,
                    rpt_content.slot_q.gb_acmode,
                    rpt_content.slot_q.delay,
                     rpt_content.slot_q.pps_out_delay,
                    (flag_6 == 0) ? '3' : rpt_content.slot_q.gb_tod[0],
                    (flag_7 == 0) ? "00": (char *)rpt_content.slot_q.leapalm,
                    (flag_8 == 0) ? "00": (char *)rpt_content.slot_q.leapnum);
			
			sprintf((char *)data, "%s%s,%s,%s,%s,%s,%s,%s", data, 
				    ((flag_9 == 0) ? (unsigned char *)COMMA_9 : rpt_content.slot_q.gb_ori),
                    rpt_content.slot_q.rec_type,
                    rpt_content.slot_q.rec_ver,
                    ((flag_10 == 0) ? (unsigned char *)GBTP_GPS : rpt_content.slot_q.gb_rev_g),
                    ((flag_11 == 0) ? (unsigned char *)GBTP_BD : rpt_content.slot_q.gb_rev_b),
                    ((flag_4 == 0) ? (unsigned char *)GBTP_GLO : rpt_content.slot_q.gb_rev_glo),
                    ((flag_5 == 0) ? (unsigned char *)GBTP_GAL : rpt_content.slot_q.gb_rev_gal));
		         
		}
    }
    else if(slot == GBTP2_SLOT)
    {
        if(slot_type[17] == 'L')
        {
            flag_1 = strlen(rpt_content.slot_r.sy_ver);
            flag_2 = strlen(rpt_content.slot_r.gb_tod);
            flag_3 = strlen(rpt_content.slot_r.gb_ori);
            flag_4 = strlen(rpt_content.slot_r.gb_rev_g);
            flag_5 = strlen(rpt_content.slot_r.gb_rev_b);
            sprintf((char *)data, "%s%c,L,%s,%s,%s,%s,", data, slot,
                    flag_1 == 0 ? (unsigned char *)COMMA_3 : (rpt_content.slot_r.sy_ver),
                    rpt_content.slot_r.bdzb,
                    flag_2 == 0 ? (unsigned char *)COMMA_1 : (rpt_content.slot_r.gb_tod),
                    rpt_content.slot_r.gb_pos_mode);
            sprintf((char *)data, "%s%s,%s,%s,", data, rpt_content.slot_r.gb_elevation,
                    rpt_content.slot_r.gb_format, rpt_content.slot_r.gb_acmode);
            sprintf((char *)data, "%s%s,%s,%c,%s,%s", data, rpt_content.slot_r.gb_mask,
                    flag_3 == 0 ? (unsigned char *)COMMA_8 : (rpt_content.slot_r.gb_ori),
                    conf_content.slot_u.fb,
                    flag_4 == 0 ? (unsigned char *)GBTP_G : (rpt_content.slot_r.gb_rev_g),
                    flag_5 == 0 ? (unsigned char *)GBTP_B : (rpt_content.slot_r.gb_rev_b));

        }
        else if(slot_type[17] == 'P')
        {
            flag_1 = strlen(rpt_content.slot_r.sy_ver);
            flag_2 = strlen(rpt_content.slot_r.gb_tod);
            flag_3 = strlen(rpt_content.slot_r.gb_ori);
            flag_4 = strlen(rpt_content.slot_r.gb_rev_g);
            sprintf((char *)data, "%s%c,P,%s,,%s,%s,", data, slot,
                    flag_1 == 0 ? (unsigned char *)COMMA_3 : (rpt_content.slot_r.sy_ver),
                    flag_2 == 0 ? (unsigned char *)COMMA_1 : (rpt_content.slot_r.gb_tod),
                    rpt_content.slot_r.gb_pos_mode);
            sprintf((char *)data, "%s%s,%s,%s,", data, rpt_content.slot_r.gb_elevation,
                    rpt_content.slot_r.gb_format, rpt_content.slot_r.gb_acmode);
            sprintf((char *)data, "%s%s,%s,%c,%s", data, rpt_content.slot_r.gb_mask,
                    flag_3 == 0 ? (unsigned char *)COMMA_8 : (rpt_content.slot_r.gb_ori),
                    conf_content.slot_u.fb,
                    flag_4 == 0 ? (unsigned char *)GBTP_G : (rpt_content.slot_r.gb_rev_g));
        }
        else if(slot_type[17] == 'e')
        {
            flag_1 = strlen(rpt_content.slot_r.sy_ver);
            flag_2 = strlen(rpt_content.slot_r.gb_tod);
            flag_3 = strlen(rpt_content.slot_r.gb_ori);
            flag_4 = strlen(rpt_content.slot_r.gb_rev_g);
            flag_5 = strlen(rpt_content.slot_r.gb_rev_b);
            sprintf((char *)data, "%s%c,e,%s,%s,%s,%s,", data, slot,
                    flag_1 == 0 ? (unsigned char *)COMMA_3 : (rpt_content.slot_r.sy_ver),
                    rpt_content.slot_r.bdzb,
                    flag_2 == 0 ? (unsigned char *)COMMA_1 : (rpt_content.slot_r.gb_tod),
                    rpt_content.slot_r.gb_pos_mode);
            sprintf((char *)data, "%s%s,%s,%s,", data, rpt_content.slot_r.gb_elevation,
                    rpt_content.slot_r.gb_format, rpt_content.slot_r.gb_acmode);
            sprintf((char *)data, "%s%s,%s,%c,%s,%s", data, rpt_content.slot_r.gb_mask,
                    flag_3 == 0 ? (unsigned char *)COMMA_8 : (rpt_content.slot_r.gb_ori),
                    conf_content.slot_u.fb,
                    flag_4 == 0 ? (unsigned char *)GBTP_G : (rpt_content.slot_r.gb_rev_g),
                    flag_5 == 0 ? (unsigned char *)GBTP_B : (rpt_content.slot_r.gb_rev_b));
        }
        else if(slot_type[17] == 'g')
        {
            flag_1 = strlen(rpt_content.slot_r.sy_ver);
            flag_2 = strlen(rpt_content.slot_r.gb_tod);
            flag_3 = strlen(rpt_content.slot_r.gb_ori);
            flag_4 = strlen(rpt_content.slot_r.gb_rev_g);
            flag_5 = strlen(rpt_content.slot_r.gb_rev_b);
            sprintf((char *)data, "%s%c,g,%s,%s,%s,%s,", data, slot,
                    flag_1 == 0 ? (unsigned char *)COMMA_3 : (rpt_content.slot_r.sy_ver),
                    rpt_content.slot_r.bdzb,
                    flag_2 == 0 ? (unsigned char *)COMMA_1 : (rpt_content.slot_r.gb_tod),
                    rpt_content.slot_r.gb_pos_mode);
            sprintf((char *)data, "%s%s,%s,%s,", data, rpt_content.slot_r.gb_elevation,
                    rpt_content.slot_r.gb_format, rpt_content.slot_r.gb_acmode);
            sprintf((char *)data, "%s%s,%s,%c,%s,%s", data, rpt_content.slot_r.gb_mask,
                    flag_3 == 0 ? (unsigned char *)COMMA_8 : (rpt_content.slot_r.gb_ori),
                    conf_content.slot_u.fb,
                    flag_4 == 0 ? (unsigned char *)GBTP_G : (rpt_content.slot_r.gb_rev_g),
                    flag_5 == 0 ? (unsigned char *)GBTP_B : (rpt_content.slot_r.gb_rev_b));
        }
        else if(slot_type[17] == 'l')
        {
            flag_1 = strlen(rpt_content.slot_r.sy_ver);
            flag_2 = strlen(rpt_content.slot_r.gb_tod);
            flag_3 = strlen(rpt_content.slot_r.gb_ori);
            flag_4 = strlen(rpt_content.slot_r.gb_rev_g);
            flag_5 = strlen(rpt_content.slot_r.gb_rev_b);
            sprintf((char *)data, "%s%c,l,%s,%s,%s,%s,", data, slot,
                    flag_1 == 0 ? (unsigned char *)COMMA_3 : (rpt_content.slot_r.sy_ver),
                    rpt_content.slot_r.bdzb,
                    flag_2 == 0 ? (unsigned char *)COMMA_1 : (rpt_content.slot_r.gb_tod),
                    rpt_content.slot_r.gb_pos_mode);
            sprintf((char *)data, "%s%s,%s,%s,", data, rpt_content.slot_r.gb_elevation,
                    rpt_content.slot_r.gb_format, rpt_content.slot_r.gb_acmode);
            sprintf((char *)data, "%s%s,%s,%c,%s,%s", data, rpt_content.slot_r.gb_mask,
                    flag_3 == 0 ? (unsigned char *)COMMA_8 : (rpt_content.slot_r.gb_ori),
                    conf_content.slot_u.fb,
                    flag_4 == 0 ? (unsigned char *)GBTP_G : (rpt_content.slot_r.gb_rev_g),
                    flag_5 == 0 ? (unsigned char *)GBTP_B : (rpt_content.slot_r.gb_rev_b));
        }
		else if(slot_type[17] == 'v')
		{
			flag_1 = strlen(rpt_content.slot_r.sy_ver);
			flag_2 = (rpt_content.slot_r.gb_sta == '\0') ? 0:1;
			flag_3 = (rpt_content.slot_r.staen == '\0') ? 0:1;
            //flag_4 =  strlen(rpt_content.slot_q.gb_pos_mode);
            //flag_5 =  strlen(rpt_content.slot_q.gb_acmode);
			flag_6 =  rpt_content.slot_r.gb_tod[0] == '\0' ? 0:1;
			flag_7 =  strlen(rpt_content.slot_r.leapalm);
			flag_8 =  strlen(rpt_content.slot_r.leapnum);
			
			flag_9 = strlen(rpt_content.slot_r.gb_ori);
			flag_10 = strlen(rpt_content.slot_r.gb_rev_g);
            flag_11 = strlen(rpt_content.slot_r.gb_rev_b);
			sprintf((char *)data, "%s%c,v,%s,%c,%c,%s,%s,%c,%s,%s,", data, slot,
                    (flag_1 == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot_r.sy_ver),
                    (flag_2 == 0) ? '2' : rpt_content.slot_r.gb_sta, 
                    (flag_3 == 0) ? '1' : rpt_content.slot_r.staen,
                    rpt_content.slot_r.gb_pos_mode,
                    rpt_content.slot_r.gb_acmode,
                    (flag_6 == 0) ? '3' : rpt_content.slot_r.gb_tod[0],
                    (flag_7 == 0) ? "00": (char *)rpt_content.slot_r.leapalm,
                    (flag_8 == 0) ? "00": (char *)rpt_content.slot_r.leapnum);
			
			sprintf((char *)data, "%s%s,%s,%s,%s,%s", data, 
				    ((flag_9 == 0) ? (unsigned char *)COMMA_9 : rpt_content.slot_r.gb_ori),
                    rpt_content.slot_r.rec_type,
                    rpt_content.slot_r.rec_ver,
                    ((flag_10 == 0) ? (unsigned char *)GBTP_GPS : rpt_content.slot_r.gb_rev_g),
                    ((flag_11 == 0) ? (unsigned char *)GBTP_BD : rpt_content.slot_r.gb_rev_b));
		         
		}
		else if(slot_type[17] == 'x')
		{
			flag_1 = strlen(rpt_content.slot_r.sy_ver);
			flag_2 = (rpt_content.slot_r.gb_sta == '\0') ? 0:1;
			flag_3 = (rpt_content.slot_r.staen == '\0') ? 0:1;
            //flag_4 =  strlen(rpt_content.slot_q.gb_pos_mode);
            //flag_5 =  strlen(rpt_content.slot_q.gb_acmode);
            //flag_4 =  strlen(rpt_content.slot_q.delay);
			
			flag_6 =  rpt_content.slot_r.gb_tod[0] == '\0' ? 0:1;
			flag_7 =  strlen(rpt_content.slot_r.leapalm);
			flag_8 =  strlen(rpt_content.slot_r.leapnum);
			
			flag_9 = strlen(rpt_content.slot_r.gb_ori);
			flag_10 = strlen(rpt_content.slot_r.gb_rev_g);
            flag_11 = strlen(rpt_content.slot_r.gb_rev_b);
			flag_4 = strlen(rpt_content.slot_r.gb_rev_glo);
            flag_5 = strlen(rpt_content.slot_r.gb_rev_gal);
			sprintf((char *)data, "%s%c,x,%s,%c,%c,%s,%s,%s,%s,%c,%s,%s,", data, slot,
                    (flag_1 == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot_r.sy_ver),
                    (flag_2 == 0) ? '2' : rpt_content.slot_r.gb_sta, 
                    (flag_3 == 0) ? '1' : rpt_content.slot_r.staen,
                    rpt_content.slot_r.gb_pos_mode,
                    rpt_content.slot_r.gb_acmode,
                    rpt_content.slot_r.delay,
                    rpt_content.slot_r.pps_out_delay,
                    (flag_6 == 0) ? '3' : rpt_content.slot_r.gb_tod[0],
                    (flag_7 == 0) ? "00": (char *)rpt_content.slot_r.leapalm,
                    (flag_8 == 0) ? "00": (char *)rpt_content.slot_r.leapnum);
			
			sprintf((char *)data, "%s%s,%s,%s,%s,%s,%s,%s", data, 
				    ((flag_9 == 0) ? (unsigned char *)COMMA_9 : rpt_content.slot_r.gb_ori),
                    rpt_content.slot_r.rec_type,
                    rpt_content.slot_r.rec_ver,
                    ((flag_10 == 0) ? (unsigned char *)GBTP_GPS : rpt_content.slot_r.gb_rev_g),
                    ((flag_11 == 0) ? (unsigned char *)GBTP_BD : rpt_content.slot_r.gb_rev_b),
                    ((flag_4 == 0) ? (unsigned char *)GBTP_GLO : rpt_content.slot_r.gb_rev_glo),
                    ((flag_5 == 0) ? (unsigned char *)GBTP_GAL : rpt_content.slot_r.gb_rev_gal));
			}
		         
		
    }
}

void get_sta1_ver(unsigned char *data, int num)
{
    //sprintf((char *)data,"%s,%s,%s,,1,JLABAE@C,JLABAE@A,OOOOOO@@,,,",data,slot,"V01.00","V01.00");

	if(NULL != strchr(rpt_content.slot[num].sy_ver,','))
	{
		sprintf((char *)data, "%s,%s1", data, rpt_content.slot[num].sy_ver);
	}
	else
	{
		sprintf((char *)data, "%s,,,,1", data);
	}
    


    if( (( (conf_content3.sta1.ip[0] ) & 0xf0 ) != 0x40) ||
            (((conf_content3.sta1.ip[2] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.ip[4] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.ip[6] ) & 0xf0) != 0x40) )
    {

        memset(conf_content3.sta1.ip, 0, 9);
        memcpy(conf_content3.sta1.ip, "L@JH@AFC", 8);
    }

    if( (((conf_content3.sta1.gate[0] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.gate[2] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.gate[4] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.gate[6] ) & 0xf0) != 0x40) )
    {

        memset(conf_content3.sta1.gate, 0, 9);
        memcpy(conf_content3.sta1.gate, "L@JH@A@A", 8);
    }

    if( (((conf_content3.sta1.mask[0] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.mask[2] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.mask[4] ) & 0xf0) != 0x40) ||
            (((conf_content3.sta1.mask[6] ) & 0xf0) != 0x40) )
    {


        memset(conf_content3.sta1.mask, 0, 9);
        memcpy(conf_content3.sta1.mask, "OOOOOO@@", 8);
    }

    sprintf((char *)data, "%s,%s,%s,%s,,,", data, conf_content3.sta1.ip, conf_content3.sta1.gate, conf_content3.sta1.mask);
    //sprintf((char *)data,"%s,%s,%s,,1,JLABAE@C,JLABAE@A,OOOOOO@@,,,",data,slot,"V01.00","V01.00");
}
void __srtcat(unsigned char *dist,unsigned char *tail)
{
	int res;
	unsigned char comma[2];
	comma[0] = ',';
	comma[1] = 0;
	if(dist == NULL || tail == NULL)
	{
        return ;
    }
	strcat(dist,comma);
	res = strlen(tail);
	if(0 == res)
	{
		//strcat(dist,",");
	}
	else
	{
        strcat(dist,tail);
	}
    
}

void str_2_net(unsigned char net_addr,unsigned char *h_buf,unsigned char *l_buf)
{
	unsigned char h_4bit,l_4bit;
	
	l_4bit = (net_addr & 0x0f) | 0x40;
	h_4bit = ((net_addr & 0xf0) >> 4) | 0x40;
	
	//printf("o:%x\n",net_addr);
	//printf("h:%x,l:%x\n",h_4bit,l_4bit);
	*h_buf = h_4bit;
	*l_buf = l_4bit;
}

void strcat_mac(unsigned char * data,int num,int port)
{
	int i;
	char buf[13];
	memset(buf,0,13);
	for(i=0;i<6;i++)
	{ 
		str_2_net(rpt_content.slot[num].ntp_pmac[port][i],&buf[i*2],&buf[i*2+1]);
	}
	__srtcat(data,buf);
}

void strcat_ip(unsigned char * data,int num,int port)
{
	int i;
	char buf[13];
	memset(buf,0,13);
	for(i=0;i<4;i++)
	{ 
		str_2_net(rpt_content.slot[num].ntp_pip[port][i],&buf[i*2],&buf[i*2+1]);
	}
	__srtcat(data,buf);
}
void strcat_gate(unsigned char * data,int num,int port)
{
	int i;
	char buf[13];
	memset(buf,0,13);
	for(i=0;i<4;i++)
	{ 
		str_2_net(rpt_content.slot[num].ntp_pgateway[port][i],&buf[i*2],&buf[i*2+1]);
	}
	__srtcat(data,buf);
}
void strcat_mask(unsigned char * data,int num,int port)
{
	int i;
	char buf[13];
	memset(buf,0,13);
	for(i=0;i<4;i++)
	{ 
		str_2_net(rpt_content.slot[num].ntp_pnetmask[port][i],&buf[i*2],&buf[i*2+1]);
	}
	__srtcat(data,buf);
}

void rpt_out(char slot, unsigned char *data)
{
    int num, i;
    unsigned char tmp[4];
    unsigned char tmp1[3], tmp2[3], tmp3[3], tmp4[3];
    int flag_1, flag_2;
    unsigned char tmp_buf[4];
    int length = 0;
    num = (int)(slot - 'a');
    if(slot_type[num] == 'I')
    {
        /*	 "<aid>,<type>,
        [<version-MCU>],[<version-FPGA>], [<version-CPLD>],[<version-PCB>],
        <ppssta>,<sourcetype>,<en>”*/
        flag_1 = strlen(rpt_content.slot[num].sy_ver);
        flag_2 = strlen(rpt_content.slot[num].tp16_tod);
        memcpy(tmp_buf, rpt_content.slot[num].tp16_tod, strlen(rpt_content.slot[num].tp16_tod));
		//
        //pps state switch
        //
        if('0' == tmp_buf[0])
        {
            tmp_buf[0] = '1';
        }
        else if('1' == tmp_buf[0])
        {
            tmp_buf[0] = '2';
        }
        else if('2' == tmp_buf[0])
        {
            tmp_buf[0] = '3';
        }
        else if('5' == tmp_buf[0])
        {
            tmp_buf[0] = '4';
        }
        else
        {
            tmp_buf[0] = '3';
        }

        //source type switch
        if('2' == tmp_buf[2])
        {
            tmp_buf[2] = '3';
        }
        else if('3' == tmp_buf[2])
        {
            tmp_buf[2] = '2';
        }
        else
        {
            //do nothing
        }
        //printf("%s\t%s\n",rpt_content.slot[num].tp16_tod,tmp_buf);
        sprintf((char *)data, "%s%c,I,%s,%s,%s", data, slot,
                (flag_1 == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot[num].sy_ver),
                (flag_2 == 0) ? (unsigned char *)COMMA_1 : tmp_buf,
                rpt_content.slot[num].tp16_en);
    }
    else if( (slot_type[num] == 'J') ||
             (slot_type[num] == 'a') ||
             (slot_type[num] == 'b') ||
             (slot_type[num] == 'c') ||
             (slot_type[num] == 'h') ||
             (slot_type[num] == 'i'))
    {
        if((rpt_content.slot[num].out_mode == '0') || (rpt_content.slot[num].out_mode == '1'))
        {}
        else
        {
            rpt_content.slot[num].out_mode = '0';
        }
        flag_1 = strlen(rpt_content.slot[num].sy_ver);
        sprintf((char *)data, "%s%c,%c,%s,%s,%s,%c,%c", data, slot, slot_type[num],
                flag_1 == 0 ? (unsigned char *)(COMMA_3) : (rpt_content.slot[num].sy_ver),
                rpt_content.slot[num].out_lev,
                rpt_content.slot[num].out_typ,
                rpt_content.slot[num].out_mode,
                rpt_content.slot[num].out_sta);
    }
    else if(((slot_type[num] > '@') && (slot_type[num] < 'I')) || ((slot_type[num] > 109) && (slot_type[num] < 114)) )//PTP
    {
        flag_1 = strlen(rpt_content.slot[num].sy_ver);
        sprintf((char *)data, "%s%c,%c,%s", data, slot, slot_type[num],
                (flag_1 == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot[num].sy_ver));
        for(i = 0; i < 4; i++)
        {

            sprintf((char *)data, "%s,%d,%s,%s,%s,%s,%s,%s", data, i + 1,
                    rpt_content.slot[num].ptp_ip[i],
                    rpt_content.slot[num].ptp_mac[i],
                    rpt_content.slot[num].ptp_gate[i],
                    rpt_content.slot[num].ptp_mask[i],
                    rpt_content.slot[num].ptp_dns1[i],
                    rpt_content.slot[num].ptp_dns2[i]);

            sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_en[i]);
            sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_delaytype[i]);
            sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_multicast[i]);
            sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_enp[i]);
            sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_step[i]);
#if 0
            sprintf((char *)data, "%s,%c,%c,%c,%c,%c", data,
                    rpt_content.slot[num].ptp_en[i],
                    rpt_content.slot[num].ptp_delaytype[i],
                    rpt_content.slot[num].ptp_multicast[i],
                    rpt_content.slot[num].ptp_enp[i],
                    rpt_content.slot[num].ptp_step[i]);
#endif
            sprintf((char *)data, "%s,%s", data, rpt_content.slot[num].ptp_sync[i]);
            sprintf((char *)data, "%s,%s", data, rpt_content.slot[num].ptp_announce[i]);
            sprintf((char *)data, "%s,%s", data, rpt_content.slot[num].ptp_delayreq[i]);
            sprintf((char *)data, "%s,%s", data, rpt_content.slot[num].ptp_pdelayreq[i]);
            sprintf((char *)data, "%s,%s", data, rpt_content.slot[num].ptp_delaycom[i]);
            sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_linkmode[i]);
            sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_out[i]);
            //2014.12.2
            if(rpt_ptp_mtc.ptp_prio1[num][i][0] != 'A')
            {
                //printf("\nrpt_ptp_mtc.ptp_prio1[%d][%d][0] != A\n",num,i);
                memset(rpt_ptp_mtc.ptp_prio1[num][i], 0, 5);
            }
            if(rpt_ptp_mtc.ptp_prio2[num][i][0] != 'B')
            {
                //printf("\nrpt_ptp_mtc.ptp_prio2[%d][%d][0] != B\n",num,i);
                memset(rpt_ptp_mtc.ptp_prio2[num][i], 0, 5);
            }
            sprintf((char *)data, "%s,%s", data, rpt_ptp_mtc.ptp_prio1[num][i]);
            sprintf((char *)data, "%s,%s", data, rpt_ptp_mtc.ptp_prio2[num][i]);
			//sprintf((char *)data, "%s,%s", data, rpt_ptp_mtc.ptp_dom[num][i]);

#if 0
            sprintf((char *)data, "%s,%s,%s,%s,%s,%s,%c,%c", data,
                    rpt_content.slot[num].ptp_sync[i],
                    rpt_content.slot[num].ptp_announce[i],
                    rpt_content.slot[num].ptp_delayreq[i],
                    rpt_content.slot[num].ptp_pdelayreq[i],
                    rpt_content.slot[num].ptp_delaycom[i],
                    rpt_content.slot[num].ptp_linkmode[i],
                    rpt_content.slot[num].ptp_out[i]);
#endif

        }
		sprintf((char *)data, "%s,%s", data, rpt_content.slot[num].ntp_leap);
    }
	else if(slot_type[num] == 'j')
	{
	      flag_1 = strlen(rpt_content.slot[num].sy_ver);
          sprintf((char *)data, "%s%c,%c,%s", data, slot, slot_type[num],
                (flag_1 == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot[num].sy_ver));
		  for(i = 0; i < 4; i++)
          {

            sprintf((char *)data, "%s,%d,%s,%s,%s,%s", data, i + 1,
				    rpt_content.slot[num].ptp_mac[i],
                    rpt_content.slot[num].ptp_ip[i],
                    rpt_content.slot[num].ptp_mask[i],
                    rpt_content.slot[num].ptp_gate[i]
                    );

            sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_en[i]);       //PTP Enable
			sprintf((char *)data, "%s,%c", data, rpt_ptp_mtc.esmc_en[num][i]);           //Esmc enable
            sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_delaytype[i]);//Delay type
            sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_multicast[i]);//multicast
            sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_enp[i]);      //enp
            sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_step[i]);     //step
            //
            //!
            //
            sprintf((char *)data, "%s,%s", data, rpt_content.slot[num].ptp_sync[i]);
            sprintf((char *)data, "%s,%s", data, rpt_content.slot[num].ptp_announce[i]);
            sprintf((char *)data, "%s,%s", data, rpt_content.slot[num].ptp_delaycom[i]);

            if(rpt_ptp_mtc.ptp_prio1[num][i][0] != 'A')
            {
                //printf("\nrpt_ptp_mtc.ptp_prio1[%d][%d][0] != A\n",num,i);
                memset(rpt_ptp_mtc.ptp_prio1[num][i], 0, 5);
            }
            if(rpt_ptp_mtc.ptp_prio2[num][i][0] != 'B')
            {
                //printf("\nrpt_ptp_mtc.ptp_prio2[%d][%d][0] != B\n",num,i);
                memset(rpt_ptp_mtc.ptp_prio2[num][i], 0, 5);
            }
            sprintf((char *)data, "%s,%s", data, rpt_ptp_mtc.ptp_prio1[num][i]);
            sprintf((char *)data, "%s,%s", data, rpt_ptp_mtc.ptp_prio2[num][i]);
			sprintf((char *)data, "%s,%s", data, rpt_ptp_mtc.ptp_dom[num][i]);  //ptp domain
			sprintf((char *)data, "%s,%s", data, rpt_ptp_mtc.ptp_ver[num][i]);  //pt version
			//
			//!sfp modual .
			//
            sprintf((char *)data, "%s,%s,%s,%s,%s", data, rpt_ptp_mtc.ptp_mtc_ip[num][i],rpt_ptp_mtc.sfp_type[num][i],rpt_ptp_mtc.sfp_oplo[num][i],rpt_ptp_mtc.sfp_ophi[num][i]);  //PTP mtc,sfp type,oplo,ophi
            //
            //!ptp status byte.
            //
            sprintf((char *)data, "%s,%s,%s,%s,%s", data, rpt_ptp_mtc.time_source[num][i],rpt_ptp_mtc.clock_class[num][i],rpt_ptp_mtc.time_acc[num][i],rpt_ptp_mtc.synce_ssm[num][i]);//time source,clock class,time acc,synce
        }
		sprintf((char *)data, "%s,%s", data, rpt_content.slot[num].ntp_leap);
		  
	}
	else if(slot_type[num] == 's')
	{
	    flag_1 = strlen(rpt_content.slot[num].sy_ver);
        sprintf((char *)data, "%s%c,%c,%s", data, slot, slot_type[num],
                (flag_1 == 0) ? (unsigned char *)COMMA_3 : (rpt_content.slot[num].sy_ver));
        sprintf((char *)data, "%s,%d,%s,%s,%s", data, 1, 	    rpt_content.slot[num].ptp_ip[0],
                    											rpt_content.slot[num].ptp_gate[0],
                    											rpt_content.slot[num].ptp_mask[0]);
		
        sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_delaytype[0]);
        sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_multicast[0]);
        sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_enp[0]);
        sprintf((char *)data, "%s,%c", data, rpt_content.slot[num].ptp_step[0]);

       
        sprintf((char *)data, "%s,%s", data, rpt_content.slot[num].ptp_delayreq[0]);
        sprintf((char *)data, "%s,%s", data, rpt_content.slot[num].ptp_pdelayreq[0]);
        sprintf((char *)data, "%s,%s", data, rpt_content.slot[num].ptp_clkcls[0]);
		sprintf((char *)data, "%s,%s", data, rpt_content.slot[num].ntp_leap);
     
	}
    else if(slot_type[num] == 'T' || slot_type[num] == 'X')
    {
        flag_1 = strlen(rpt_content.slot[num].sy_ver);
        sprintf((char *)data, "%s%c,%c,%s,%s,%s", data, slot, slot_type[num],
                flag_1 == 0 ? (unsigned char *)(COMMA_3) : (rpt_content.slot[num].sy_ver),
                rpt_content.slot[num].rs_en,
                rpt_content.slot[num].rs_tzo);
    }
    else if(slot_type[num] == 'S')
    {
        /*<response message>::= "<aid>,<type>,[<version-MCU>],[<version-FPGA>], [<version-CPLD>],[<version-PCB>],
        < priority1 >,< priority2>,<mode1>,<port1>,<mode2>,<port2>,
        <tl1>,<tl2>,<tl3>,<tl4>,<tl5>,<tl6>,<tl7>,<tl8>,<ph1 >,
        <ph2>,<ph3 >,<ph4>,<ph5 >,<ph6>,<ph7 >,<ph8>"*/
        flag_1 = strlen(rpt_content.slot[num].sy_ver);
        sprintf((char *)data, "%s%c,S,%s,", data, slot,
                flag_1 == 0 ? (unsigned char *)(COMMA_3) : (rpt_content.slot[num].sy_ver));

        flag_1 = strlen(rpt_content.slot[num].ref_mod1);
        flag_2 = strlen(rpt_content.slot[num].ref_mod2);
        sprintf((char *)data, "%s%s,%s,%s,%s,", data,
                rpt_content.slot[num].ref_prio1,
                rpt_content.slot[num].ref_prio2,
                flag_1 == 0 ? (unsigned char *)(COMMA_1) : (rpt_content.slot[num].ref_mod1),
                flag_2 == 0 ? (unsigned char *)(COMMA_1) : (rpt_content.slot[num].ref_mod2));

        flag_1 = strlen(rpt_content.slot[num].ref_ph);
        //	flag_2=strlen(rpt_content.slot[num].ref_tl);
        sprintf((char *)data, "%s%s,%s,%s,%s,%s,%s,%s,%c", data,
                rpt_content.slot[num].ref_typ1,
                rpt_content.slot[num].ref_typ2,
                rpt_content.slot[num].ref_sa,
                rpt_content.slot[num].ref_tl,
                rpt_content.slot[num].ref_en,
                flag_1 == 0 ? (unsigned char *)(COMMA_7) : (rpt_content.slot[num].ref_ph),
                ((0 == strlen(rpt_content.slot[num].ref_ssm_en)) ? (unsigned char *)("00000000") : rpt_content.slot[num].ref_ssm_en),rpt_content.slot[num].ref_2mb_lvlm);
        //		flag_2==0?(unsigned char *)(COMMA_7):(rpt_content.slot[num].ref_tl));
    }
    else if(slot_type[num] == 'U')
    {
        flag_1 = strlen(rpt_content.slot[num].sy_ver);
        sprintf((char *)data, "%s%c,U,%s,%s", data, slot,
                flag_1 == 0 ? (unsigned char *)(COMMA_3) : (rpt_content.slot[num].sy_ver),
                rpt_content.slot[num].ppx_mod);
    }
    else if(slot_type[num] == 'V')
    {
        flag_1 = strlen(rpt_content.slot[num].sy_ver);
        memset(tmp, 0, 4);
        if((rpt_content.slot[num].tod_br < 0x30 ) || (rpt_content.slot[num].tod_br > 0x37))
        {
            sprintf((char *)tmp, ",%c", rpt_content.slot[num].tod_tzo);
        }
        else
        {
            sprintf((char *)tmp, "%c,%c", rpt_content.slot[num].tod_br, rpt_content.slot[num].tod_tzo);
        }

        sprintf((char *)data, "%s%c,V,%s,%s,%s", data, slot,
                flag_1 == 0 ? (unsigned char *)(COMMA_3) : (rpt_content.slot[num].sy_ver),
                rpt_content.slot[num].tod_en, tmp);
    }
    else if(slot_type[num] == 'W')
    {
        flag_1 = strlen(rpt_content.slot[num].sy_ver);
        sprintf((char *)data, "%s%c,W,%s,%s,%s,%s", data, slot,
                flag_1 == 0 ? (unsigned char *)(COMMA_3) : (rpt_content.slot[num].sy_ver),
                rpt_content.slot[num].igb_en,
                rpt_content.slot[num].igb_rat,
                rpt_content.slot[num].igb_max);
    }
    else if(slot_type[num] == 'Z')
    {
        /*<ip>,<mac>,<gateway>,<netmask>,
        <bcast_en>,<interval>,<tpv_en>,<md5_en>,<ois_en>,
        [<keyid>,<md5key>]+*/
#if 1
        flag_1 = strlen(rpt_content.slot[num].sy_ver);
        sprintf((char *)data, "%s%c,Z,%s,", data, slot,
                flag_1 == 0 ? (unsigned char *)(COMMA_3) : (rpt_content.slot[num].sy_ver));

        if((rpt_content.slot[num].ntp_bcast_en == '1') || (rpt_content.slot[num].ntp_bcast_en == '0'))
        {
            sprintf((char *)tmp1, "%c,", rpt_content.slot[num].ntp_bcast_en);
            tmp1[2] = '\0';
        }
        else
        {
            tmp1[0] = ',';
            tmp1[1] = '\0';
        }
        if((rpt_content.slot[num].ntp_tpv_en >= '@') && (rpt_content.slot[num].ntp_tpv_en < 'H'))
        {
            sprintf((char *)tmp2, "%c,", rpt_content.slot[num].ntp_tpv_en);
            tmp2[2] = '\0';
        }
        else
        {
            tmp2[0] = ',';
            tmp2[1] = '\0';
        }
        if((rpt_content.slot[num].ntp_md5_en == '1') || (rpt_content.slot[num].ntp_md5_en == '0'))
        {
            sprintf((char *)tmp3, "%c,", rpt_content.slot[num].ntp_md5_en);
            tmp3[2] = '\0';
        }
        else
        {
            tmp3[0] = ',';
            tmp3[1] = '\0';
        }
        if((rpt_content.slot[num].ntp_leap[0] >= '0') && (rpt_content.slot[num].ntp_leap[0] <= '9'))
        {
            strncpy((char *)tmp4,rpt_content.slot[num].ntp_leap,2);
            tmp4[2] = '\0';
        }
        else
        {
            tmp4[0] = '\0';
        }

        sprintf((char *)data, "%s%s%s,%s%s%s", data,
                tmp1,
                rpt_content.slot[num].ntp_interval,
                tmp2,
                tmp3,
                tmp4);
		if(rpt_content.slot[num].ntp_mcu[0] < '0' || rpt_content.slot[num].ntp_mcu[0] >'9')
		{
			memset(rpt_content.slot[num].ntp_mcu,0,sizeof(rpt_content.slot[num].ntp_mcu));
		}
		__srtcat(data,rpt_content.slot[num].ntp_mcu);
		__srtcat(data,rpt_content.slot[num].ntp_portEn);
		__srtcat(data,rpt_content.slot[num].ntp_portStatus);
        for(i = 0; i < 12; i++)
        {
			tmp1[0] = i+'1';
			tmp1[1] = 0;
			if(9 == i)
			{
				tmp1[0] = '1';
				tmp1[1] = '0';
			}
			else if(10 == i)
			{
				tmp1[0] = '1';
				tmp1[1] = '1';
			}
			if(11 == i)
			{
				tmp1[0] = '1';
				tmp1[1] = '2';
			}
			__srtcat(data,tmp1);
			strcat_mac(data,num,i);
			strcat_ip(data,num,i);
			strcat_gate(data,num,i);
			strcat_mask(data,num,i);
		}
		for(i = 0; i < 4; i++)
		{
		     length = strlen(data);
			 if(0 == i)
		        sprintf(&data[length],",%s,%s,",rpt_content.slot[num].ntp_keyid[i],rpt_content.slot[num].ntp_key[i]);
			 else if(3 == i)
			 	sprintf(&data[length],"%s,%s",rpt_content.slot[num].ntp_keyid[i],rpt_content.slot[num].ntp_key[i]);
			 else
			 	sprintf(&data[length],"%s,%s,",rpt_content.slot[num].ntp_keyid[i],rpt_content.slot[num].ntp_key[i]);
			 	
				
		}
#else
		sprintf(data,"%s%s",data,"i,Z,V01.04,V02.01,,V00.01,1,0064,D,0,17,9,OO@O,OO@O,1,AABBCCDDEEFF,L@JH@OA@,L@JH@O@A,OOOOOO@@,2,AABBCCDDEEFF,L@JH@OA@,L@JH@O@A,OOOOOO@@,3,AABBCCDDEEFF,L@JH@OA@,L@JH@O@A,OOOOOO@@,4,AABBCCDDEEFF,L@JH@OA@,L@JH@O@A,OOOOOO@@,5,AABBCCDDEEFF,L@JH@OA@,L@JH@O@A,OOOOOO@@,6,AABBCCDDEEFF,L@JH@OA@,L@JH@O@A,OOOOOO@@,7,AABBCCDDEEFF,L@JH@OA@,L@JH@O@A,OOOOOO@@,8,AABBCCDDEEFF,L@JH@OA@,L@JH@O@A,OOOOOO@@,9,AABBCCDDEEFF,L@JH@OA@,L@JH@O@A,OOOOOO@@,10,AABBCCDDEEFF,L@JH@OA@,L@JH@O@A,OOOOOO@@,11,AABBCCDDEEFF,L@JH@OA@,L@JH@O@A,OOOOOO@@,12,AABBCCDDEEFF,L@JH@OA@,L@JH@O@A,OOOOOO@@");
		//sprintf(data,"%s%s",data,"j,Z,V01.04,V02.01,,,1,0064,D,0,17,9,,,1,L@JH@OA@,AABBCCDDEEFF,L@JH@O@A,OOOOOO@@");
#endif
    }
    else if('m' == slot_type[num])
    {
        sprintf((char *)data, "%s%c,m", data, slot);
        get_sta1_ver(data, num);
        //sprintf((char *)data,"%s%c,m,%s,%s,,1,JLABAE@C,JLABAE@A,OOOOOO@@,,,",data,slot,"V01.00","V01.00");
    }
}

void rpt_test(char slot, unsigned char *data)
{
    int num = (int)(slot - 'a');
    if((num >= 0) && (num < 14))
    {
        sprintf((char *)data, "%s%c,Y,%s", data, slot, rpt_content.slot[num].sy_ver);
    }
    else if(num == 14)
    {
        sprintf((char *)data, "%s%c,Y,%s", data, slot, rpt_content.slot_o.sy_ver);
    }
    else if(num == 15)
    {
        sprintf((char *)data, "%s%c,Y,%s", data, slot, rpt_content.slot_p.sy_ver);
    }
    else if(num == 16)
    {
        sprintf((char *)data, "%s%c,Y,%s", data, slot, rpt_content.slot_q.sy_ver);
    }
    else if(num == 17)
    {
        sprintf((char *)data, "%s%c,Y,%s", data, slot, rpt_content.slot_r.sy_ver);
    }
}



void rpt_drv(char slot, unsigned char *data)
{
    int num = (int)(slot - 'a');

    if( (12 == num) || (13 == num) )
    {
        sprintf((char *)data, "%s%c,d,%s,,,%d,%d,%d",
                data, slot,
                ((0 == strlen(gExtCtx.drv[num - 12].drvVer)) ? (char *)"," : (char *)(gExtCtx.drv[num - 12].drvVer)),
                gExtCtx.drv[num - 12].drvPR[0],
                gExtCtx.drv[num - 12].drvPR[1],
                gExtCtx.drv[num - 12].drvPR[2]);
    }
}



void rpt_mcp(unsigned char *data)
{
    /*“<aid>,<type>,[<version-MCU>],[<version-FPGA>], [<version-CPLD>],[<version-PCB>],
    <ip>,<gateway>,<netmask>, < dns1>,< dns2>,<mac>,<source>,<leapnum>”*/

    unsigned char  mcp_ssm_th = 0;
    unsigned char  mcp_ip[16];
    unsigned char  mcp_gateway[16];
    unsigned char  mcp_mask[16];
    unsigned char  mcp_mac[18];
    //int len;
    memset( mcp_ip, '\0', sizeof(unsigned char) * 16);
    memset( mcp_gateway, '\0', sizeof(unsigned char) * 16);
    memset( mcp_mask, '\0', sizeof(unsigned char) * 16);
    memset( mcp_mac, '\0', sizeof(unsigned char) * 18);

    API_Get_McpIp(mcp_ip);
    API_Get_McpGateWay(mcp_gateway);
    //printf("gateway=%s\n",mcp_gateway);
    if(if_a_string_is_a_valid_ipv4_address(mcp_gateway) == 0)
    {}
    else
    {
        memset( mcp_gateway, '\0', sizeof(unsigned char) * 16);
    }
    API_Get_McpMask(mcp_mask);
    API_Get_McpMacAddr(mcp_mac);
    if(if_a_string_is_a_valid_ipv4_address(conf_content.slot_u.dns1) == 0)
    {}
    else
    {
        memset( conf_content.slot_u.dns1, '\0', sizeof(unsigned char) * 16);
    }
    if(if_a_string_is_a_valid_ipv4_address(conf_content.slot_u.dns2) == 0)
    {}
    else
    {
        memset( conf_content.slot_u.dns2, '\0', sizeof(unsigned char) * 16);
    }
    sprintf((char *)data, "%su,N,%s,%s,,,%s,%s,%s,%s,%s,%s", data, (unsigned char *)MCP_VER,
            fpga_ver,
            mcp_ip, mcp_gateway, mcp_mask,
            conf_content.slot_u.dns1,
            conf_content.slot_u.dns2, mcp_mac);
    switch(conf_content.slot_u.out_ssm_oth)
    {
    case 0x02:
        mcp_ssm_th = 'a';
        break;
    case 0x04:
        mcp_ssm_th = 'b';
        break;
    case 0x08:
        mcp_ssm_th = 'c';
        break;
    case 0x0b:
        mcp_ssm_th = 'd';
        break;
    case 0x0f:
        mcp_ssm_th = 'e';
        break;
    case 0x00:
        mcp_ssm_th = 'f';
        break;
    }
#if 0
    sprintf((char *)data, "%s,%c,%s,%c,%s,%c,%c", data,
            conf_content.slot_u.time_source,
            conf_content.slot_u.leap_num,
            conf_content.slot_u.fb,
            conf_content.slot_u.tl,
            conf_content.slot_u.out_ssm_en,
            mcp_ssm_th);
#endif
    //2014-6-19
    sprintf((char *)data, "%s,%c", data, conf_content.slot_u.time_source);
    sprintf((char *)data, "%s,%s", data, conf_content.slot_u.leap_num);
    sprintf((char *)data, "%s,%c", data, conf_content.slot_u.fb);
    sprintf((char *)data, "%s,%s", data, conf_content.slot_u.tl);
    sprintf((char *)data, "%s,%c", data, conf_content.slot_u.out_ssm_en);
    sprintf((char *)data, "%s,%c,%c", data, mcp_ssm_th,conf_content3.mcp_protocol);
	sprintf((char *)data, "%s,%s%c%s", data, mcp_date,SP,mcp_time);
    //2014.12.9
    /*
    if(conf_content3.LeapMod !='0' && conf_content3.LeapMod != '1')
    {
    	conf_content3.LeapMod = '0';
    	save_config();
    }
    sprintf((char *)data,"%s,%c",data,conf_content3.LeapMod);
    */


}



void get_gbtp_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
    switch(num)
    {
    case 0x00:
    case 0x01:
	{
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
        break;
    }
}
void get_btp_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
    switch(num)
    {
    
    case 0x01:
	{
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }
	case 0x00:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
        break;
    }
}
void get_gtp_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{ 
    switch(num)
    {
    case 0x00:
    {
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
        break;
    }
}


void get_rb_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
    switch(num)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:


    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
	case 16:
	case 17:
	case 18:
    {
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }
    default:
		break;
    }
}

void get_TP16_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
    switch(num)
    {
    case 00:
    case 01:
    case 02:
    case 03:
    {
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }
    /*case 04:
    case 05:
    case 06:
    case 07:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
	case 20:	
    {
        memcpy(ntfcncde, TYPE_MJ, 2);
        memcpy(almcde, TYPE_MJ_A, 2);
        break;
    }
    case 21:
    case 22:
    case 23:
        break;*/
    default:
    break;

    }
}

void get_OUT16_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
    switch(num)
    {
	    /*case 00:
	    case 01:
	    case 18:
	    {
	        memcpy(ntfcncde, TYPE_MN, 2);
	        memcpy(almcde, TYPE_MN_A, 2);
	        break;
	    }
	    case 02:
	    case 03:
	    case 04:
	    case 05:
	    case 06:
	    case 07:
	    case 8:
	    case 9:
	    case 10:
	    case 11:
	    case 12:
	    case 13:
	    case 14:
	    case 15:
	    case 16:
	    case 17:
	    {
	        memcpy(ntfcncde, TYPE_MJ, 2);
	        memcpy(almcde, TYPE_MJ_A, 2);
	        break;
	    }
	    case 19:
	    case 20:
	    case 21:
	    case 22:
	    case 23:
	        break;*/
    default:
    break;
    }
}

void get_PTP_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
    switch(num)
    {
    /*case 00:
    case 01:
    case 02:
    case 03:
    {
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }
    case 04:
    case 05:
    case 06:
    case 07:
    case 8:
    case 9:
    case 10:
    case 11:
	case 12:
    {
        memcpy(ntfcncde, TYPE_MJ, 2);
        memcpy(almcde, TYPE_MJ_A, 2);
        break;
    }
    case 13:
    case 14:
    case 15:
        break;*/
    default:
    break;
    }
}
/***************************FUNCTION  :get_PGEIN_alm_type********************************/
/***************************PARAMETERS:                  ********************************/
void get_PGEIN_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
  switch(num)
  {
    case 00:
	{
        memcpy(ntfcncde, TYPE_MJ, 2);
        memcpy(almcde, TYPE_MJ_A, 2);
        break;
    }	
    /*case 01:
		
    case 02:
    case 03:
    case 04:
    case 05:
    case 06:
    case 07:
    case 8:
    case 9:
    case 10:
    case 11:
	case 12:
    case 13:
    case 14:
    case 15:
        break;*/
    default:
	break;
  }
}
void get_PGE4V_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
  switch(num)
  {
    case 00:
	case 01:
	case 02:
    case 03:
    case 04:
    case 05:
    case 06:
    case 07:
	case 8:
    case 9:
   // case 08:
   // case 09:
	case 16:
	case 17:
	case 24:
	case 25:
	case 32:
	case 33:	
	{
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }	
   
    /*case 10:
    case 11:
	case 12:
    case 13:
    case 14:
    case 15:
        break;*/
    default:
    break;
  }
}

void get_GBTPIIV2_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
  switch(num)
  {
    case 00:
	case 02:
    case 03:
    case 04:
	{
        memcpy(ntfcncde, TYPE_MJ, 2);
        memcpy(almcde, TYPE_MJ_A, 2);
		break;
	}
    
	case 01:	
	{
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }	
   
    /*case 10:
    case 11:
	case 12:
    case 13:
    case 14:
    case 15:
        break;*/
    default:
    break;
  }
}

void get_drv_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
  switch(num)
  {
    case 00:
	case 01:
	case 02:
    
	{
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }	
   
    /*
    case 03:
    case 04:
    case 05:
    case 06:
    case 07:
    case 08:
    case 09:
	case 16:
	case 17:
	case 24:
	case 25:
	case 32:
	case 33:	
    case 10:
    case 11:
	case 12:
    case 13:
    case 14:
    case 15:
        break;*/
    default:
    break;
  }
}
void get_mge_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
  switch(num)
  {
    case 00:
	case 01:
	case 02:
    case 03:
	case 04:
	case 05:
	case 06:
	case 07:
	case 8:
    case 9:
	{
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }	
    case 10:
	case 11:
	{
        memcpy(ntfcncde, TYPE_MJ, 2);
        memcpy(almcde, TYPE_MJ_A, 2);
        break;
    }
    /*
    case 03:
    case 04:
    case 05:
    case 06:
    case 07:
    case 08:
    case 09:
	case 16:
	case 17:
	case 24:
	case 25:
	case 32:
	case 33:	
    case 10:
    case 11:
	case 12:
    case 13:
    case 14:
    case 15:
        break;*/
    default:
    break;
  }
}


void get_tod_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
  switch(num)
  {
    case 00:
	case 01:
	case 02:
    case 03:
	{
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }	
   
    /*case 10:
    case 11:
	case 12:
    case 13:
    case 14:
    case 15:
        break;*/
    default:
    break;
  }
}

void get_ntp_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
  switch(num)
  {
    case 00:
	case 01:
	case 02:
    case 03:
	case 04:
	case 05:
	case 06:
    case 07:
	case 8:
	case 9:
	case 10:
    case 11:
	case 12:
	{
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }	
   
    /*case 10:
    case 11:
	case 12:
    case 13:
    case 14:
    case 15:
        break;*/
    default:
    break;
  }
}

void get_MCP_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
    switch(num)
    {
    case 0:
    case 1:
    case 4:
    case 5:
    {
        memcpy(ntfcncde, TYPE_MJ, 2);
        memcpy(almcde, TYPE_MJ_A, 2);
        break;
    }
	case 6:
	{
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }	
    /*case 2:
    case 3:

    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    {
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }*/
    default:
    break;
    }
}

void get_REF_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num)
{
    switch(num)
    {
    case 00:
    case 01:
    case 02:
    case 03:
    case 12:

    case 04:
    case 05:
    case 06:
    case 07:
    case 8:
    case 9:
    case 10:
    case 11:

    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
    case 40:
    case 41:
    case 42:
    case 43:
    case 44:
    case 45:
    case 46:
    case 47:

    {
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        break;
    }
	default:
		break;
    }
}




int get_format(char *format, unsigned char *ctag)
{
    unsigned char tmp[MAXDATASIZE];
    int len = 0;
    memset(tmp, '\0', MAXDATASIZE);
    get_time_fromnet();
    sprintf((char *)tmp, "%c%c%c%c%c%c%s%c", CR, LF, LF, SP, SP, SP, conf_content.slot_u.tid, SP);
    sprintf((char *)tmp, "%s%s%c%s", tmp, mcp_date, SP, mcp_time);
    sprintf((char *)tmp, "%s%c%cM%c%c%s%cCOMPLD%c%c", tmp, CR, LF, SP, SP, ctag, SP, CR, LF);
    len = strlen(tmp);
    memcpy(format, tmp, len);

    return len;
}




void get_alm(char slot, unsigned char *data)
{
    int num, i;

    num = (int)(slot - 'a');
    //printf("<get_alm>:%d\n",num);
    if( (num >= 0) && (num < 23) )
    {
        if( (num == 18) || (num == 19) ) //电源
        {
            if(slot_type[num] != 'O')
            {
               
                sprintf((char *)data, "%s%c%c%c\"%c,M,\"%c%c", data, SP, SP, SP, slot, CR, LF);
            }
        }
        else if(num == 21)
        {

        }
        else
        {
            if(slot_type[num] != 'O')
            { 
                sprintf((char *)data, "%s%c%c%c%s%c%c", data, SP, SP, SP, alm_sta[num], CR, LF);
            }
			//
			//!reftf alarm : @@@@@@ PPPPPP
			//
            if((slot_type[num] == 'S') && (num == 0))
            {
                sprintf((char *)data, "%s%c%c%c%s%c%c;", data, SP, SP, SP, alm_sta[18], CR, LF);
            }
            else if((slot_type[num] == 'S') && (num == 1))
            {
                sprintf((char *)data, "%s%c%c%c%s%c%c;", data, SP, SP, SP, alm_sta[19], CR, LF);
            }
			//
            //!pge4v2 alarm : @@@@@@ PPPPPP
            //
			else if(slot_type[num] == 'j')
            {
                sprintf((char *)data, "%s%c%c%c%s%c%c;", data, SP, SP, SP, alm_sta[PGE4S_ALM5_START+num], CR, LF);
            }
            else 
            {
                sprintf((char *)data, "%s;", data);
            }
          
			
        }
    }
    else if( (slot == ':') || (slot == ' ') )
    {
        for(i = 0; i < 18; i++)
        {
            if(slot_type[i] != 'O')
            {
                if( (slot_type[i] == 'd' ) && (12 == i || 13 == i) )
                {
                    //20140926改变drv在线告警
                    alm_sta[i][5] &= gExtCtx.save.onlineSta;
                }
                else
                {
                    ;
                }
                if(alm_sta[i][0] != '\0')
                	sprintf((char *)data, "%s%c%c%c%s%c%c", data, SP, SP, SP, alm_sta[i], CR, LF);
            }
			//
			//! reftf @@@@@@ PPPPPP
			//
            if((slot_type[i] == 'S') && (i == 0))
            {
                sprintf((char *)data, "%s%c%c%c%s%c%c", data, SP, SP, SP, alm_sta[18], CR, LF);
            }
            else if((slot_type[i] == 'S') && (i == 1))
            {
                sprintf((char *)data, "%s%c%c%c%s%c%c", data, SP, SP, SP, alm_sta[19], CR, LF);
            }
			//
			//! pge4v2 @@@@@@ PPPPPP
			//
			else if(slot_type[i] == 'j')
            {
                sprintf((char *)data, "%s%c%c%c%s%c%c", data, SP, SP, SP, alm_sta[PGE4S_ALM5_START+i], CR, LF);
            }
        }
        if(slot_type[22] != 'O')
        {
            sprintf((char *)data, "%s%c%c%c%s%c%c", data, SP, SP, SP, alm_sta[20], CR, LF);
            sprintf((char *)data, "%s%c%c%c%s%c%c;", data, SP, SP, SP, alm_sta[22], CR, LF);
        }
        else
        {
            sprintf((char *)data, "%s%c%c%c%s%c%c;", data, SP, SP, SP, alm_sta[20], CR, LF);
        }
    }
}







void get_ext_alm(char slot, unsigned char *data)
{
    int i;
    int opt = 0;

    if( (slot == ':') || (slot == ' ') )
    {
        for(i = 0; i < 10; i++)
        {
            if(	(EXT_NONE != gExtCtx.extBid[0][i]) &&
                    (0 != memcmp(gExtCtx.out[i].outAlm, "@@@@@", 5)))
            {
                opt = 1;
                sprintf((char *)data,
                        "%s%c%c%c\"%c,%c,%s\"\r\n",
                        data,
                        SP, SP, SP,
                        i + 'a',
                        gExtCtx.extBid[0][i],
                        gExtCtx.out[i].outAlm);
            }
        }

        if( (EXT_NONE != gExtCtx.extBid[0][10]) &&
                (0 != memcmp(gExtCtx.mgr[0].mgrAlm, "@@@@@@@", 7)))
        {
            opt = 1;
            sprintf((char *)data,
                    "%s%c%c%c\"%c,%c,%s\"\r\n",
                    data,
                    SP, SP, SP,
                    'k',
                    gExtCtx.extBid[0][10],
                    gExtCtx.mgr[0].mgrAlm);
        }

        if( (EXT_NONE != gExtCtx.extBid[0][11]) &&
                (0 != memcmp(gExtCtx.mgr[1].mgrAlm, "@@@@@@@", 7)))
        {
            opt = 1;
            sprintf((char *)data,
                    "%s%c%c%c\"%c,%c,%s\"\r\n",
                    data,
                    SP, SP, SP,
                    'l',
                    gExtCtx.extBid[0][11],
                    gExtCtx.mgr[1].mgrAlm);
        }

        if(0 == opt)
        {
            strcat(data, "   \"\"\r\n");
        }
        strcat(data, "/\r\n");

        opt = 0;
        for(i = 0; i < 10; i++)
        {
            if( (EXT_NONE != gExtCtx.extBid[1][i]) &&
                    (0 != memcmp(gExtCtx.out[10 + i].outAlm, "@@@@@", 5)))
            {
                opt = 1;
                sprintf((char *)data,
                        "%s%c%c%c\"%c,%c,%s\"\r\n",
                        data, SP, SP, SP,
                        i + 'a',
                        gExtCtx.extBid[1][i],
                        gExtCtx.out[10 + i].outAlm);
            }
        }

        if( (EXT_NONE != gExtCtx.extBid[1][10]) &&
                (0 != memcmp(gExtCtx.mgr[2].mgrAlm, "@@@@@@@", 7)))
        {
            opt = 1;
            sprintf((char *)data,
                    "%s%c%c%c\"%c,%c,%s\"\r\n",
                    data, SP, SP, SP, 'k',
                    gExtCtx.extBid[1][10],
                    gExtCtx.mgr[2].mgrAlm);
        }

        if( (EXT_NONE != gExtCtx.extBid[1][11]) &&
                (0 != memcmp(gExtCtx.mgr[3].mgrAlm, "@@@@@@@", 7)))
        {
            opt = 1;
            sprintf((char *)data,
                    "%s%c%c%c\"%c,%c,%s\"\r\n",
                    data, SP, SP, SP, 'l',
                    gExtCtx.extBid[1][11],
                    gExtCtx.mgr[3].mgrAlm);
        }

        if(0 == opt)
        {
            strcat(data, "   \"\"\r\n");
        }
        strcat(data, "/\r\n");

        opt = 0;
        for(i = 0; i < 10; i++)
        {
            if( (EXT_NONE != gExtCtx.extBid[2][i]) &&
                    (0 != memcmp(gExtCtx.out[20 + i].outAlm, "@@@@@", 5)))
            {
                opt = 1;
                sprintf((char *)data,
                        "%s%c%c%c\"%c,%c,%s\"\r\n",
                        data, SP, SP, SP, i + 'a',
                        gExtCtx.extBid[2][i],
                        gExtCtx.out[20 + i].outAlm);
            }
        }

        if( (EXT_NONE != gExtCtx.extBid[2][10]) &&
                (0 != memcmp(gExtCtx.mgr[4].mgrAlm, "@@@@@@@", 7)))
        {
            opt = 1;
            sprintf((char *)data,
                    "%s%c%c%c\"%c,%c,%s\"\r\n",
                    data, SP, SP, SP, 'k',
                    gExtCtx.extBid[2][10],
                    gExtCtx.mgr[4].mgrAlm);
        }

        if( (EXT_NONE != gExtCtx.extBid[2][11]) &&
                (0 != memcmp(gExtCtx.mgr[5].mgrAlm, "@@@@@@@", 7)))
        {
            opt = 1;
            sprintf((char *)data,
                    "%s%c%c%c\"%c,%c,%s\"\r\n",
                    data, SP, SP, SP, 'l',
                    gExtCtx.extBid[2][11],
                    gExtCtx.mgr[5].mgrAlm);
        }

        if(0 == opt)
        {
            strcat(data, "   \"\"\r\n");
        }
        strcat(data, ";");
    }
}







void get_ext1_alm(char slot, unsigned char *data)
{
    int num, i;

    if( (slot == ':') || (slot == ' ') )
    {
        for(i = 0; i < 10; i++)
        {
            sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, i + 'a', gExtCtx.extBid[0][i]);
            if(EXT_NONE != gExtCtx.extBid[0][i])
            {
                sprintf((char *)data, "%s,%s\"%c%c", data, gExtCtx.out[i].outAlm, CR, LF);
            }
            else
            {
                strcat(data, "\"\r\n");
            }
        }

        sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, 'k', gExtCtx.extBid[0][10]);
        if(EXT_NONE != gExtCtx.extBid[0][10])
        {
            sprintf((char *)data, "%s,%s\"%c%c", data, gExtCtx.mgr[0].mgrAlm, CR, LF);
        }
        else
        {
            strcat(data, "\"\r\n");
        }

        sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, 'l', gExtCtx.extBid[0][11]);
        if(EXT_NONE != gExtCtx.extBid[0][11])
        {
            sprintf((char *)data, "%s,%s\"%c%c;", data, gExtCtx.mgr[1].mgrAlm, CR, LF);
        }
        else
        {
            strcat(data, "\"\r\n;");
        }
    }
    else
    {
        num = (int)(slot - 'a');
        if( (num >= 0) && (num <= 9) )
        {
            sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, slot, gExtCtx.extBid[0][num]);
            if(EXT_NONE != gExtCtx.extBid[0][num])
            {
                sprintf((char *)data, "%s,%s\"%c%c;", data, gExtCtx.out[num].outAlm, CR, LF);
            }
            else
            {
                strcat(data, "\"\r\n;");
            }
        }
        else if(10 == num)
        {
            sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, slot, gExtCtx.extBid[0][num]);
            if(EXT_NONE != gExtCtx.extBid[0][num])
            {
                sprintf((char *)data, "%s,%s\"%c%c;", data, gExtCtx.mgr[0].mgrAlm, CR, LF);
            }
            else
            {
                strcat(data, "\"\r\n;");
            }
        }
        else if(11 == num)
        {
            sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, slot, gExtCtx.extBid[0][num]);
            if(EXT_NONE != gExtCtx.extBid[0][num])
            {
                sprintf((char *)data, "%s,%s\"%c%c;", data, gExtCtx.mgr[1].mgrAlm, CR, LF);
            }
            else
            {
                strcat(data, "\"\r\n;");
            }
        }
        else
        {
            //
        }
    }
}







void get_ext2_alm(char slot, unsigned char *data)
{
    int num, i;

    if( (slot == ':') || (slot == ' ') )
    {
        for(i = 0; i < 10; i++)
        {
            sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, i + 'a', gExtCtx.extBid[1][i]);
            if(EXT_NONE != gExtCtx.extBid[1][i])
            {
                sprintf((char *)data, "%s,%s\"%c%c", data, gExtCtx.out[10 + i].outAlm, CR, LF);
            }
            else
            {
                strcat(data, "\"\r\n");
            }
        }

        sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, 'k', gExtCtx.extBid[1][10]);
        if(EXT_NONE != gExtCtx.extBid[1][10])
        {
            sprintf((char *)data, "%s,%s\"%c%c", data, gExtCtx.mgr[2].mgrAlm, CR, LF);
        }
        else
        {
            strcat(data, "\"\r\n");
        }

        sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, 'l', gExtCtx.extBid[1][11]);
        if(EXT_NONE != gExtCtx.extBid[1][11])
        {
            sprintf((char *)data, "%s,%s\"%c%c;", data, gExtCtx.mgr[3].mgrAlm, CR, LF);
        }
        else
        {
            strcat(data, "\"\r\n;");
        }
    }
    else
    {
        num = (int)(slot - 'a');
        if( (num >= 0) && (num <= 9) )
        {
            sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, slot, gExtCtx.extBid[1][num]);
            if(EXT_NONE != gExtCtx.extBid[1][num])
            {
                sprintf((char *)data, "%s,%s\"%c%c;", data, gExtCtx.out[10 + num].outAlm, CR, LF);
            }
            else
            {
                strcat(data, "\"\r\n;");
            }
        }
        else if(10 == num)
        {
            sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, slot, gExtCtx.extBid[1][num]);
            if(EXT_NONE != gExtCtx.extBid[1][num])
            {
                sprintf((char *)data, "%s,%s\"%c%c;", data, gExtCtx.mgr[2].mgrAlm, CR, LF);
            }
            else
            {
                strcat(data, "\"\r\n;");
            }
        }
        else if(11 == num)
        {
            sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, slot, gExtCtx.extBid[1][num]);
            if(EXT_NONE != gExtCtx.extBid[1][num])
            {
                sprintf((char *)data, "%s,%s\"%c%c;", data, gExtCtx.mgr[3].mgrAlm, CR, LF);
            }
            else
            {
                strcat(data, "\"\r\n;");
            }
        }
        else
        {
            //
        }
    }
}







void get_ext3_alm(char slot, unsigned char *data)
{
    int num, i;

    if( (slot == ':') || (slot == ' ') )
    {
        for(i = 0; i < 10; i++)
        {
            sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, i + 'a', gExtCtx.extBid[2][i]);
            if(EXT_NONE != gExtCtx.extBid[2][i])
            {
                sprintf((char *)data, "%s,%s\"%c%c", data, gExtCtx.out[20 + i].outAlm, CR, LF);
            }
            else
            {
                strcat(data, "\"\r\n");
            }
        }

        sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, 'k', gExtCtx.extBid[2][10]);
        if(EXT_NONE != gExtCtx.extBid[2][10])
        {
            sprintf((char *)data, "%s,%s\"%c%c", data, gExtCtx.mgr[4].mgrAlm, CR, LF);
        }
        else
        {
            strcat(data, "\"\r\n");
        }

        sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, 'l', gExtCtx.extBid[2][11]);
        if(EXT_NONE != gExtCtx.extBid[2][11])
        {
            sprintf((char *)data, "%s,%s\"%c%c;", data, gExtCtx.mgr[5].mgrAlm, CR, LF);
        }
        else
        {
            strcat(data, "\"\r\n;");
        }
    }
    else
    {
        num = (int)(slot - 'a');
        if( (num >= 0) && (num <= 9) )
        {
            sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, slot, gExtCtx.extBid[2][num]);
            if(EXT_NONE != gExtCtx.extBid[2][num])
            {
                sprintf((char *)data, "%s,%s\"%c%c;", data, gExtCtx.out[20 + num].outAlm, CR, LF);
            }
            else
            {
                strcat(data, "\"\r\n;");
            }
        }
        else if(10 == num)
        {
            sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, slot, gExtCtx.extBid[2][num]);
            if(EXT_NONE != gExtCtx.extBid[2][num])
            {
                sprintf((char *)data, "%s,%s\"%c%c;", data, gExtCtx.mgr[4].mgrAlm, CR, LF);
            }
            else
            {
                strcat(data, "\"\r\n;");
            }
        }
        else if(11 == num)
        {
            sprintf((char *)data, "%s%c%c%c\"%c,%c", data, SP, SP, SP, slot, gExtCtx.extBid[2][num]);
            if(EXT_NONE != gExtCtx.extBid[2][num])
            {
                sprintf((char *)data, "%s,%s\"%c%c;", data, gExtCtx.mgr[5].mgrAlm, CR, LF);
            }
            else
            {
                strcat(data, "\"\r\n;");
            }
        }
        else
        {
            //
        }
    }
}




/*********************Function:save_one_alm********************/
/*********************Parmeters: slot      *******************/
/*********************Modify:2017/7/19      *******************/


void save_alm(char slot, unsigned char *data, int flag)
{
    int num;
    num = (int)(slot - 'a');
    if(flag == 1) //REF
    {
        memcpy(alm_sta[num + 18], data, strlen(data));
    }
	else if(flag == 2)
	{
	    memcpy(alm_sta[num + PGE4S_ALM5_START], data, strlen(data));
	}
    else if((num >= 0) && (num < 18))
    {
        /*if(0 == num || 1 == num)
        {
            printf("<ref alm %d>:'0x4':%s\n", num, data);
        }*/  
        memcpy(alm_sta[num], data, strlen(data));
    }
    else if(num == 20)
    {
        memcpy(alm_sta[20], data, strlen(data));
    }
    else if(num == 22)
    {
        memcpy(alm_sta[22], data, strlen(data));
    }
#if DEBUG_NET_INFO
    if(print_switch == 0)
    {
        printf("%s:<%d>:%s;\n", NET_INFO, num + 1, data);
    }
#endif
}

int send_pullout_alm(int num, int flag_cl)
{
    unsigned char alm[3];
    unsigned char tmp[50];
    unsigned char ntfcncde[3];
    unsigned char almcde[3];
    alm[0] = ((num & 0x0f) | 0x50); // 0x6x  di 4 wei

    if(flag_cl == 0)
    {
        alm[1] = (((num & 0x70) >> 3) | 0x50);
        alm[2] = '\0';
        memcpy(ntfcncde, TYPE_MN, 2);
        memcpy(almcde, TYPE_MN_A, 2);
        rpt_alm_framing('u', TYPE_MN, TYPE_MN_A, alm);
    }
    else
    {
        alm[1] = (((num & 0x70) >> 3) | 0x51);
        alm[2] = '\0';
        memcpy(ntfcncde, TYPE_CL, 2);
        memcpy(almcde, TYPE_CL_A, 2);
        rpt_alm_framing('u', ntfcncde, almcde, alm);
    }

    get_time_fromnet();
    sprintf((char *)tmp, "\"u:N,%s,%s,%s,%s,%s,NSA\"%c%c%c%c%c",
            mcp_date, mcp_time, almcde, ntfcncde, alm, CR, LF, SP, SP, SP);
    strncpy(saved_alms[saved_alms_cnt], tmp, strlen(tmp));
    saved_alms_cnt++;
    if(saved_alms_cnt == SAVE_ALM_NUM)
    {
        saved_alms_cnt = 0;
    }
    /******/

    return 0;
}

void  send_alm(char slot, int num, char type, int flag_cl)
{

    unsigned char ntfcncde[3];
    unsigned char almcde[3];
    unsigned char alm[3];
    unsigned char tmp[50];
	//unsigned char act_almbit = 0;
    memset(ntfcncde, '\0', 3);
    memset(almcde, '\0', 3);
    memset(alm, '\0', 3);
    memset(tmp, '\0', 50);
    if(type == 'L' ||'e' == type || 'g' == type )
    {
        get_gbtp_alm_type(ntfcncde, almcde, num);
    }
	else if(  'l' == type)
	{
	    get_btp_alm_type(ntfcncde, almcde, num);
	}
	else if( type == 'P')
	{
	    get_gtp_alm_type(ntfcncde, almcde, num);
	}
    //else if(type=='P')
    //	{
    //	get_gbtp_alm_type(ntfcncde,almcde,num);
    //}
    else if(type == 'K' || type == 'R')
    {
        get_rb_alm_type(ntfcncde, almcde, num);
    }
    else if(type == 'J' || type == 'a' || type == 'h' || type == 'i')
    {
        get_OUT16_alm_type(ntfcncde, almcde, num);
    }
    else if(type == 'I')
    {
        get_TP16_alm_type(ntfcncde, almcde, num);
    }
    else if(type == 'S')
    {
        get_REF_alm_type(ntfcncde, almcde, num);
    }
    else if((type < 'I') && (type > '@'))
    {
        get_PTP_alm_type(ntfcncde, almcde, num);
    }
    else if(type == 'Q')
    {
        memcpy(ntfcncde, TYPE_MJ, 2);
        memcpy(almcde, TYPE_MJ_A, 2);
    }
    else if(type == 'N')
    {
        get_MCP_alm_type(ntfcncde, almcde, num);
    }
    else if(type == 'V')
	{
	    get_tod_alm_type(ntfcncde, almcde, num);
    }
	else if(type == 'Z')
	{
	    get_ntp_alm_type(ntfcncde, almcde, num);
    }
    else if(type == 's')
    {
        get_PGEIN_alm_type(ntfcncde, almcde, num);
    }
    else if(type == 'j')
    {
        get_PGE4V_alm_type(ntfcncde, almcde, num);
    }
	else if(type == 'd')
    {
        get_drv_alm_type(ntfcncde, almcde, num);
    }
	else if(type == 'f')
    {
        get_mge_alm_type(ntfcncde, almcde, num);
    }
	else if(type == 'v')
	{
	    get_GBTPIIV2_alm_type(ntfcncde, almcde, num);
	}
    //num=num+1;  0110 Z3Z2Z1Z0 0110 Z7 Z6Z5Z4
    //@@@@@@@@@@@@@@@@@@
    alm[0] = ((num & 0x0f) | 0x50);
    if(flag_cl == 0)
    {
        alm[1] = (((num & 0x70) >> 3) | 0x50);
        memcpy(ntfcncde, TYPE_CL, 2);
        memcpy(almcde, TYPE_CL_A, 2);
    }
    else
    {
        alm[1] = (((num & 0x70) >> 3) | 0x51);
    }

    alm[2] = '\0';
    /******/
    get_time_fromnet();
    sprintf((char *)tmp, "\"%c:%c,%s,%s,%s,%s,%s,NSA\"%c%c%c%c%c",
            slot, type, mcp_date, mcp_time, almcde, ntfcncde, alm, CR, LF, SP, SP, SP);
    strncpy(saved_alms[saved_alms_cnt], tmp, strlen(tmp));
    saved_alms_cnt++;
    if(saved_alms_cnt == SAVE_ALM_NUM)
    {
        saved_alms_cnt = 0;
    }
    /******/
	if(ntfcncde[0] != '\0')
    rpt_alm_framing(slot, ntfcncde, almcde, alm);


}


void rpt_alm_to_client(char slot, unsigned char *data)
{
    int num, i, j, len;
    char tmp, type_flag, temp;
	int star_ind = 0;
    char errflag = 0;
    len = strlen(data);
    num = (int)(slot - 'a');

    type_flag = slot_type[num];
    //
    //屏蔽掉多余告警
    //
    almMsk_cmdRALM(num, data, len);
	
	if(num == 20)
    {
        num = 18;     //mcp
    }
    else if(num == 22)
    {
        num = 19;     //fan
    }
    //
    //!0x04 第一行报警，0x05 第二行报警, 每行24报警
    //
	for(i = 0; i < len; i++)
    {
        if((data[0] >> 4) == 0x04)
        {
            tmp = (data[i] & 0x0f);
            for(j = 0; j < 4; j++)
            {
                temp = ((tmp >> j) & 0x01);
                if((temp == 0x00) && (flag_alm[num][i * 4 + j] == 0x01))
                {
                    /*清除告警*/
                    send_alm(slot, (i * 4 + j), type_flag, 0);
                }
                else if((temp == 0x01) && (flag_alm[num][i * 4 + j] == 0x00))
                {
                    /*上报告警*/
                    send_alm(slot, (i * 4 + j), type_flag, 1);
                }
                flag_alm[num][i * 4 + j] = temp;
            }
        }
        else if((data[0] >> 4) == 0x05) //ref
        {
            //printf("<rpt_alm_to_client>:'ref num':%d\n",num);
            //! flag_alm 20 ref1 21 ref2 (0x50)
			switch(type_flag)
			{
			  case 'S':
			  {
			  	if((num != 1) || (num != 0))
					errflag = 1;
				else
					star_ind = (int)PGE4S_TALM5_START + num - 2;
			  
			  }
			  break;
			  case 'j':
			  {
			  	if((num < 0) || (num > 13))
					errflag = 1;
				else
					star_ind = (int)PGE4S_TALM5_START + num;
			  }
			  break;
			  default:
			  	errflag = 1;
			  break;
			}

			if(errflag == 1)
				return ;

			tmp = (data[i] & 0x0f);
            for(j = 0; j < 4; j++)
            {
                temp = ((tmp >> j) & 0x01);
                if((temp == 0x00) && (flag_alm[star_ind][i * 4 + j] == 0x01))
                {
                    /*清除告警*/
                    send_alm(slot, (i * 4 + j + 24), type_flag, 0);
                }
                else if((temp == 0x01) && (flag_alm[star_ind][i * 4 + j] == 0x00))
                {
                    /*上报告警*/
                    send_alm(slot, (i * 4 + j + 24), type_flag, 1);
                }
                flag_alm[star_ind][i * 4 + j] = temp;
            }
        }
    }

}







void respond_success(int client, unsigned char *ctag)
{
    int len, send_ok;
    unsigned char  sendbuf[MAXDATASIZE];
    memset(sendbuf, '\0', MAXDATASIZE);
    len = get_format(sendbuf, ctag);
    sprintf((char *)sendbuf, "%s;", sendbuf);
    send_ok = send(client, sendbuf, len + 1, MSG_NOSIGNAL);
    if(send_ok == -1)
    {
        /*	close(client);
        	FD_CLR(client, &allset);
        	client= -1;*/
        printf("timeout_<1>!\n");
    }
    if(print_switch == 0)
    {
        printf("##YES,%s\n", ctag);
    }
}

void respond_fail(int client, unsigned char *ctag, int type)
{
    /*	<cr><lf><lf>
    ^^^<sid>^<date>^<time><cr><lf>
    M^^<catg>^DENY<cr><lf>
    ^^^<error code><cr><lf>;*/
    int send_ok, len;
    unsigned char  sendbuf[MAXDATASIZE];
    memset(sendbuf, '\0', MAXDATASIZE);
    get_time_fromnet();
    sprintf((char *)sendbuf, "%c%c%c%c%c%c%s%c", CR, LF, LF, SP, SP, SP, conf_content.slot_u.tid, SP);
    sprintf((char *)sendbuf, "%s%s%c%s%c%c", sendbuf, mcp_date, SP, mcp_time, CR, LF);
    sprintf((char *)sendbuf, "%sM%c%c%s%cDENY%c%c", sendbuf, SP, SP, ctag, SP, CR, LF);
    sprintf((char *)sendbuf, "%s%c%c%c%s%c%c;", sendbuf, SP, SP, SP, g_cmd_fun_ret[type], CR, LF);
    len = strlen(sendbuf);
    send_ok = send(client, sendbuf, len + 1, MSG_NOSIGNAL);
    if(send_ok == -1)
    {
        /*	close(client);
        	FD_CLR(client, &allset);
        	client= -1;*/
        printf("timeout_<2>!\n");
    }
    if(print_switch == 0)
    {
        printf("##NO,%s\n", ctag);
    }
}




void rpt_event_framing(unsigned char *event,
                       char slot,
                       int flag_na,
                       unsigned char *data,
                       int flag_sa,
                       unsigned char *reason)
{
    unsigned char tmp[1024];
    memset(tmp, '\0', 1024);
    get_time_fromnet();
    sprintf((char *)tmp, "%c%c%c%c%c%c%s%c", CR, LF, LF, SP, SP, SP, conf_content.slot_u.tid, SP);
    sprintf((char *)tmp, "%s%s%c%s%c%c", tmp, mcp_date, SP, mcp_time, CR, LF);
    sprintf((char *)tmp, "%sA%c%c%06d%cREPT%cEVNT%c%s%c%c", tmp, SP, SP, atag, SP, SP, SP, event, CR, LF);

	if((event[0]=='C')&&(event[1]=='0')&&(event[2]=='0'))
	{
	   if(conf_content.slot_u.fb == '0')
	   	{
			if(slot == RB1_SLOT)
				sprintf(tmp,"%s%c%c%c\"%c:%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s:\\\"\\\"\"%c%c;",
					tmp,SP,SP,SP,slot,((flag_na==1)?"NA":"CL"),rpt_content.slot_o.rb_ph[0],
					rpt_content.slot_o.rb_ph[1],rpt_content.slot_o.rb_ph[2],rpt_content.slot_o.rb_ph[3],rpt_content.slot_p.rb_ph[4],rpt_content.slot_o.rb_ph[4],
					rpt_content.slot_p.rb_ph[5],rpt_content.slot_o.rb_ph[5],rpt_content.slot_o.rb_ph[6],
					((flag_sa==1)?"SA":"NSA"),reason,CR,LF);
			else if(slot == RB2_SLOT)
				sprintf(tmp,"%s%c%c%c\"%c:%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s:\\\"\\\"\"%c%c;",
					tmp,SP,SP,SP,slot,((flag_na==1)?"NA":"CL"),rpt_content.slot_p.rb_ph[0],
					rpt_content.slot_p.rb_ph[1],rpt_content.slot_p.rb_ph[2],rpt_content.slot_p.rb_ph[3],rpt_content.slot_p.rb_ph[4],rpt_content.slot_o.rb_ph[4],
					rpt_content.slot_p.rb_ph[5],rpt_content.slot_o.rb_ph[5],rpt_content.slot_p.rb_ph[6],
					((flag_sa==1)?"SA":"NSA"),reason,CR,LF);
	   	}
	   else
	   	{
	   	   if(slot == RB1_SLOT)
				sprintf(tmp,"%s%c%c%c\"%c:%s,%s,%s,%s,%s,%s,%s,,,%s,%s,%s:\\\"\\\"\"%c%c;",
					tmp,SP,SP,SP,slot,((flag_na==1)?"NA":"CL"),rpt_content.slot_o.rb_ph[0],
					rpt_content.slot_o.rb_ph[1],rpt_content.slot_o.rb_ph[2],rpt_content.slot_o.rb_ph[3],rpt_content.slot_o.rb_ph[4],
					rpt_content.slot_o.rb_ph[5],rpt_content.slot_o.rb_ph[6],
					((flag_sa==1)?"SA":"NSA"),reason,CR,LF);
		   else if(slot == RB2_SLOT)
				sprintf(tmp,"%s%c%c%c\"%c:%s,%s,%s,%s,%s,%s,%s,,,%s,%s,%s:\\\"\\\"\"%c%c;",
					tmp,SP,SP,SP,slot,((flag_na==1)?"NA":"CL"),rpt_content.slot_p.rb_ph[0],
					rpt_content.slot_p.rb_ph[1],rpt_content.slot_p.rb_ph[2],rpt_content.slot_p.rb_ph[3],rpt_content.slot_p.rb_ph[4],
					rpt_content.slot_p.rb_ph[5],rpt_content.slot_p.rb_ph[6],
					((flag_sa==1)?"SA":"NSA"),reason,CR,LF);
	   	}
		//printf("send client data,%s\n",tmp);
	}
	else
    sprintf((char *)tmp, "%s%c%c%c\"%c:%s,%s,%s,%s:\\\"\\\"\"%c%c;",
            tmp, SP, SP, SP, slot, ((flag_na == 1) ? "NA" : "CL"), data, ((flag_sa == 1) ? "SA" : "NSA"), reason, CR, LF);

    sendto_all_client(tmp);
    atag++;
    if(atag >= 0xffff)
    {
        atag = 0;
    }
}





/*
<cr><lf><lf>
^^^<sid>^<date>^<time><cr><lf>
<almcde>^<atag>^REPT^EVNT^<event><cr><lf>
^^^”<aid>:<ntfcncde>,<condtype>,<serveff>,<reason>:\”<conddescr>\””<cr><lf>
;
   	<ntfcncde>::=NA|CL
	<serveff>::=SA|NSA
   LT00000000000002 2011-11-19 10:27:41
A  003366 REPT EVNT C91
   "l:NA,,NSA,AUTO:\"\""
;
*/

void rpt_alm_framing(char slot, unsigned char *ntfcncde,
                     unsigned char *almcde, unsigned char *alm)
{
    /*<cr><lf><lf>
    ^^^<sid>^<date>^<time><cr><lf>
    <almcde>^<atag>^REPT^ALRM<cr><lf>
    ^^^”<aid>:<ntfcncde>,<condtype>,<serveff>:\”<conddescr>\””<cr><lf>
    ;*/
    /*<almcde>::=*C|**|*^|A^   对应的<ctfcncde>格式为：
    <ntfcncde>::=CR|MJ|MN|CL
    分别表示严重告警、主要告警、次要告警及告警清除信息；*/
    /*
       LT00000000000002 2011-11-19 10:36:34
    *  003412 REPT ALRM
    "g:MN,co,NSA:\"\""
    ;
    */
    unsigned char tmp[1024];

    memset(tmp, '\0', 1024);
    get_time_fromnet();
    sprintf((char *)tmp, "%c%c%c%c%c%c%s%c", CR, LF, LF, SP, SP, SP, conf_content.slot_u.tid, SP);
    sprintf((char *)tmp, "%s%s%c%s%c%c", tmp, mcp_date, SP, mcp_time, CR, LF);
    sprintf((char *)tmp, "%s%s%c%06d%cREPT%cALRM%c%c%c", tmp, almcde, SP, atag, SP, SP, SP, CR, LF);
    sprintf((char *)tmp, "%s%c%c%c\"%c:%s,%s,NSA:\\\"\\\"\"%c%c;",
            tmp, SP, SP, SP, slot, ntfcncde, alm, CR, LF);

    sendto_all_client(tmp);
    atag++;
    if(atag >= 0xffff)
    {
        atag = 0;
    }

}

void send_online_framing(int num, char type)
{
    /*<cr><lf><lf>
    ^^^<sid>^<date>^<time><cr><lf>
    <almcde>^<atag>^REPT^EVNT^<event><cr><lf>
    ^^^”<aid>:<ntfcncde>,<condtype>,<serveff>,<reason>:\”<conddescr>\””<cr><lf>
    ;

    LT00000000000002 2011-11-19 10:27:45
    A  003367 REPT EVNT C91
    "m:NA,O,NSA,AUTO:\"\""
    ;


    */

    //	unsigned char  *tmp=(unsigned char *) malloc ( sizeof(unsigned char) * 1024 );
    unsigned char tmp[1024];
    if(num > 24 || num < 0)
    {
        return ;
    }
    memset(tmp, '\0', 1024);
    get_time_fromnet();
    sprintf((char *)tmp, "%c%c%c%c%c%c%s%c", CR, LF, LF, SP, SP, SP, conf_content.slot_u.tid, SP);
    sprintf((char *)tmp, "%s%s%c%s%c%c", tmp, mcp_date, SP, mcp_time, CR, LF);
    sprintf((char *)tmp, "%sA%c%c%06d%cREPT%cEVNT%cC91%c%c", tmp, SP, SP, atag, SP, SP, SP, CR, LF);
    sprintf((char *)tmp, "%s%c%c%c\"%c:NA,%c,NSA,AUTO:\\\"\\\"\"%c%c;",
            tmp, SP, SP, SP, (char)(num + 0x61), type, CR, LF);

    sendto_all_client(tmp);
    atag++;
    if(atag >= 0xffff)
    {
        atag = 0;
    }
    //	free(tmp);
    //	tmp=NULL;


}



void sendto_all_client(unsigned char *buf)
{
    int count, len, i, send_ok;
    int client_num[MAX_CLIENT];

    len = strlen_r(buf, ';') + 1;
    count = judge_client_online(client_num);

    for(i = 0; i < count; i++)
    {
        send_ok = send(client_fd[client_num[i]], buf, len, MSG_NOSIGNAL);

        //printf("\n\n%s SEND: %s\n\n", __func__, buf);

        if(send_ok == -1)
        {
            printf("-----------------timeout!\n");
        }
    }
}





void sendtodown(unsigned char *buf, unsigned char slot)
{
    struct mymsgbuf msgbuffer;
    int msglen;

    memset(&msgbuffer, '\0', sizeof(struct mymsgbuf));
    msgbuffer.msgtype = 4;
    msgbuffer.slot = slot - 0x60;
    strcpy(msgbuffer.ctrlstring, buf);
    msgbuffer.len = strlen(msgbuffer.ctrlstring);
    memcpy(msgbuffer.ctag, "000000", 6);
    msglen = sizeof(msgbuffer) - 4;

    if((msgsnd (qid, &msgbuffer, msglen, 0)) == -1)
    {
        perror("msgget error<1>!\n");
        printf("errno=%d\n", errno);
    }
    if(print_switch == 0)
    {
        printf("UPMSG>%s<>%d<>%s<\n", msgbuffer.ctrlstring, msgbuffer.len, msgbuffer.ctag);
    }
}





void sendtodown_cli(unsigned char *buf, unsigned char slot, unsigned char *ctag)
{
    struct mymsgbuf msgbuffer;
    int msglen;

    memset(&msgbuffer, '\0', sizeof(struct mymsgbuf));
    msgbuffer.msgtype = 4;
    msgbuffer.slot = slot - 0x60;
    strcpy(msgbuffer.ctrlstring, buf);
    msgbuffer.len = strlen(msgbuffer.ctrlstring);
    memcpy (msgbuffer.ctag, ctag, 6);
    msglen = sizeof(msgbuffer) - 4;

    if((msgsnd (qid, &msgbuffer, msglen, 0)) == -1)
    {
        perror ("msgget error<2>!\n");
        printf("errno=%d\n", errno);
    }
    if(print_switch == 0)
    {
        printf("UPMSG>%s<>%d<>%s<\n", msgbuffer.ctrlstring, msgbuffer.len, msgbuffer.ctag);
    }
}









/*
  1	成功
  0	失败
*/
int ext_issue_cmd(u8_t *buf, u16_t len, u8_t slot, u8_t *ctag)
{
    struct mymsgbuf msgbuffer;
    int msglen;

    memset(&msgbuffer, '\0', sizeof(struct mymsgbuf));
    msgbuffer.msgtype = 4;
    memcpy(msgbuffer.ctrlstring, buf, len);
    msgbuffer.len = len;
    memcpy(msgbuffer.ctag, ctag, 6);
    msglen = sizeof(msgbuffer) - 4;

    if( (EXT_DRV == slot_type[12]) &&
            (slot >= EXT_1_OUT_LOWER && slot <= EXT_1_OUT_UPPER) &&
            (1 == gExtCtx.drv[0].drvPR[0]) )
    {
        msgbuffer.slot = 13;
        if(-1 == (msgsnd(qid, &msgbuffer, msglen, 0)))
        {
            return 0;
        }
    }

    else if((EXT_DRV == slot_type[12]) &&
            (slot >= EXT_2_OUT_LOWER && slot <= EXT_2_OUT_UPPER) &&
            (1 == gExtCtx.drv[0].drvPR[1]) )
    {
        msgbuffer.slot = 13;
        if(-1 == (msgsnd(qid, &msgbuffer, msglen, 0)))
        {
            return 0;
        }
    }

    else if((EXT_DRV == slot_type[12]) &&
            (slot >= EXT_3_OUT_LOWER && slot <= EXT_3_OUT_UPPER && slot != ':' && slot != ';') &&
            (1 == gExtCtx.drv[0].drvPR[2]) )
    {
        msgbuffer.slot = 13;
        if(-1 == (msgsnd(qid, &msgbuffer, msglen, 0)))
        {
            return 0;
        }
    }

    else if((EXT_DRV == slot_type[13]) &&
            (slot >= EXT_1_OUT_LOWER && slot <= EXT_1_OUT_UPPER) &&
            (1 == gExtCtx.drv[1].drvPR[0]) )
    {
        msgbuffer.slot = 14;
        if(-1 == (msgsnd(qid, &msgbuffer, msglen, 0)))
        {
            return 0;
        }
    }

    else if((EXT_DRV == slot_type[13]) &&
            (slot >= EXT_2_OUT_LOWER && slot <= EXT_2_OUT_UPPER) &&
            (1 == gExtCtx.drv[1].drvPR[1]) )
    {
        msgbuffer.slot = 14;
        if(-1 == (msgsnd(qid, &msgbuffer, msglen, 0)))
        {
            return 0;
        }
    }

    else if((EXT_DRV == slot_type[13]) &&
            (slot >= EXT_3_OUT_LOWER && slot <= EXT_3_OUT_UPPER && slot != ':' && slot != ';') &&
            (1 == gExtCtx.drv[1].drvPR[2]) )
    {
        msgbuffer.slot = 14;
        if(-1 == (msgsnd(qid, &msgbuffer, msglen, 0)))
        {
            return 0;
        }
    }
    else
    {
        //do nothing
    }

    if(print_switch == 0)
    {
        printf("UPMSG>%s<>%d<>%s<\n", msgbuffer.ctrlstring, msgbuffer.len, msgbuffer.ctag);
    }

    return 1;
}









int get_freq_lev(int lev)//快捕频率的时候   降一等级
{
    switch (lev)
    {
    case 0x00:
        lev = 0x00;
        break;
    case 0x02:
        lev = 0x04;
        break;
    case 0x04:
        lev = 0x08;
        break;
    case 0x08:
        lev = 0x0b;
        break;
    case 0x0b:
        lev = 0x0f;
        break;
    case 0x0f:
        lev = 0x0f;
        break;
    default:
        lev = 0x0f;
        break;
    }
    return lev;
}

int get_out_now_lev(unsigned char *parg)
{
    int lev;
    if(strncmp1(parg, "00", 2) == 0)
    {
        lev = 0;
    }
    else if(strncmp1(parg, "02", 2) == 0)
    {
        lev = 0x02;
    }
    else if(strncmp1(parg, "04", 2) == 0)
    {
        lev = 0x04;
    }
    else if(strncmp1(parg, "08", 2) == 0)
    {
        lev = 0x08;
    }
    else if(strncmp1(parg, "0B", 2) == 0)
    {
        lev = 0x0b;
    }
    else if(strncmp1(parg, "0F", 2) == 0)
    {
        lev = 0x0f;
    }
    else
    {
        lev = 0x0f;
    }
    return lev;
}

void get_out_now_lev_char(int lev )
{
    if(lev == 0x00)
    {
        memcpy(conf_content.slot_u.tl, "00", 2);
    }
    else if(lev == 0x02)
    {
        memcpy(conf_content.slot_u.tl, "02", 2);
    }
    else if(lev == 0x04)
    {
        memcpy(conf_content.slot_u.tl, "04", 2);
    }
    else if(lev == 0x08)
    {
        memcpy(conf_content.slot_u.tl, "08", 2);
    }
    else if(lev == 0x0b)
    {
        memcpy(conf_content.slot_u.tl, "0B", 2);
    }
    else if(lev == 0x0f)
    {
        memcpy(conf_content.slot_u.tl, "0F", 2);
    }
    else if(lev == 0xff)
    {
        memcpy(conf_content.slot_u.tl, "FF", 2);
    }
    else
    {
        memcpy(conf_content.slot_u.tl, "00", 2);
    }
}

void get_out_lev(int num, int flag)
{
    int flag_lev = 0;
    //int tmp_lev=0;
    int c_port = 0;
    int i;
    int tmp1, tmp2;
    unsigned char alm[3];
    unsigned short alm_num = 47;
    unsigned char ntfcncde[3];
    unsigned char almcde[3];
	unsigned char rled_en = 0;
	unsigned char yled_en = 0;

    if(strncmp1(rpt_content.slot_o.rb_msmode, "MAIN", 4) == 0)
    {
        if(strncmp1(rpt_content.slot_o.rb_trmode, "FREE", 4) == 0)
        {
            flag_lev = 0x0f;
            rled_en  = 0x01;
        }
        else if(strncmp1(rpt_content.slot_o.rb_trmode, "HOLD", 4) == 0)
        {
            yled_en = 0x01;
   
            if(slot_type[14] == 'K')
            {
                flag_lev = 0x04;
            }
            else if(slot_type[14] == 'R')
            {
                flag_lev = 0x0b;
            }
        }
        else if((strncmp1(rpt_content.slot_o.rb_trmode, "FATR", 4) == 0)
                || (strncmp1(rpt_content.slot_o.rb_trmode, "TRCK", 4) == 0))
        {
            yled_en = 0x01;
            i = rpt_content.slot_o.rb_ref - 0x31;
            if((i >= 0) && (i < 4)) 
            {
                if(strncmp1(rpt_content.slot_o.rb_trmode, "FATR", 4) == 0)
                {
                    flag_lev = 0x04;
                }
                else
                {
                    flag_lev = 0x02;
                }
            }
            else if((i > 3) && (i < 8)) 
            {
                if(conf_content.slot_u.fb == '0')
                {
                    if(i == 4)/* INP-5、INP-6，分别代表RB1、RB2盘的2Mhz输入信号
											INP-7、INP-8，分别代表RB1、RB2盘的2Mbit输入信号
											2hz下发的时钟等级已知，只需转发SSM字节
											*/
                    {
                        //读取寄存器 获取TL


                    }
                    else if(i == 5)
                    {
                        /*NULL*/
                        flag_lev = get_ref_tl(rpt_content.slot_o.rb_tl_2mhz);
                        if(strncmp1(rpt_content.slot_o.rb_trmode, "FATR", 4) == 0)
                        {
                            flag_lev = get_freq_lev(flag_lev);
                        }
                    }
                    else if(i == 6)
                    {

                    }
                    else if(i == 7)
                    {
                        /*NULL*/
                        flag_lev = get_ref_tl(rpt_content.slot_o.rb_tl);
                        if(strncmp1(rpt_content.slot_o.rb_trmode, "FATR", 4) == 0)
                        {
                            flag_lev = get_freq_lev(flag_lev);
                        }
                    }

                }
                else if (conf_content.slot_u.fb == '1') //后面板
                {
                    if(i == 4) //ref1
                    {
                        if((rpt_content.slot[0].ref_mod1[2] > '0') && (rpt_content.slot[0].ref_mod1[2] < '9'))
                        {
                            c_port = rpt_content.slot[0].ref_mod1[2] - 0x31;
                            flag_lev = get_ref_tl(rpt_content.slot[0].ref_tl[c_port]);
                            if(strncmp1(rpt_content.slot_o.rb_trmode, "FATR", 4) == 0)
                            {
                                flag_lev = get_freq_lev(flag_lev);
                            }

                            if('1' == conf_content.slot_u.out_ssm_en)
                            {
                                tmp1 = flag_lev;
                                if(tmp1 == 0)
                                {
                                    tmp1 = 16;
                                }

                                tmp2 = conf_content.slot_u.out_ssm_oth;
                                if(tmp2 == 0)
                                {
                                    tmp2 = 16;
                                }

                                if(tmp1 > tmp2)
                                {
                                    ssm_alm_curr = 1;// 1表示有告警
                                }
                                else
                                {
                                    ssm_alm_curr = 0;
                                }
                                //SSM门限 告警产生
                                if(0 == ssm_alm_last && 1 == ssm_alm_curr)
                                {
                                    ssm_alm_last = ssm_alm_curr;
                                    alm[0] = ((alm_num & 0x0f) | 0x60);
                                    alm[1] = (((alm_num & 0x30) >> 3) | 0x69);
                                    alm[2] = '\0';
                                    memcpy(ntfcncde, TYPE_MN, 2);
                                    ntfcncde[2] = '\0';
                                    memcpy(almcde, TYPE_MN_A, 2);
                                    almcde[2] = '\0';
                                    rpt_alm_framing('u', ntfcncde, almcde, alm);
									
                                }
                                //SSM门限 告警清除
                                if(1 == ssm_alm_last && 0 == ssm_alm_curr)
                                {
                                    ssm_alm_last = ssm_alm_curr;
                                    alm[0] = ((alm_num & 0x0f) | 0x60);
                                    alm[1] = (((alm_num & 0x30) >> 3) | 0x68);
                                    alm[2] = '\0';
                                    memcpy(ntfcncde, TYPE_CL, 2);
                                    ntfcncde[2] = '\0';
                                    memcpy(almcde, TYPE_CL_A, 2);
                                    almcde[2] = '\0';
                                    rpt_alm_framing('u', ntfcncde, almcde, alm);
                                }
                            }
                            else
                            {
                                flag_lev = 0xff;
                                //SSM闭塞时，如果已经产生SSM门限告警，需要清除此告警
                                if(1 == ssm_alm_curr)
                                {
                                    ssm_alm_curr = 0;
                                    alm[0] = ((alm_num & 0x0f) | 0x60);
                                    alm[1] = (((alm_num & 0x30) >> 3) | 0x68);
                                    alm[2] = '\0';
                                    memcpy(ntfcncde, TYPE_CL, 2);
                                    ntfcncde[2] = '\0';
                                    memcpy(almcde, TYPE_CL_A, 2);
                                    almcde[2] = '\0';
                                    rpt_alm_framing('u', ntfcncde, almcde, alm);
                                }
                            }
                        }
                    }
                    else if(i == 5) //ref2
                    {
                        if((rpt_content.slot[1].ref_mod1[2] > '0') && (rpt_content.slot[1].ref_mod1[2] < '9'))
                        {
                            c_port = rpt_content.slot[1].ref_mod1[2] - 0x31;
                            flag_lev = get_ref_tl(rpt_content.slot[1].ref_tl[c_port]);
                            if(strncmp1(rpt_content.slot_o.rb_trmode, "FATR", 4) == 0)
                            {
                                flag_lev = get_freq_lev(flag_lev);
                            }

                            if('1' == conf_content.slot_u.out_ssm_en)
                            {
                                tmp1 = flag_lev;
                                if(tmp1 == 0)
                                {
                                    tmp1 = 16;
                                }

                                tmp2 = conf_content.slot_u.out_ssm_oth;
                                if(tmp2 == 0)
                                {
                                    tmp2 = 16;
                                }

                                if(tmp1 > tmp2)
                                {
                                    ssm_alm_curr = 1;// 1表示有告警
                                }
                                else
                                {
                                    ssm_alm_curr = 0;
                                }
                                //SSM门限 告警产生
                                if(0 == ssm_alm_last && 1 == ssm_alm_curr)
                                {
                                    ssm_alm_last = ssm_alm_curr;
                                    alm[0] = ((alm_num & 0x0f) | 0x60);
                                    alm[1] = (((alm_num & 0x30) >> 3) | 0x69);
                                    alm[2] = '\0';
                                    memcpy(ntfcncde, TYPE_MN, 2);
                                    ntfcncde[2] = '\0';
                                    memcpy(almcde, TYPE_MN_A, 2);
                                    almcde[2] = '\0';
                                    rpt_alm_framing('u', ntfcncde, almcde, alm);
                                }
                                //SSM门限 告警清除
                                if(1 == ssm_alm_last && 0 == ssm_alm_curr)
                                {
                                    ssm_alm_last = ssm_alm_curr;
                                    alm[0] = ((alm_num & 0x0f) | 0x60);
                                    alm[1] = (((alm_num & 0x30) >> 3) | 0x68);
                                    alm[2] = '\0';
                                    memcpy(ntfcncde, TYPE_CL, 2);
                                    ntfcncde[2] = '\0';
                                    memcpy(almcde, TYPE_CL_A, 2);
                                    almcde[2] = '\0';
                                    rpt_alm_framing('u', ntfcncde, almcde, alm);
                                }
                            }
                            else
                            {
                                flag_lev = 0xff;
                                //SSM闭塞时，如果已经产生SSM门限告警，需要清除此告警
                                if(1 == ssm_alm_curr)
                                {
                                    ssm_alm_curr = 0;
                                    alm[0] = ((alm_num & 0x0f) | 0x60);
                                    alm[1] = (((alm_num & 0x30) >> 3) | 0x68);
                                    alm[2] = '\0';
                                    memcpy(ntfcncde, TYPE_CL, 2);
                                    ntfcncde[2] = '\0';
                                    memcpy(almcde, TYPE_CL_A, 2);
                                    almcde[2] = '\0';
                                    rpt_alm_framing('u', ntfcncde, almcde, alm);
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                printf("ref change %c\n", rpt_content.slot_o.rb_ref);
                return;
            }

        }
		else
		{
		    printf("clock status unknow %s\n",rpt_content.slot_o.rb_trmode);
		    return;
		}

    }
    else if(strncmp1(rpt_content.slot_p.rb_msmode, "MAIN", 4) == 0)
    {
        if(strncmp1(rpt_content.slot_p.rb_trmode, "FREE", 4) == 0)
        {
            rled_en = 0x01;
            flag_lev = 0x0f;
        }
        else if(strncmp1(rpt_content.slot_p.rb_trmode, "HOLD", 4) == 0)
        {
            yled_en = 0x01;
            if(slot_type[15] == 'K')
            {
                flag_lev = 0x04;
            }
            else if(slot_type[15] == 'R')
            {
                flag_lev = 0x0b;
            }
        }
        else if((strncmp1(rpt_content.slot_p.rb_trmode, "FATR", 4) == 0)
                || (strncmp1(rpt_content.slot_p.rb_trmode, "TRCK", 4) == 0))
        {
            yled_en = 0x01;
            i = rpt_content.slot_p.rb_ref - 0x31;
            if((i >= 0) && (i < 4)) //时间
            {
                if(strncmp1(rpt_content.slot_p.rb_trmode, "FATR", 4) == 0)
                {
                    flag_lev = 0x04;
                }
                else
                {
                    flag_lev = 0x02;
                }
            }
            else if((i > 3) && (i < 8)) //pinglv
            {
                if(conf_content.slot_u.fb == '0')
                {
                    if(i == 4)
                    {
                        flag_lev = get_ref_tl(rpt_content.slot_p.rb_tl_2mhz);
                        if(strncmp1(rpt_content.slot_p.rb_trmode, "FATR", 4) == 0)
                        {
                            flag_lev = get_freq_lev(flag_lev);
                        }
                    }
                    else if(i == 5)
                    {

                    }
                    else if(i == 6)
                    {
                        flag_lev = get_ref_tl(rpt_content.slot_p.rb_tl);
                        if(strncmp1(rpt_content.slot_p.rb_trmode, "FATR", 4) == 0)
                        {
                            flag_lev = get_freq_lev(flag_lev);
                        }
                    }
                    else if(i == 7)
                    {

                    }
                }
                else if(conf_content.slot_u.fb == '1') //后面板
                {
                    if(i == 4) //ref1
                    {
                        if((rpt_content.slot[0].ref_mod1[2] > '0') && (rpt_content.slot[0].ref_mod1[2] < '9'))
                        {
                            c_port = rpt_content.slot[0].ref_mod1[2] - 0x31;
                            flag_lev = get_ref_tl(rpt_content.slot[0].ref_tl[c_port]);
                            if(strncmp1(rpt_content.slot_p.rb_trmode, "FATR", 4) == 0)
                            {
                                flag_lev = get_freq_lev(flag_lev);
                            }

                            if('1' == conf_content.slot_u.out_ssm_en)
                            {
                                tmp1 = flag_lev;
                                if(tmp1 == 0)
                                {
                                    tmp1 = 16;
                                }

                                tmp2 = conf_content.slot_u.out_ssm_oth;
                                if(tmp2 == 0)
                                {
                                    tmp2 = 16;
                                }

                                if(tmp1 > tmp2)
                                {
                                    ssm_alm_curr = 1;// 1表示有告警
                                }
                                else
                                {
                                    ssm_alm_curr = 0;
                                }
                                //SSM门限 告警产生
                                if(0 == ssm_alm_last && 1 == ssm_alm_curr)
                                {
                                    ssm_alm_last = ssm_alm_curr;
                                    alm[0] = ((alm_num & 0x0f) | 0x60);
                                    alm[1] = (((alm_num & 0x30) >> 3) | 0x69);
                                    alm[2] = '\0';
                                    memcpy(ntfcncde, TYPE_MN, 2);
                                    ntfcncde[2] = '\0';
                                    memcpy(almcde, TYPE_MN_A, 2);
                                    almcde[2] = '\0';
                                    rpt_alm_framing('u', ntfcncde, almcde, alm);
                                }
                                //SSM门限 告警清除
                                if(1 == ssm_alm_last && 0 == ssm_alm_curr)
                                {
                                    ssm_alm_last = ssm_alm_curr;
                                    alm[0] = ((alm_num & 0x0f) | 0x60);
                                    alm[1] = (((alm_num & 0x30) >> 3) | 0x68);
                                    alm[2] = '\0';
                                    memcpy(ntfcncde, TYPE_CL, 2);
                                    ntfcncde[2] = '\0';
                                    memcpy(almcde, TYPE_CL_A, 2);
                                    almcde[2] = '\0';
                                    rpt_alm_framing('u', ntfcncde, almcde, alm);
                                }
                            }
                            else
                            {
                                flag_lev = 0xff;
                                //SSM闭塞时，如果已经产生SSM门限告警，需要清除此告警
                                if(1 == ssm_alm_curr)
                                {
                                    ssm_alm_curr = 0;
                                    alm[0] = ((alm_num & 0x0f) | 0x60);
                                    alm[1] = (((alm_num & 0x30) >> 3) | 0x68);
                                    alm[2] = '\0';
                                    memcpy(ntfcncde, TYPE_CL, 2);
                                    ntfcncde[2] = '\0';
                                    memcpy(almcde, TYPE_CL_A, 2);
                                    almcde[2] = '\0';
                                    rpt_alm_framing('u', ntfcncde, almcde, alm);
                                }
                            }
                        }
                    }
                    else if(i == 5) //ref2
                    {
                        if((rpt_content.slot[1].ref_mod1[2] > '0') && (rpt_content.slot[1].ref_mod1[2] < '9'))
                        {
                            c_port = rpt_content.slot[1].ref_mod1[2] - 0x31;
                            flag_lev = get_ref_tl(rpt_content.slot[1].ref_tl[c_port]);
                            if(strncmp1(rpt_content.slot_p.rb_trmode, "FATR", 4) == 0)
                            {
                                flag_lev = get_freq_lev(flag_lev);
                            }

                            if('1' == conf_content.slot_u.out_ssm_en)
                            {
                                tmp1 = flag_lev;
                                if(tmp1 == 0)
                                {
                                    tmp1 = 16;
                                }

                                tmp2 = conf_content.slot_u.out_ssm_oth;
                                if(tmp2 == 0)
                                {
                                    tmp2 = 16;
                                }

                                if(tmp1 > tmp2)
                                {
                                    ssm_alm_curr = 1;// 1表示有告警
                                }
                                else
                                {
                                    ssm_alm_curr = 0;
                                }
                                //SSM门限 告警产生
                                if(0 == ssm_alm_last && 1 == ssm_alm_curr)
                                {
                                    ssm_alm_last = ssm_alm_curr;
                                    alm[0] = ((alm_num & 0x0f) | 0x60);
                                    alm[1] = (((alm_num & 0x30) >> 3) | 0x69);
                                    alm[2] = '\0';
                                    memcpy(ntfcncde, TYPE_MN, 2);
                                    ntfcncde[2] = '\0';
                                    memcpy(almcde, TYPE_MN_A, 2);
                                    almcde[2] = '\0';
                                    rpt_alm_framing('u', ntfcncde, almcde, alm);
                                }
                                //SSM门限 告警清除
                                if(1 == ssm_alm_last && 0 == ssm_alm_curr)
                                {
                                    ssm_alm_last = ssm_alm_curr;
                                    alm[0] = ((alm_num & 0x0f) | 0x60);
                                    alm[1] = (((alm_num & 0x30) >> 3) | 0x68);
                                    alm[2] = '\0';
                                    memcpy(ntfcncde, TYPE_CL, 2);
                                    ntfcncde[2] = '\0';
                                    memcpy(almcde, TYPE_CL_A, 2);
                                    almcde[2] = '\0';
                                    rpt_alm_framing('u', ntfcncde, almcde, alm);
                                }
                            }
                            else
                            {
                                flag_lev = 0xff;
                                //SSM闭塞时，如果已经产生SSM门限告警，需要清除此告警
                                if(1 == ssm_alm_curr)
                                {
                                    ssm_alm_curr = 0;
                                    alm[0] = ((alm_num & 0x0f) | 0x60);
                                    alm[1] = (((alm_num & 0x30) >> 3) | 0x68);
                                    alm[2] = '\0';
                                    memcpy(ntfcncde, TYPE_CL, 2);
                                    ntfcncde[2] = '\0';
                                    memcpy(almcde, TYPE_CL_A, 2);
                                    almcde[2] = '\0';
                                    rpt_alm_framing('u', ntfcncde, almcde, alm);
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                 printf("ref change %c\n", rpt_content.slot_p.rb_ref);
                 return;
          
            }

        }
		else
		{ 
		   printf("clock status unknow %s\n",rpt_content.slot_p.rb_trmode);
		   return;
		}

    }
	else
	{
	  return;
	}

	if(strncmp1(rpt_content.slot_p.rb_msmode, "STBY", 4) == 0)
	{
	     if(strncmp1(rpt_content.slot_p.rb_trmode, "FREE", 4) == 0)
	        rled_en = 0x01;
	     else 
	        yled_en = 0x01;
	
	}
	else if(strncmp1(rpt_content.slot_o.rb_msmode, "STBY", 4) == 0)
	{
	     if(strncmp1(rpt_content.slot_p.rb_trmode, "FREE", 4) == 0)
	        rled_en = 0x01;
	     
	      else
		  	yled_en = 0x01;
	}
	if (rled_en == 0x01)
	{
	   FpgaWrite(ALMControl, 0xfff9);
	   FpgaWrite(IndicateLED,0x0001);
	}
	else
	{
	   FpgaWrite(ALMControl, 0xfffe);
	   FpgaWrite(IndicateLED,0x0002);
	}

    get_out_now_lev_char(flag_lev);

    if(flag == 1) //初始化，单独对一个单盘下发
    {
        send_out_lev(flag_lev, num);
		printf("singal out len slot=%d,lev=%d\n", num, flag_lev);
    }
    else
    {
        if(flag_lev != g_lev)
        {
            g_lev = flag_lev;

            for(i = 0; i < 14; i++)
            {
                //g_lev = get_out_now_lev(rpt_content.slot[i].out_lev);
                if( (slot_type[i] == 'J') ||
                        (slot_type[i] == 'a') ||
                        (slot_type[i] == 'b') ||
                        (slot_type[i] == 'c') ||
                        (slot_type[i] == 'h') ||
                        (slot_type[i] == 'i'))
                {
                    send_out_lev(flag_lev, i);
					printf("all out len slot=%d,lev=%d\n", i, flag_lev);
                }
            }

            //扩展框 时钟等级
            ext_ssm_cfg(conf_content.slot_u.tl);
        }
    }
}



void send_out_lev(int flag_lev, int num)
{
    unsigned char sendbuf[SENDBUFSIZE];
    memset(sendbuf, '\0', SENDBUFSIZE);

    //printf("-------------in send  -------lev=%02x\n",flag_lev);
    switch(flag_lev)
    {
    case 0x00:
    {
        sprintf((char *)sendbuf, "SET-OUT-LEV:000000::00;");
        sendtodown(sendbuf, (char)('a' + num));
        break;
    }
    case 0x02:
    {
        sprintf((char *)sendbuf, "SET-OUT-LEV:000000::02;");
        sendtodown(sendbuf, (char)('a' + num));
        break;
    }
    case 0x04:
    {
        sprintf((char *)sendbuf, "SET-OUT-LEV:000000::04;");
        sendtodown(sendbuf, (char)('a' + num));
        break;
    }
    case 0x08:
    {
        sprintf((char *)sendbuf, "SET-OUT-LEV:000000::08;");
        sendtodown(sendbuf, (char)('a' + num));
        break;
    }
    case 0x0b:
    {
        sprintf((char *)sendbuf, "SET-OUT-LEV:000000::0B;");
        sendtodown(sendbuf, (char)('a' + num));
        break;
    }
    case 0x0f:
    {
        sprintf((char *)sendbuf, "SET-OUT-LEV:000000::0F;");
        sendtodown(sendbuf, (char)('a' + num));
        break;
    }
    case 0xff:
    {
        sprintf((char *)sendbuf, "SET-OUT-LEV:000000::FF;");
        sendtodown(sendbuf, (char)('a' + num));
        break;
    }

    default:
    {
        break;
    }

    }

}

int get_ref_tl(unsigned char c)
{
    int lev = 0;

    if(c == 'a')
    {
        lev = 0x02;
    }
    else if(c == 'b')
    {
        lev = 0x04;
    }
    else if(c == 'c')
    {
        lev = 0x08;
    }
    else if(c == 'd')
    {
        lev = 0x0b;
    }
    else if(c == 'e')
    {
        lev = 0x0f;
    }
    else if(c == 'f')
    {
        lev = 0x00;
    }
    else if(c == 'g')
    {
        lev = 0x00;
    }

    return lev;
}



int send_ref_lev(int num)
{
    int c_port = 0;
    int c_tl = 0;
    int i;
    char ext_ssm[3];

    if((num != 0) && (num != 1))
    {
        return 1;
    }
    if(strncmp1(rpt_content.slot_o.rb_msmode, "MAIN", 4) == 0)
    {
        if((rpt_content.slot_o.rb_ref == '7') && ((num == 0)))
        {
            //先选择跟踪源 路数
            if((rpt_content.slot[0].ref_mod1[2] > '0') && (rpt_content.slot[0].ref_mod1[2] < '9'))
            {
                c_port = rpt_content.slot[0].ref_mod1[2] - 0x31;
                c_tl = get_ref_tl(rpt_content.slot[0].ref_tl[c_port]);
            }
            else
            {
                return 1;
            }
        }
        else if((rpt_content.slot_o.rb_ref == '8') && ((num == 1)))
        {
            if((rpt_content.slot[1].ref_mod1[2] > '0') && (rpt_content.slot[1].ref_mod1[2] < '9'))
            {
                c_port = rpt_content.slot[1].ref_mod1[2] - 0x31;
                c_tl = get_ref_tl(rpt_content.slot[1].ref_tl[c_port]);
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
    else if(strncmp1(rpt_content.slot_p.rb_msmode, "MAIN", 4) == 0)
    {
        if((rpt_content.slot_p.rb_ref == '7') && ((num == 0)))
        {
            //先选择跟踪源 路数
            if((rpt_content.slot[0].ref_mod1[2] > '0') && (rpt_content.slot[0].ref_mod1[2] < '9'))
            {
                c_port = rpt_content.slot[0].ref_mod1[2] - 0x31;
                c_tl = get_ref_tl(rpt_content.slot[0].ref_tl[c_port]);
            }
            else
            {
                return 1;
            }
        }
        else if((rpt_content.slot_p.rb_ref == '8') && ((num == 1)))
        {
            if((rpt_content.slot[1].ref_mod1[2] > '0') && (rpt_content.slot[1].ref_mod1[2] < '9'))
            {
                c_port = rpt_content.slot[1].ref_mod1[2] - 0x31;
                c_tl = get_ref_tl(rpt_content.slot[1].ref_tl[c_port]);

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
    else
    {
        return 1;
    }

    if(c_tl != g_lev)			//再比较是否改变
    {
        g_lev = c_tl;
        for(i = 0; i < 14; i++)
        {
            if( (slot_type[i] == 'J') ||
                    (slot_type[i] == 'a') ||
                    (slot_type[i] == 'b') ||
                    (slot_type[i] == 'c') ||
                    (slot_type[i] == 'h') ||
                    (slot_type[i] == 'i') )
            {
                send_out_lev(c_tl, i);
            }
        }

        memset(ext_ssm, 0, 3);
        if(0x00 == c_tl)
        {
            memcpy(ext_ssm, "00", 2);
        }
        else if(0x02 == c_tl)
        {
            memcpy(ext_ssm, "02", 2);
        }
        else if(0x04 == c_tl)
        {
            memcpy(ext_ssm, "04", 2);
        }
        else if(0x08 == c_tl)
        {
            memcpy(ext_ssm, "08", 2);
        }
        else if(0x0B == c_tl)
        {
            memcpy(ext_ssm, "0B", 2);
        }
        else if(0x0F == c_tl)
        {
            memcpy(ext_ssm, "0F", 2);
        }
        else if(0xFF == c_tl)
        {
            memcpy(ext_ssm, "FF", 2);
        }
        else
        {
            memcpy(ext_ssm, "00", 2);
        }

        //扩展框 时钟等级
        ext_ssm_cfg(ext_ssm);
    }

    return 0;

}

int file_exists(char *filename)
{
    return (access(filename, 0) == 0);
}

/*
int find_config()
{
	struct dirent *ptr;
	DIR *dir;
	dir=opendir("/usr/");
	while((ptr=readdir(dir))!=NULL)
	{
		if(ptr->d_name[0] == '.')
		continue;
		if(strncmp1(ptr->d_name,"LT",2)==0)
			{
				memset(exist_filename,0,50);
				sprintf((char *)exist_filename,"/usr/%s",ptr->d_name);
				closedir(dir);
				return 1;
			}
	}
	closedir(dir);
	return 0;
}*/







/*
  1	成功
  0	失败
*/
int ext_alm_cmp(u8_t ext_sid, u8_t *ext_new_data, struct extctx *ctx)
{
    u8_t ext_old_alm[16];
    int i, j;

    memset(ext_old_alm, 0, 16);

    if((ext_sid >= EXT_1_OUT_LOWER) && (ext_sid <= EXT_1_OUT_UPPER))
    {
        memcpy(ext_old_alm, ctx->out[ext_sid - EXT_1_OUT_OFFSET].outAlm, 5);
        if(0 != memcmp(ext_old_alm, ext_new_data, 5))
        {
            for(i = 0; i < 5; i++)
            {
                for(j = 0; j < 4; j++)
                {
                    if( (0 == ((ext_old_alm[i])&BIT(j))) && (0 != ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('1', ext_sid - EXT_1_OUT_OFFSET + 97, 4 * i + j + 1, "**", "MJ");
						//printf("ext out1 alarm happened %d\r\n",4 * i + j + 1);
                    }
                    else if( (0 != ((ext_old_alm[i])&BIT(j))) && (0 == ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('1', ext_sid - EXT_1_OUT_OFFSET + 97, 4 * i + j + 1, "A ", "CL");
						//printf("ext out1 alarm clear %d\r\n",4 * i + j + 1);
                    }
                    else
                    {
                        //do nothing
                    }
                }
            }
        }
    }
    else if((ext_sid >= EXT_2_OUT_LOWER) && (ext_sid <= EXT_2_OUT_UPPER))
    {
        memcpy(ext_old_alm, ctx->out[ext_sid - EXT_2_OUT_OFFSET].outAlm, 5);
        if(0 != memcmp(ext_old_alm, ext_new_data, 5))
        {
            for(i = 0; i < 5; i++)
            {
                for(j = 0; j < 4; j++)
                {
                    if( (0 == ((ext_old_alm[i])&BIT(j))) && (0 != ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('2', ext_sid - EXT_2_OUT_OFFSET - 10 + 97, 4 * i + j + 1, "**", "MJ");
						//printf("ext out2 alarm happened %d\r\n",4 * i + j + 1);
                    }
                    else if( (0 != ((ext_old_alm[i])&BIT(j))) && (0 == ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('2', ext_sid - EXT_2_OUT_OFFSET - 10 + 97, 4 * i + j + 1, "A ", "CL");
						//printf("ext out2 alarm clear %d\r\n",4 * i + j + 1);
                    }
                    else
                    {
                        //do nothing
                    }
                }
            }
        }
    }
    else if((ext_sid >= EXT_3_OUT_LOWER) && (ext_sid <= EXT_3_OUT_UPPER))
    {
        if(ext_sid < 58)//except ':;'
        {
            memcpy(ext_old_alm, ctx->out[ext_sid - EXT_3_OUT_OFFSET].outAlm, 5);
            if(0 != memcmp(ext_old_alm, ext_new_data, 5))
            {
                for(i = 0; i < 5; i++)
                {
                    for(j = 0; j < 4; j++)
                    {
                        if( (0 == ((ext_old_alm[i])&BIT(j))) && (0 != ((ext_new_data[i])&BIT(j))) )
                        {
                            ext_alm_evt('3', ext_sid - EXT_3_OUT_OFFSET - 20 + 97, 4 * i + j + 1, "**", "MJ");
							//printf("ext out3 alarm happened %d\r\n",4 * i + j + 1);
                        }
                        else if( (0 != ((ext_old_alm[i])&BIT(j))) && (0 == ((ext_new_data[i])&BIT(j))) )
                        {
                            ext_alm_evt('3', ext_sid - EXT_3_OUT_OFFSET - 20 + 97, 4 * i + j + 1, "A ", "CL");
							//printf("ext out3 alarm clear %d\r\n",4 * i + j + 1);
                        }
                        else
                        {
                            //do nothing
                        }
                    }
                }
            }
        }
        else
        {
            memcpy(ext_old_alm, ctx->out[ext_sid - EXT_3_OUT_OFFSET - 2].outAlm, 5);
            if(0 != memcmp(ext_old_alm, ext_new_data, 5))
            {
                for(i = 0; i < 5; i++)
                {
                    for(j = 0; j < 4; j++)
                    {
                        if( (0 == ((ext_old_alm[i])&BIT(j))) && (0 != ((ext_new_data[i])&BIT(j))) )
                        {
                            ext_alm_evt('3', ext_sid - EXT_3_OUT_OFFSET - 22 + 97, 4 * i + j + 1, "**", "MJ");
							//printf("ext out3 alarm happened %d\r\n",4 * i + j + 1);
                        }
                        else if( (0 != ((ext_old_alm[i])&BIT(j))) && (0 == ((ext_new_data[i])&BIT(j))) )
                        {
                            ext_alm_evt('3', ext_sid - EXT_3_OUT_OFFSET - 22 + 97, 4 * i + j + 1, "A ", "CL");
							//printf("ext out3 alarm clear %d\r\n",4 * i + j + 1);
                        }
                        else
                        {
                            //do nothing
                        }
                    }
                }
            }
        }
    }
    else if(EXT_1_MGR_PRI == ext_sid)
    {
        memcpy(ext_old_alm, ctx->mgr[0].mgrAlm, MGE_RALM_NUM);
        if(0 != memcmp(ext_old_alm, ext_new_data, MGE_RALM_NUM))
        {
            for(i = 0; i < MGE_RALM_NUM; i++)
            {
                for(j = 0; j < 4; j++)
                {
                    if( (0 == ((ext_old_alm[i])&BIT(j))) && (0 != ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('1', 'k', 4 * i + j + 1, "**", "MJ");
                    }
                    else if( (0 != ((ext_old_alm[i])&BIT(j))) && (0 == ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('1', 'k', 4 * i + j + 1, "A ", "CL");
                    }
                    else
                    {
                        //do nothing
                    }
                }
            }
        }
    }
    else if(EXT_1_MGR_RSV == ext_sid)
    {
        memcpy(ext_old_alm, ctx->mgr[1].mgrAlm, MGE_RALM_NUM);
        if(0 != memcmp(ext_old_alm, ext_new_data, MGE_RALM_NUM))
        {
            for(i = 0; i < MGE_RALM_NUM; i++)
            {
                for(j = 0; j < 4; j++)
                {
                    if( (0 == ((ext_old_alm[i])&BIT(j))) && (0 != ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('1', 'l', 4 * i + j + 1, "**", "MJ");
                    }
                    else if( (0 != ((ext_old_alm[i])&BIT(j))) && (0 == ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('1', 'l', 4 * i + j + 1, "A ", "CL");
                    }
                    else
                    {
                        //do nothing
                    }
                }
            }
        }
    }
    else if(EXT_2_MGR_PRI == ext_sid)
    {
        memcpy(ext_old_alm, ctx->mgr[2].mgrAlm, MGE_RALM_NUM);
        if(0 != memcmp(ext_old_alm, ext_new_data, MGE_RALM_NUM))
        {
            for(i = 0; i < MGE_RALM_NUM; i++)
            {
                for(j = 0; j < 4; j++)
                {
                    if( (0 == ((ext_old_alm[i])&BIT(j))) && (0 != ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('2', 'k', 4 * i + j + 1, "**", "MJ");
                    }
                    else if( (0 != ((ext_old_alm[i])&BIT(j))) && (0 == ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('2', 'k', 4 * i + j + 1, "A ", "CL");
                    }
                    else
                    {
                        //do nothing
                    }
                }
            }
        }
    }
    else if(EXT_2_MGR_RSV == ext_sid)
    {
        memcpy(ext_old_alm, ctx->mgr[3].mgrAlm, MGE_RALM_NUM);
        if(0 != memcmp(ext_old_alm, ext_new_data, MGE_RALM_NUM))
        {
            for(i = 0; i < MGE_RALM_NUM; i++)
            {
                for(j = 0; j < 4; j++)
                {
                    if( (0 == ((ext_old_alm[i])&BIT(j))) && (0 != ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('2', 'l', 4 * i + j + 1, "**", "MJ");
                    }
                    else if( (0 != ((ext_old_alm[i])&BIT(j))) && (0 == ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('2', 'l', 4 * i + j + 1, "A ", "CL");
                    }
                    else
                    {
                        //do nothing
                    }
                }
            }
        }
    }
    else if(EXT_3_MGR_PRI == ext_sid)
    {
        memcpy(ext_old_alm, ctx->mgr[4].mgrAlm, MGE_RALM_NUM);
        if(0 != memcmp(ext_old_alm, ext_new_data, MGE_RALM_NUM))
        {
            for(i = 0; i < MGE_RALM_NUM; i++)
            {
                for(j = 0; j < 4; j++)
                {
                    if( (0 == ((ext_old_alm[i])&BIT(j))) && (0 != ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('3', 'k', 4 * i + j + 1, "**", "MJ");
						//printf("ext mgr3 alarm happened %d\r\n",4 * i + j + 1);
                    }
                    else if( (0 != ((ext_old_alm[i])&BIT(j))) && (0 == ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('3', 'k', 4 * i + j + 1, "A ", "CL");
						//printf("ext mgr3 alarm clear %d\r\n",4 * i + j + 1);
                    }
                    else
                    {
                        //do nothing
                    }
                }
            }
        }
    }
    else if(EXT_3_MGR_RSV == ext_sid)
    {
        memcpy(ext_old_alm, ctx->mgr[5].mgrAlm, MGE_RALM_NUM);
        if(0 != memcmp(ext_old_alm, ext_new_data, MGE_RALM_NUM))
        {
            for(i = 0; i < MGE_RALM_NUM; i++)
            {
                for(j = 0; j < 4; j++)
                {
                    if( (0 == ((ext_old_alm[i])&BIT(j))) && (0 != ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('3', 'l', 4 * i + j + 1, "**", "MJ");
						//printf("ext mge3 alarm happened %d\r\n",4 * i + j + 1);
                    }
                    else if( (0 != ((ext_old_alm[i])&BIT(j))) && (0 == ((ext_new_data[i])&BIT(j))) )
                    {
                        ext_alm_evt('3', 'l', 4 * i + j + 1, "A ", "CL");
						//printf("ext mge3 alarm clear %d\r\n",4 * i + j + 1);
                    }
                    else
                    {
                        //do nothing
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
int ext_alm_evt(u8_t eid, u8_t sid, u8_t almid, u8_t *agcc1, u8_t *agcc2)
{
    u8_t sendbuf[1024];
    signed char offset;
    u8_t onlinesta;

    offset = eid - '1';
    onlinesta = gExtCtx.save.onlineSta;
    if( offset > 2 || offset < 0)
    {
        return 0;
    }
    else
    {
        if( ((onlinesta >> offset) & 0x01) == 0)
        {
            return 0;
        }
        else
        {
            //lalalalala;
        }
    }


    memset(sendbuf, 0, 1024);
    get_time_fromnet();
    sprintf((char *)sendbuf, "\r\n\n   %s %s %s\r\n%s %06d EXT ALM\r\n   \"%c,%c,%03d,%s\"\r\n;",
            conf_content.slot_u.tid, mcp_date, mcp_time, agcc1, atag,
            eid, sid, almid, agcc2);
    
    sendto_all_client(sendbuf);
	//printf("send ext alm %s\r\n",sendbuf);
    atag++;
    if(atag >= 0xffff)
    {
        atag = 0;
    }

    return 1;
}





/*
  1	成功
  0	失败
*/
int ext_bpp_cfg(u8_t ext_sid, u8_t *ext_new_type, struct extctx *ctx)
{
    u8_t ext_old_type[15];
    int i;
    u8_t tmp_ctag[7];
    u8_t sendbuf[256];
    u8_t slotid;

    memset(ext_old_type, 0, 15);
    memset(tmp_ctag, 0, 7);
    memcpy(tmp_ctag, "000000", 6);

    if((EXT_1_MGR_PRI == ext_sid) || (EXT_1_MGR_RSV == ext_sid))
    {
        memcpy(ext_old_type, ctx->extBid[0], 14);
        for(i = 0; i < 10; i++)
        {
            if( (EXT_NONE == ext_old_type[i]) &&
                    ((EXT_OUT16 == ext_new_type[i]) || (EXT_OUT32 == ext_new_type[i]) || (EXT_OUT16S == ext_new_type[i]) || (EXT_OUT32S == ext_new_type[i])) )
            {
                // lev
                printf("1+%d+%s\n", i + 1, ctx->save.outSsm[i]);
                if(2 == strlen(ctx->save.outSsm[i]))
                {
                    memset(sendbuf, 0, 256);
                    sprintf((char *)sendbuf, "SET-OUT-LEV:%c:%s::%s;",
                            i + EXT_1_OUT_LOWER,
                            tmp_ctag,
                            ctx->save.outSsm[i]);
                    ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, i + EXT_1_OUT_LOWER, tmp_ctag);
                }

                //signal
                printf("1+%d+%s\n", i + 1, ctx->save.outSignal[i]);
                if(16 == strlen(ctx->save.outSignal[i]))
                {
                    memset(sendbuf, 0, 256);
                    sprintf((char *)sendbuf, "SET-OUT-TYP:%c:%s::%s;",
                            i + EXT_1_OUT_LOWER,
                            tmp_ctag,
                            ctx->save.outSignal[i]);
                    ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, i + EXT_1_OUT_LOWER, tmp_ctag);
                }

                //mod
                printf("1+%d+%s\n", i + 1, ctx->save.outPR[i]);
                if('0' == ctx->save.outPR[i][0] || '1' == ctx->save.outPR[i][0])
                {
                    memset(sendbuf, 0, 256);
                    sprintf((char *)sendbuf, "SET-OUT-MOD:%c:%s::%c;",
                            i + EXT_1_OUT_LOWER,
                            tmp_ctag,
                            ctx->save.outPR[i][0]);
                    ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, i + EXT_1_OUT_LOWER, tmp_ctag);
                }
            }
        }
    }
    else if((EXT_2_MGR_PRI == ext_sid) || (EXT_2_MGR_RSV == ext_sid))
    {
        memcpy(ext_old_type, ctx->extBid[1], 14);
        for(i = 0; i < 10; i++)
        {
            if( (EXT_NONE == ext_old_type[i]) &&
                    ((EXT_OUT16 == ext_new_type[i]) || (EXT_OUT32 == ext_new_type[i]) || (EXT_OUT16S == ext_new_type[i]) || (EXT_OUT32S == ext_new_type[i])) )
            {
                //lev
                printf("2+%d+%s\n", i + 1, ctx->save.outSsm[10 + i]);
                if(2 == strlen(ctx->save.outSsm[10 + i]))
                {
                    memset(sendbuf, 0, 256);
                    sprintf((char *)sendbuf, "SET-OUT-LEV:%c:%s::%s;",
                            i + EXT_2_OUT_LOWER,
                            tmp_ctag,
                            ctx->save.outSsm[10 + i]);
                    ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, i + EXT_2_OUT_LOWER, tmp_ctag);
                }

                //signal
                printf("2+%d+%s\n", i + 1, ctx->save.outSignal[10 + i]);
                if(16 == strlen(ctx->save.outSignal[10 + i]))
                {
                    memset(sendbuf, 0, 256);
                    sprintf((char *)sendbuf, "SET-OUT-TYP:%c:%s::%s;",
                            i + EXT_2_OUT_LOWER,
                            tmp_ctag,
                            ctx->save.outSignal[10 + i]);
                    ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, i + EXT_2_OUT_LOWER, tmp_ctag);
                }

                //mod
                printf("2+%d+%s\n", i + 1, ctx->save.outPR[10 + i]);
                if('0' == ctx->save.outPR[10 + i][0] || '1' == ctx->save.outPR[10 + i][0])
                {
                    memset(sendbuf, 0, 256);
                    sprintf((char *)sendbuf, "SET-OUT-MOD:%c:%s::%c;",
                            i + EXT_2_OUT_LOWER,
                            tmp_ctag,
                            ctx->save.outPR[10 + i][0]);
                    ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, i + EXT_2_OUT_LOWER, tmp_ctag);
                }
            }
        }
    }
    else if((EXT_3_MGR_PRI == ext_sid) || (EXT_3_MGR_RSV == ext_sid))
    {
        memcpy(ext_old_type, ctx->extBid[2], 14);
        for(i = 0; i < 10; i++)
        {
            if( (EXT_NONE == ext_old_type[i]) &&
                    ((EXT_OUT16 == ext_new_type[i]) || (EXT_OUT32 == ext_new_type[i]) || (EXT_OUT16S == ext_new_type[i]) || (EXT_OUT32S == ext_new_type[i])) )
            {
                if((i + EXT_3_OUT_LOWER) >= ':')//except ':;'
                {
                    slotid = i + EXT_3_OUT_LOWER + 2;
                }
                else
                {
                    slotid = i + EXT_3_OUT_LOWER;
                }

                //lev
                printf("3+%d+%s\n", i + 1, ctx->save.outSsm[20 + i]);
                if(2 == strlen(ctx->save.outSsm[20 + i]))
                {
                    memset(sendbuf, 0, 256);
                    sprintf((char *)sendbuf, "SET-OUT-LEV:%c:%s::%s;",
                            slotid,
                            tmp_ctag,
                            ctx->save.outSsm[20 + i]);
                    ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, slotid, tmp_ctag);
                }

                //signal
                printf("3+%d+%s\n", i + 1, ctx->save.outSignal[20 + i]);
                if(16 == strlen(ctx->save.outSignal[20 + i]))
                {
                    memset(sendbuf, 0, 256);
                    sprintf((char *)sendbuf, "SET-OUT-TYP:%c:%s::%s;",
                            slotid,
                            tmp_ctag,
                            ctx->save.outSignal[20 + i]);
                    ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, slotid, tmp_ctag);
                }

                //mod
                printf("3+%d+%s\n", i + 1, ctx->save.outPR[20 + i]);
                if('0' == ctx->save.outPR[20 + i][0] || '1' == ctx->save.outPR[20 + i][0])
                {
                    memset(sendbuf, 0, 256);
                    sprintf((char *)sendbuf, "SET-OUT-MOD:%c:%s::%c;",
                            slotid,
                            tmp_ctag,
                            ctx->save.outPR[20 + i][0]);
                    ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, slotid, tmp_ctag);
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






int ethn_down(const char *networkcard)
{
    int sock_fd;
    struct ifreq ifr;

    if ( ( sock_fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
        close(sock_fd);
        return (0);
    }
    memset( &ifr, 0, sizeof(struct ifreq) );
    strncpy( ifr.ifr_name, networkcard, IFNAMSIZ );
    if (ioctl(sock_fd, SIOCGIFFLAGS, &ifr) < 0)
    {
        close(sock_fd);
        return (0);
    }

    ifr.ifr_flags &= (~IFF_UP);
    if ( ioctl ( sock_fd, SIOCSIFFLAGS, &ifr) < 0 )
    {
        close(sock_fd);
        return (0);
    }

    close(sock_fd);
    return (1);
}



int ethn_up(const char *networkcard)
{
    int sock_fd;
    struct ifreq ifr;

    if ( ( sock_fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
        close(sock_fd);
        return (0);
    }
    memset( &ifr, 0, sizeof(struct ifreq) );
    strncpy( ifr.ifr_name, networkcard, IFNAMSIZ );
    if (ioctl(sock_fd, SIOCGIFFLAGS, &ifr) < 0)
    {
        close(sock_fd);
        return (0);
    }

    ifr.ifr_flags |= (IFF_UP | IFF_RUNNING);
    if ( ioctl ( sock_fd, SIOCSIFFLAGS, &ifr) < 0 )
    {
        close(sock_fd);
        return (0);
    }

    close(sock_fd);
    return (1);
}



int mcp_get_gateway( char *local_gateway )
{
    char buf[1024];
    int pl = 0, ph = 0;
    int i;
    FILE *readfp;

    memset( buf, 0, sizeof(buf) );
    readfp = popen( "route -n", "r" );
    if ( NULL == readfp )
    {
        printf("route -n is NULL\n");
        pclose( readfp );
        return(0);
    }

    if ( 0 == fread( buf, sizeof(char), sizeof(buf),  readfp) )
    {
        pclose( readfp );
        return (0);
    }

    for( i = 0; i < 1024; i++ )
    {
        if ( buf[i] == 'U' && buf[i + 1] == 'G' && (i + 1) < 1024 )
        {
            pl = i;
            break;
        }
    }
    if( pl == 0 )
    {
        pclose( readfp );
        return(0);
    }

    while ( buf[pl] > '9' || buf[pl] < '0' )
    {
        pl--;
    }
    while ( (buf[pl] >= '0' && buf[pl] <= '9') || buf[pl] == '.')
    {
        pl--;
    }
    while ( buf[pl] > '9' || buf[pl] < '0' )
    {
        pl--;
    }
    buf[pl + 1] = '\0';

    for ( i = pl; ((buf[i] >= '0' && buf[i] <= '9') || buf[i] == '.'); i--);
    ph = i + 1;
    strcpy(local_gateway, &buf[ph]);
    //printf("<mcp_get_gateway>:%s\n",local_gateway);
    pclose( readfp );

    return (1);
}



int mcp_set_gateway( char *networkcard, char *old_gateway, char *new_gateway )
{
    int sock_fd = -1;
    struct rtentry rt;
    struct sockaddr rtsockaddr;
    struct sockaddr_in	*sin = NULL;

    if ( (sock_fd = socket(AF_INET, SOCK_STREAM, 0) ) < 0 )
    {
        close(sock_fd);
        return(0);
    }

    memset(&rt, 0, sizeof(struct rtentry));
    memset(&rtsockaddr, 0, sizeof(struct sockaddr));

    sin = (struct sockaddr_in *)&rtsockaddr;
    sin->sin_family = AF_INET;
    sin->sin_addr.s_addr = INADDR_ANY;

    rt.rt_dst = rtsockaddr;		//set the destination address to '0.0.0.0'
    rt.rt_genmask = rtsockaddr;	//set the netmask to '0.0.0.0'

    //Fill in the other fields.
    rt.rt_flags = (RTF_UP | RTF_GATEWAY);
    rt.rt_metric = 0;
    rt.rt_dev = networkcard;

    //delete the current default gateway.
    sin->sin_addr.s_addr = inet_addr(old_gateway);
    rt.rt_gateway = rtsockaddr;

    if (ioctl(sock_fd, SIOCDELRT, &rt) < 0)
    {
        if( errno != ESRCH)
        {
            close(sock_fd);
            return(0);
        }
    }
    //set the new default gateway.
    sin->sin_addr.s_addr = inet_addr(new_gateway);
    rt.rt_gateway = rtsockaddr;

    if (ioctl(sock_fd, SIOCADDRT, &rt) < 0)
    {
        close(sock_fd);
        return(0);
    }

    close(sock_fd);
    return(1);
}




void mcpd_reset_network_card(void)
{
    int retval = 0;
    unsigned char gateway[16];

    memset(gateway, 0, sizeof(gateway));

    retval = mcp_get_gateway(gateway);
    if( 1 == retval )
    {
        printf("reset_network_card: 1\n");
        if ( 1 == ethn_down("eth0"))
        {
            printf("eth0 down success!!!\n");
            while ( 0 == ethn_up("eth0") )
            {
                printf("eth0 up failure!!!\n");
            }
            printf("eth0 up success!!!\n");

            mcp_set_gateway(TCP_NETWORK_CARD, gateway, gateway);
        }
        else
        {
            if(print_switch == 0)
            {
                printf("eth0 down failure!!!\n");
            }
        }
    }
    else//2014-1-24添加else语句，未获取成功时，使用flash中保存的网关
    {
        printf("reset_network_card: 0\n");
        memcpy(gateway, conf_content.slot_u.gate, 16);
        if ( 1 == ethn_down("eth0"))
        {
            printf("eth0 down success!!!\n");
            while ( 0 == ethn_up("eth0") )
            {
                printf("eth0 up failure!!!\n");
            }
            printf("eth0 up success!!!\n");

            mcp_set_gateway(TCP_NETWORK_CARD, gateway, gateway);
        }
        else
        {
            if(print_switch == 0)
            {
                printf("eth0 down failure!!!\n");
            }
        }

    }
}






/*
  1	成功
  0	失败
*/
int ext_drv_mgr_evt(struct extctx *ctx)
{
    u8_t sendbuf[1024];
    u8_t aid;

    memset(ctx->new_drv_mgr_sta, 0, 32);
    sprintf((char *) ctx->new_drv_mgr_sta,
             "%d%d%d%d%d%d,%d%d%d%d%d%d",
             ctx->drv[0].drvPR[0], ctx->drv[1].drvPR[0],
             ctx->drv[0].drvPR[1], ctx->drv[1].drvPR[1],
             ctx->drv[0].drvPR[2], ctx->drv[1].drvPR[2],
             ctx->mgr[0].mgrPR, ctx->mgr[1].mgrPR,
             ctx->mgr[2].mgrPR, ctx->mgr[3].mgrPR,
             ctx->mgr[4].mgrPR, ctx->mgr[5].mgrPR );

    if(0 != strcmp(ctx->old_drv_mgr_sta, ctx->new_drv_mgr_sta))
    {
        memcpy(ctx->old_drv_mgr_sta, ctx->new_drv_mgr_sta, 13);
        memset(sendbuf, 0, 1024);
        get_time_fromnet();

        if('d' == slot_type[12])
        {
            aid = 'm';
        }
        else if('d' == slot_type[13])
        {
            aid = 'n';
        }
        else
        {
            aid = 'm';
        }

        sprintf((char *)sendbuf, "\r\n\n   %s %s %s\r\nA  %06d REPT EVNT CDM\r\n   \"%c:NA,%s,NSA,AUTO:\\\"\\\"\"\r\n;",
                conf_content.slot_u.tid, mcp_date, mcp_time,
                atag, aid,
                ctx->new_drv_mgr_sta);

        sendto_all_client(sendbuf);
        atag++;
        if(atag >= 0xffff)
        {
            atag = 0;
        }
    }

    return 1;
}








void ext_bpp_evt(int aid, char bid)
{
    unsigned char tmp[1024];

    memset(tmp, '\0', 1024);
    get_time_fromnet();
    sprintf((char *)tmp, "%c%c%c%c%c%c%s%c", CR, LF, LF, SP, SP, SP, conf_content.slot_u.tid, SP);
    sprintf((char *)tmp, "%s%s%c%s%c%c", tmp, mcp_date, SP, mcp_time, CR, LF);
    sprintf((char *)tmp, "%sA%c%c%06d%cREPT%cEVNT%cC91%c%c", tmp, SP, SP, atag, SP, SP, SP, CR, LF);
    sprintf((char *)tmp, "%s%c%c%c\"%c:NA,%c,NSA,AUTO:\\\"\\\"\"%c%c;",
            tmp, SP, SP, SP, aid, bid, CR, LF);

    sendto_all_client(tmp);
	//printf("Send C91 %s\n",tmp);
    atag++;
    if(atag >= 0xffff)
    {
        atag = 0;
    }
}






/*
  1	成功
  0	失败
*/
int ext_bid_cmp(u8_t ext_sid, u8_t *ext_new_data, struct extctx *ctx)
{
    u8_t ext_old_bid[16];
    u32_t tmp;
    u8_t bppa[8];
    int i;

    memset(ext_old_bid, 0, 16);

    if(EXT_1_MGR_PRI == ext_sid)
    {
        tmp = 0;
        memcpy(ext_old_bid, ctx->extBid[0], 14);
        if(0 != memcmp(ext_old_bid, ext_new_data, 14))
        {
            for(i = 0; i < 14; i++)
            {
                if(ext_old_bid[i] != ext_new_data[i])
                {
                    if(EXT_NONE == ext_old_bid[i] && EXT_NONE != ext_new_data[i])
                    {
                        tmp &= (~BIT(i));
                        ext_alm_evt('1', 'k', 13 + i, "A ", "CL");
                    }
                    else if(EXT_NONE != ext_old_bid[i] && EXT_NONE == ext_new_data[i])
                    {
                        tmp |= BIT(i);
                        ext_alm_evt('1', 'k', 13 + i, "**", "MJ");
                    }
                    else
                    {
                        //do nothing
                    }

                    if(i < 10)
                    {
                        ext_bpp_evt(23 + i, ext_new_data[i]);
                    }
                    else if(10 == i)
                    {
                        ext_bpp_evt(EXT_1_MGR_PRI, ext_new_data[i]);
                    }
                    else if(11 == i)
                    {
                        ext_bpp_evt(EXT_1_MGR_RSV, ext_new_data[i]);
                    }
                    else if(12 == i)
                    {
                        ext_bpp_evt(EXT_1_PWR1, ext_new_data[i]);
                    }
                    else if(13 == i)
                    {
                        ext_bpp_evt(EXT_1_PWR2, ext_new_data[i]);
                    }
                    else
                    {
                        //do nothing
                    }
                }
            }

            memset(bppa, 0, 8);
            bppa[0] = 0x40 | (tmp & 0x0000000F);
            bppa[1] = 0x40 | ((tmp >> 4) & 0x0000000F);
            bppa[2] = 0x40 | ((tmp >> 8) & 0x0000000F);
            bppa[3] = 0x40 | ((tmp >> 12) & 0x0000000F);
            memcpy(&ctx->mgr[0].mgrAlm[3], bppa, 4);
        }
    }
    else if(EXT_1_MGR_RSV == ext_sid)
    {
        tmp = 0;
        memcpy(ext_old_bid, ctx->extBid[0], 14);
        if(0 != memcmp(ext_old_bid, ext_new_data, 14))
        {
            for(i = 0; i < 14; i++)
            {
                if(ext_old_bid[i] != ext_new_data[i])
                {
                    if(EXT_NONE == ext_old_bid[i] && EXT_NONE != ext_new_data[i])
                    {
                        tmp &= (~BIT(i));
                        ext_alm_evt('1', 'l', 13 + i, "A ", "CL");
                    }
                    else if(EXT_NONE != ext_old_bid[i] && EXT_NONE == ext_new_data[i])
                    {
                        tmp |= BIT(i);
                        ext_alm_evt('1', 'l', 13 + i, "**", "MJ");
                    }
                    else
                    {
                        //do nothing
                    }

                    if(i < 10)
                    {
                        ext_bpp_evt(23 + i, ext_new_data[i]);
                    }
                    else if(10 == i)
                    {
                        ext_bpp_evt(EXT_1_MGR_PRI, ext_new_data[i]);
                    }
                    else if(11 == i)
                    {
                        ext_bpp_evt(EXT_1_MGR_RSV, ext_new_data[i]);
                    }
                    else if(12 == i)
                    {
                        ext_bpp_evt(EXT_1_PWR1, ext_new_data[i]);
                    }
                    else if(13 == i)
                    {
                        ext_bpp_evt(EXT_1_PWR2, ext_new_data[i]);
                    }
                    else
                    {
                        //do nothing
                    }
                }
            }

            memset(bppa, 0, 8);
            bppa[0] = 0x40 | (tmp & 0x0000000F);
            bppa[1] = 0x40 | ((tmp >> 4) & 0x0000000F);
            bppa[2] = 0x40 | ((tmp >> 8) & 0x0000000F);
            bppa[3] = 0x40 | ((tmp >> 12) & 0x0000000F);
            memcpy(&ctx->mgr[1].mgrAlm[3], bppa, 4);
        }
    }
    else if(EXT_2_MGR_PRI == ext_sid)
    {
        tmp = 0;
        memcpy(ext_old_bid, ctx->extBid[1], 14);
        if(0 != memcmp(ext_old_bid, ext_new_data, 14))
        {
            for(i = 0; i < 14; i++)
            {
                if(ext_old_bid[i] != ext_new_data[i])
                {
                    if(EXT_NONE == ext_old_bid[i] && EXT_NONE != ext_new_data[i])
                    {
                        tmp &= (~BIT(i));
                        ext_alm_evt('2', 'k', 13 + i, "A ", "CL");
                    }
                    else if(EXT_NONE != ext_old_bid[i] && EXT_NONE == ext_new_data[i])
                    {
                        tmp |= BIT(i);
                        ext_alm_evt('2', 'k', 13 + i, "**", "MJ");
                    }
                    else
                    {
                        //do nothing
                    }

                    if(i < 10)
                    {
                        ext_bpp_evt(39 + i, ext_new_data[i]);
                    }
                    else if(10 == i)
                    {
                        ext_bpp_evt(EXT_2_MGR_PRI, ext_new_data[i]);
                    }
                    else if(11 == i)
                    {
                        ext_bpp_evt(EXT_2_MGR_RSV, ext_new_data[i]);
                    }
                    else if(12 == i)
                    {
                        ext_bpp_evt(EXT_2_PWR1, ext_new_data[i]);
                    }
                    else if(13 == i)
                    {
                        ext_bpp_evt(EXT_2_PWR2, ext_new_data[i]);
                    }
                    else
                    {
                        //do nothing
                    }
                }
            }

            memset(bppa, 0, 8);
            bppa[0] = 0x40 | (tmp & 0x0000000F);
            bppa[1] = 0x40 | ((tmp >> 4) & 0x0000000F);
            bppa[2] = 0x40 | ((tmp >> 8) & 0x0000000F);
            bppa[3] = 0x40 | ((tmp >> 12) & 0x0000000F);
            memcpy(&ctx->mgr[2].mgrAlm[3], bppa, 4);
        }
    }
    else if(EXT_2_MGR_RSV == ext_sid)
    {
        tmp = 0;
        memcpy(ext_old_bid, ctx->extBid[1], 14);
        if(0 != memcmp(ext_old_bid, ext_new_data, 14))
        {
            for(i = 0; i < 14; i++)
            {
                if(ext_old_bid[i] != ext_new_data[i])
                {
                    if(EXT_NONE == ext_old_bid[i] && EXT_NONE != ext_new_data[i])
                    {
                        tmp &= (~BIT(i));
                        ext_alm_evt('2', 'l', 13 + i, "A ", "CL");
                    }
                    else if(EXT_NONE != ext_old_bid[i] && EXT_NONE == ext_new_data[i])
                    {
                        tmp |= BIT(i);
                        ext_alm_evt('2', 'l', 13 + i, "**", "MJ");
                    }
                    else
                    {
                        //do nothing
                    }

                    if(i < 10)
                    {
                        ext_bpp_evt(39 + i, ext_new_data[i]);
                    }
                    else if(10 == i)
                    {
                        ext_bpp_evt(EXT_2_MGR_PRI, ext_new_data[i]);
                    }
                    else if(11 == i)
                    {
                        ext_bpp_evt(EXT_2_MGR_RSV, ext_new_data[i]);
                    }
                    else if(12 == i)
                    {
                        ext_bpp_evt(EXT_2_PWR1, ext_new_data[i]);
                    }
                    else if(13 == i)
                    {
                        ext_bpp_evt(EXT_2_PWR2, ext_new_data[i]);
                    }
                    else
                    {
                        //do nothing
                    }
                }
            }

            memset(bppa, 0, 8);
            bppa[0] = 0x40 | (tmp & 0x0000000F);
            bppa[1] = 0x40 | ((tmp >> 4) & 0x0000000F);
            bppa[2] = 0x40 | ((tmp >> 8) & 0x0000000F);
            bppa[3] = 0x40 | ((tmp >> 12) & 0x0000000F);
            memcpy(&ctx->mgr[3].mgrAlm[3], bppa, 4);
        }
    }
    else if(EXT_3_MGR_PRI == ext_sid)
    {
        tmp = 0;
        memcpy(ext_old_bid, ctx->extBid[2], 14);
        if(0 != memcmp(ext_old_bid, ext_new_data, 14))
        {
            for(i = 0; i < 14; i++)
            {
                if(ext_old_bid[i] != ext_new_data[i])
                {
                    if(EXT_NONE == ext_old_bid[i] && EXT_NONE != ext_new_data[i])
                    {
                        tmp &= (~BIT(i));
                        ext_alm_evt('3', 'k', 13 + i, "A ", "CL");
                    }
                    else if(EXT_NONE != ext_old_bid[i] && EXT_NONE == ext_new_data[i])
                    {
                        tmp |= BIT(i);
                        ext_alm_evt('3', 'k', 13 + i, "**", "MJ");
                    }
                    else
                    {
                        //do nothing
                    }

                    if(i < 3)
                    {
                        ext_bpp_evt(55 + i, ext_new_data[i]);
                    }
                    else if(i >= 3 && i < 10)
                    {
                        ext_bpp_evt(57 + i, ext_new_data[i]);
                    }
                    else if(10 == i)
                    {
                        ext_bpp_evt(EXT_3_MGR_PRI, ext_new_data[i]);
                    }
                    else if(11 == i)
                    {
                        ext_bpp_evt(EXT_3_MGR_RSV, ext_new_data[i]);
                    }
                    else if(12 == i)
                    {
                        ext_bpp_evt(EXT_3_PWR1, ext_new_data[i]);
                    }
                    else if(13 == i)
                    {
                        ext_bpp_evt(EXT_3_PWR2, ext_new_data[i]);
                    }
                    else
                    {
                        //do nothing
                    }
                }
            }

            memset(bppa, 0, 8);
            bppa[0] = 0x40 | (tmp & 0x0000000F);
            bppa[1] = 0x40 | ((tmp >> 4) & 0x0000000F);
            bppa[2] = 0x40 | ((tmp >> 8) & 0x0000000F);
            bppa[3] = 0x40 | ((tmp >> 12) & 0x0000000F);
            memcpy(&ctx->mgr[4].mgrAlm[3], bppa, 4);
        }
    }
    else if(EXT_3_MGR_RSV == ext_sid)
    {
        tmp = 0;
        memcpy(ext_old_bid, ctx->extBid[2], 14);
        if(0 != memcmp(ext_old_bid, ext_new_data, 14))
        {
            for(i = 0; i < 14; i++)
            {
                if(ext_old_bid[i] != ext_new_data[i])
                {
                    if(EXT_NONE == ext_old_bid[i] && EXT_NONE != ext_new_data[i])
                    {
                        tmp &= (~BIT(i));
                        ext_alm_evt('3', 'l', 13 + i, "A ", "CL");
                    }
                    else if(EXT_NONE != ext_old_bid[i] && EXT_NONE == ext_new_data[i])
                    {
                        tmp |= BIT(i);
                        ext_alm_evt('3', 'l', 13 + i, "**", "MJ");
                    }
                    else
                    {
                        //do nothing
                    }

                    if(i < 3)
                    {
                        ext_bpp_evt(55 + i, ext_new_data[i]);
                    }
                    else if(i >= 3 && i < 10)
                    {
                        ext_bpp_evt(57 + i, ext_new_data[i]);
                    }
                    else if(10 == i)
                    {
                        ext_bpp_evt(EXT_3_MGR_PRI, ext_new_data[i]);
                    }
                    else if(11 == i)
                    {
                        ext_bpp_evt(EXT_3_MGR_RSV, ext_new_data[i]);
                    }
                    else if(12 == i)
                    {
                        ext_bpp_evt(EXT_3_PWR1, ext_new_data[i]);
                    }
                    else if(13 == i)
                    {
                        ext_bpp_evt(EXT_3_PWR2, ext_new_data[i]);
                    }
                    else
                    {
                        //do nothing
                    }
                }
            }

            memset(bppa, 0, 8);
            bppa[0] = 0x40 | (tmp & 0x0000000F);
            bppa[1] = 0x40 | ((tmp >> 4) & 0x0000000F);
            bppa[2] = 0x40 | ((tmp >> 8) & 0x0000000F);
            bppa[3] = 0x40 | ((tmp >> 12) & 0x0000000F);
            memcpy(&ctx->mgr[5].mgrAlm[3], bppa, 4);
        }
    }
    else
    {
        return 0;
    }

    return 1;
}







void ext_ssm_cfg(char *pSsm)
{
    int i;
    u8_t tmp_ctag[7];
    u8_t sendbuf[256];
    u8_t slotid;

    memcpy(tmp_ctag, "000000", 6);

    if(	((EXT_DRV == slot_type[12]) || (EXT_DRV == slot_type[13])) &&
            ((1 == gExtCtx.drv[0].drvPR[0]) || (1 == gExtCtx.drv[1].drvPR[0])) )
    {
        for(i = 0; i < 10; i++)
        {
            if((EXT_OUT16 == gExtCtx.extBid[0][i]) || (EXT_OUT32 == gExtCtx.extBid[0][i]) || (EXT_OUT16S == gExtCtx.extBid[0][i]) || (EXT_OUT32S == gExtCtx.extBid[0][i]))
            {
                // lev
                memset(sendbuf, 0, 256);
                sprintf((char *)	sendbuf, "SET-OUT-LEV:%c:%s::%s;",
                            i + EXT_1_OUT_LOWER,
                            tmp_ctag,
                            pSsm );
                ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, i + EXT_1_OUT_LOWER, tmp_ctag);
            }
        }
    }

    if(	((EXT_DRV == slot_type[12]) || (EXT_DRV == slot_type[13])) &&
            ((1 == gExtCtx.drv[0].drvPR[1]) || (1 == gExtCtx.drv[1].drvPR[1])) )
    {
        for(i = 0; i < 10; i++)
        {
            if((EXT_OUT16 == gExtCtx.extBid[1][i]) || (EXT_OUT32 == gExtCtx.extBid[1][i]) || (EXT_OUT16S == gExtCtx.extBid[1][i]) || (EXT_OUT32S == gExtCtx.extBid[1][i]))
            {
                //lev
                memset(sendbuf, 0, 256);
                sprintf((char *)	sendbuf, "SET-OUT-LEV:%c:%s::%s;",
                            i + EXT_2_OUT_LOWER,
                            tmp_ctag,
                            pSsm );
                ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, i + EXT_2_OUT_LOWER, tmp_ctag);
            }
        }
    }

    if(	((EXT_DRV == slot_type[12]) || (EXT_DRV == slot_type[13])) &&
            ((1 == gExtCtx.drv[0].drvPR[2]) || (1 == gExtCtx.drv[1].drvPR[2])) )
    {
        for(i = 0; i < 10; i++)
        {
            if((EXT_OUT16 == gExtCtx.extBid[2][i]) || (EXT_OUT32 == gExtCtx.extBid[2][i]) || (EXT_OUT16S == gExtCtx.extBid[2][i]) || (EXT_OUT32S == gExtCtx.extBid[2][i]))
            {
                if((i + EXT_3_OUT_LOWER) >= ':')//except ':;'
                {
                    slotid = i + EXT_3_OUT_LOWER + 2;
                }
                else
                {
                    slotid = i + EXT_3_OUT_LOWER;
                }

                //lev
                memset(sendbuf, 0, 256);
                sprintf((char *)	sendbuf, "SET-OUT-LEV:%c:%s::%s;",
                            slotid,
                            tmp_ctag,
                            pSsm	);
                ext_issue_cmd(sendbuf, strlen_r(sendbuf, ';') + 1, slotid, tmp_ctag);
            }
        }
    }

    if((EXT_DRV == slot_type[12]) || (EXT_DRV == slot_type[13]))
    {
        for(i = 0; i < 30; i++)
        {
            memcpy(gExtCtx.save.outSsm[i], pSsm, 2);
            memcpy(gExtCtx.out[i].outSsm, pSsm, 2);
        }

        ext_write_flash(&(gExtCtx.save));
    }
}


int is_freq_input(unsigned char slot)
{
    int i;
    unsigned char port[2];
    if(conf_content.slot_u.fb == '0')
    {
        if(slot == RB1_SLOT)
        {
            for(i = 0; i < 4; i++)
            {
                if(flag_alm[14][i + 4] == 0x00)
                {
                    return 0;
                }
            }
        }
        else if(slot == RB2_SLOT)
        {
            for(i = 0; i < 4; i++)
            {
                if(flag_alm[15][i + 4] == 0x00)
                {
                    return 0;
                }
            }
        }
    }
    else
    {
#if 1
        if(slot_type[0] == 'S')
        {
            memset(port, 0, 2);

            port[0] = alm_sta[0][5] & 0x0f;
            port[1] = alm_sta[0][6] & 0x0f;

            for(i = 9; i < 11; i++) //0x04
            {
                if( (i % 2) != 0 )
                {
                    port[0] |= (alm_sta[0][i] & 0x0f);
                }
                else
                {
                    port[1] |= (alm_sta[0][i] & 0x0f);
                }
            }
            for(i = 5; i < 11; i++) //0x05
            {
                if( (i % 2) != 0 )
                {
                    port[0] |= (alm_sta[18][i] & 0x0f);
                }
                else
                {
                    port[1] |= (alm_sta[18][i] & 0x0f);
                }
            }
            for(i = 0; i < 4; i++)
            {
                if( ((port[0] >> i) & 0x01) == 0x00)
                {
                    return 0;
                }
                if( ((port[1] >> i) & 0x01) == 0x0)
                {
                    return 0;
                }
            }
        }
        else
        {
            //nothing
        }
        if(slot_type[1] == 'S')
        {
            memset(port, 0, 2);
            port[0] = alm_sta[1][5] & 0x0f;
            port[1] = alm_sta[1][6] & 0x0f;
            for(i = 9; i < 11; i++) //0x04
            {
                if( (i % 2) != 0 )
                {
                    port[0] |= (alm_sta[1][i] & 0x0f);
                }
                else
                {
                    port[1] |= (alm_sta[1][i] & 0x0f);
                }
            }
            for(i = 5; i < 11; i++) //0x05
            {
                if( (i % 2) != 0 )
                {
                    port[0] |= (alm_sta[19][i] & 0x0f);
                }
                else
                {
                    port[1] |= (alm_sta[19][i] & 0x0f);
                }
            }
            for(i = 0; i < 4; i++)
            {
                if( ((port[0] >> i) & 0x01) == 0x00)
                {
                    return 0;
                }
                if( ((port[1] >> i) & 0x01) == 0x00)
                {
                    return 0;
                }
            }
        }
#endif
    }

    return 1;
}

//检查是否为2级时钟,小于等于G.812
int timeClock_2(const unsigned char rb)
{
    unsigned char port;
    unsigned char slot = 0;
    if('0' == conf_content.slot_u.fb)
    {
        switch(rpt_content.slot_o.rb_ref)
        {
        case '5':
            return 0;
            break;

        case '6':
            return 0;
            break;

        case '7':
            return 0;
            break;

        case '8':
            return 0;
            break;

        default:
            return 0;
            break;
        }
    }
    else
    {

        //后面板:3--reftf1 时间;4--reftf2时间;5--reftf1频率;6--reftf2频率
        if('o' == rb)//计算reftf槽位号
        {
            if('3' == rpt_content.slot_o.rb_ref || '4' == rpt_content.slot_o.rb_ref)//后面板时间输入
            {
                slot = rpt_content.slot_o.rb_ref - '3';
            }
            else if('5' == rpt_content.slot_o.rb_ref || '6' == rpt_content.slot_o.rb_ref)//后面板频率输入
            {
                slot = rpt_content.slot_o.rb_ref - '5';

            }
            else
            {
                printf("RB up pps_stat error\n");
            }
        }
        else
        {
            if('3' == rpt_content.slot_p.rb_ref || '4' == rpt_content.slot_p.rb_ref)//后面板时间输入
            {
                slot = rpt_content.slot_p.rb_ref - '3';
            }
            else if('5' == rpt_content.slot_p.rb_ref || '6' == rpt_content.slot_p.rb_ref)//后面板频率输入
            {
                slot = rpt_content.slot_p.rb_ref - '5';

            }
            else
            {
                printf("RB up pps_stat error\n");
            }
        }
        //确认端口号
        if('3' == rpt_content.slot_o.rb_ref || '4' == rpt_content.slot_o.rb_ref)
        {

            if( rpt_content.slot[slot].ref_mod2[2] == '\0' || rpt_content.slot[slot].ref_mod2[2] == '0')
            {
                return 0;
            }
            else
            {
                port = rpt_content.slot[slot].ref_mod2[2] - '1';
            }

        }
        else
        {
            if( rpt_content.slot[slot].ref_mod1[2] == '\0' || rpt_content.slot[slot].ref_mod1[2] == '0')
            {
                return 0;
            }
            else
            {
                port = rpt_content.slot[slot].ref_mod1[2] - '1';
            }
        }

        //检查时钟质量等级
        if( rpt_content.slot[slot].ref_tl[port] != 'a')
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

}

int get_tod_sta(unsigned char *buf, unsigned char slot)
{
    //int flag_a;
    unsigned char res;
    if(slot == RB1_SLOT)
    {
        if(strncmp1(rpt_content.slot_o.rb_msmode, "MAIN", 4) == 0)
        {

            //flag_a=is_freq_input('o');
            if(buf[0] == '1')
            {
                if ( rpt_content.slot_o.rb_ref == '3' ||
                     rpt_content.slot_o.rb_ref == '4' ||
                     rpt_content.slot_o.rb_ref == '5' ||
                     rpt_content.slot_o.rb_ref == '6' ||
                     rpt_content.slot_o.rb_ref == '7' ||
                     rpt_content.slot_o.rb_ref == '8'   )

                {
					res = timeClock_2('o');
                    if( res && ( '1' == conf_content3.mcp_protocol) )
                    {
                        buf[0] = '5';//二级时钟
                    }
                    else
                    {
                        buf[0] = '1'; //正常跟踪一级时钟
                    }
                }
                else//正常跟踪卫星
                {
                    buf[0] = '0';
                }
            }
            else if(buf[0] == '2')
            {
                buf[0] = '5'; //保持
            }
            else if(buf[0] == '3')
            {
				//不可用
				if( '1' == conf_content3.mcp_protocol )
				{
					buf[0] = '7';
				}
				else
				{
					buf[0] = '2';
				}
               
               
            }
            else if(buf[0] == '4')
            {
				//XO保持
				if( '1' == conf_content3.mcp_protocol )
				{
					buf[0] = '4'; 
				}
				else
				{
					buf[0] = '3';
				}
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
    else if(slot == RB2_SLOT)
    {
        if(strncmp1(rpt_content.slot_p.rb_msmode, "MAIN", 4) == 0)
        {

            //flag_a=is_freq_input('o');
            if(buf[0] == '1')
            {
                if ( rpt_content.slot_p.rb_ref == '3' ||
                     rpt_content.slot_p.rb_ref == '4' ||
                     rpt_content.slot_p.rb_ref == '5' ||
                     rpt_content.slot_p.rb_ref == '6' ||
                     rpt_content.slot_p.rb_ref == '7' ||
                     rpt_content.slot_p.rb_ref == '8'   )
                {
					res = timeClock_2('p');
                    if( res && ( '1' == conf_content3.mcp_protocol) )
                    {
                        buf[0] = '5';//二级时钟
                    }
                    else
                    {
                        buf[0] = '1'; //正常跟踪一级时钟
                    }
                }
                else//正常跟踪卫星
                {
                    buf[0] = '0';
                }
            }
            else if(buf[0] == '2')
            {
                buf[0] = '5'; //保持
            }
            else if(buf[0] == '3')
            {
				//不可用
				if( '1' == conf_content3.mcp_protocol )
				{
					buf[0] = '7';
				}
				else
				{
					buf[0] = '2';
				}
               
            }
            else if(buf[0] == '4')
            {
				//XO保持
				if( '1' == conf_content3.mcp_protocol )
				{
					buf[0] = '4'; 
				}
				else
				{
					buf[0] = '3';
				}
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
    else
    {
        return 1;
    }
    return 0;
}

int send_tod(unsigned char *buf, unsigned char slot)
{
    unsigned char  sendbuf1[SENDBUFSIZE];
    unsigned char  sendbuf2[SENDBUFSIZE];
    unsigned char  *ctag = "000000";
    int i;
    memset(sendbuf1, '\0', SENDBUFSIZE);
    memset(sendbuf2, '\0', SENDBUFSIZE);

    if(get_tod_sta(buf, slot) == 1)
    {
        return 1;
    }

    //modified by zhanghui 2013-9-10 15:50:12

    if('2' == buf[2])
    {
        buf[2] = '3';
    }
    else if(('3' == buf[2]))
    {
        buf[2] = '2';
    }
    else
    {
        //do nothing
    }

	//buf[0]=TOD_STA;
    sprintf((char *)sendbuf1, "SET-TOD-STA:%s::0,%s;", ctag, buf);
    sprintf((char *)sendbuf2, "SET-PTP-STA:%s::%s;", ctag, buf);
    for(i = 0; i < 14; i++)
    {
        if(slot_type[i] == 'I')
        {
            sendtodown(sendbuf1, (unsigned char)(i + 0x61));
            printf("+++++tod:%s\n",sendbuf1);
        }
        else if(((slot_type[i] < 'I') && (slot_type[i] > '@')) || (slot_type[i] == 'j') )
        {
            sendtodown(sendbuf2, (unsigned char)(i + 0x61));
 			printf("-----tod:%s\n",sendbuf2);
        }
    }

    return 0;
}

void rb_do_nothing(void)
{
}

void rb_16_master(void)
{
    sendtodown("SET-MAIN:000000::MAIN;",'p');
	printf("RB auto master\n");	
}

void rb_16_salve(void)
{
	sendtodown("SET-MAIN:000000::STBY;",'p');
	printf("RB auto STBY\n");
}
void lct_set_rb(void)
{
	if(strncmp(conf_content.slot_p.msmode,"STBY",4) == 0)
	{
		sendtodown("SET-MAIN:000000::STBY;",'p');
	}
	else
	{
		sendtodown("SET-MAIN:000000::MAIN;",'p');
	}
	printf("RB LCT\n");
}

int rb_stat_index(int slot)
{
	unsigned char stat_buf[5];
	
    memset(stat_buf,0,5);
    
	if(slot == 14)
	{
		if('O' == slot_type[14])
	    {
	    	return 0;
	    }
	    else
	    {
	    	memcpy(stat_buf,rpt_content.slot_o.rb_trmode,4);
	    }
	}
	if(slot == 15)
	{
		if('O' == slot_type[15])
	    {
	    	return 0;
	    }
	    else
	    {
	    	memcpy(stat_buf,rpt_content.slot_p.rb_trmode,4);
	    }
	}
	
	if(stat_buf == NULL)
		return -1;
	
	if( strncmp(stat_buf,"FREE",4)==0 )
	{
		return 1;
	}
	else if( strncmp(stat_buf,"FATR",4)==0 )
	{
		return 2;
	}
	else if( strncmp(stat_buf,"TRCK",4)==0 )
	{
		return 3;
	}
	else if( strncmp(stat_buf,"HOLD",4)==0 )
	{
		return 4;
	}
	else
	{
		return -1;
	}
}


/*
16#RB pan state
-------------------------------------------------------------------------------
|   \  16#|                                                                    |
|15# \    |   None         FreeRun       FastTrack       Locked       HoldOver |  
|------------------------------------------------------------------------------|
|None     |   None          Master          Master       Master       Master   |
|         |                                                                    |
|FreeRun  |   None          Master          Master       Master       Master   |
|         |                                                                    |
|FastTrack|   None          slave           LCT          Master       Master   |
|         |                                                                    |
|Locked   |   None          slave           slave        LCT          slave    |
|         |                                                                    |
|HoldOver |   None          slave           slave        Master       LCT      |
-------------------------------------------------------------------------------
*/
typedef void (*Func_Matrix)(void);
void timer_rb_ms(void)
{
	signed char i,j;
	static signed char rb_stat_index_save = 0; 
    Func_Matrix rb_ms_change[5][5] = {
    								  {rb_do_nothing, rb_16_master, rb_16_master, rb_16_master, rb_16_master},
    								  {rb_do_nothing, rb_16_master, rb_16_master, rb_16_master, rb_16_master},
    								  {rb_do_nothing, rb_16_salve , lct_set_rb  , rb_16_master, rb_16_master},
    								  {rb_do_nothing, rb_16_salve , rb_16_salve , lct_set_rb  , rb_16_salve },
    								  {rb_do_nothing, rb_16_salve , rb_16_salve , rb_16_master, lct_set_rb  } 
    								 };

    i = rb_stat_index(14);
    j = rb_stat_index(15);
    
	if(i==-1 || j ==-1)
	{
		return ;
	}
	if( rb_stat_index_save == i*5+j )
	{
	    return ;	
	}
	else
	{
	    rb_stat_index_save = i*5+j;
	}
	rb_ms_change[i][j]();
}

/*
mcp定时检测控制
*/
void timer_mcp(void * loop)
{

   bool_t *loop_flag = loop;

   while(!(*loop_flag)) 
   {
        sleep(120);
        timer_rb_ms();
		
   }
   printf("The RB MasterSlave pthread exit!\n");
   pthread_exit(NULL);
	
}








