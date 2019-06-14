
#ifndef _MCP_SET_STRUCT_H_
#define _MCP_SET_STRUCT_H_


typedef struct
{
    unsigned char ip[16];
    unsigned char mask[16];
    unsigned char gate[16];
    unsigned char dns1[16];
    unsigned char dns2[16];
    unsigned char mac[18];
    unsigned char leap_num[3];
    unsigned char tid[17];
    unsigned char time_source; //MCP盘时间源
    unsigned char msmode[5];   //MCP主备用状态

    unsigned char tl[3];
    unsigned char fb;
    /*'0' / '1' 默认为'1'(打开)*/
    unsigned char out_ssm_en;

    /*默认为0x04*/
    unsigned char out_ssm_oth;
} mcp_content;

typedef struct
{
    unsigned char mode[4];
    unsigned char mask[3];
    unsigned char pos[35];
    unsigned char bdzb[8];
	//unsigned char delay[8];
	//unsigned char out_pps_delay[8];
} gbtp_content;

typedef struct
{
    unsigned char msmode[5];
    unsigned char sys_ref;
    unsigned char dely[4][9];
    unsigned char priority[9];//the front of reference source priority.
    unsigned char mask[9];
    unsigned char leapmask[6];

    unsigned char tzo[4];
    unsigned char leaptag;
	unsigned char thresh[2][9];
} rb_content;

typedef struct
{
    unsigned char ptp_type;
    unsigned char out_lev[3];
    unsigned char ptp_linkmode[4];
    unsigned char ptp_enp[4];
    unsigned char ptp_step[4];
    unsigned char ptp_en[4];
    unsigned char ptp_delaytype[4];
    unsigned char ptp_multicast[4];
    unsigned char out_typ[17];
    unsigned char out_mod;
    unsigned char tp16_en[17];
    unsigned char ptp_sync[4][3];
    unsigned char ptp_announce[4][3];
    unsigned char ptp_delayreq[4][3];
    unsigned char ptp_pdelayreq[4][3];
    unsigned char ptp_delaycom[4][9];
    unsigned char ptp_ip[4][16];
    unsigned char ptp_gate[4][16];
    unsigned char ptp_mask[4][16];
    unsigned char ptp_dns1[4][16];
    unsigned char ptp_dns2[4][16];
    unsigned char ptp_mac[4][18];
    unsigned char ptp_out[4];
    unsigned char ref_prio1[9];
    unsigned char ref_prio2[9];
    unsigned char ref_mod1[4];
    unsigned char ref_mod2[4];

    unsigned char ref_in_ssm_en[9];
	unsigned char ref_2mb_lvlm;
    unsigned char rs_en[5];
    unsigned char rs_tzo[5];

    unsigned char ppx_mod[17];

    unsigned char tod_en[17];
    unsigned char tod_br;
    unsigned char tod_tzo;

    unsigned char igb_en[5];
    unsigned char igb_rat[5];
    unsigned char igb_max[5];

    unsigned char ntp_ip[9];
    unsigned char ntp_mac[13];
    unsigned char ntp_gate[9];
    unsigned char ntp_msk[9];

    unsigned char ntp_bcast_en;
    unsigned char ntp_tpv_en;
    unsigned char ntp_md5_en;
    unsigned char ntp_ois_en;
    unsigned char ntp_interval[5];

    unsigned char ntp_key_flag[4];
    unsigned char ntp_keyid[4][5];
    unsigned char ntp_key[4][17];

    /*unsigned char out_type;*/
} out_content;

#define alm_msk_total 50

struct ALM_MSK
{
    unsigned char data[24];
    unsigned char type; //type 类型，拼写错了
};


typedef struct
{
    //'M'表示保存过配置
    u8_t flashMask;

    //所有要保存设置的信息
    mcp_content slot_u;
    gbtp_content slot_q, slot_r;
    rb_content slot_o, slot_p;
    out_content slot[14];
    struct ALM_MSK alm_msk[alm_msk_total];
} file_content;
file_content conf_content;

typedef struct
{
    unsigned char ip[9];
    unsigned char gate[9];
    unsigned char mask[9];
    unsigned char dns1[9];
    unsigned char dns2[9];
    unsigned char mac[18];
} STR_STA1;

typedef struct
{
    u8_t flashMask;
    unsigned char ptp_prio1[14][4][5]; //14个槽位，4个端口，5个字节保存信息。
    unsigned char ptp_prio2[14][4][5];
    unsigned char LeapMod;
	unsigned char ptp_mtc_ip[14][4][9];//保存单播ip
	unsigned char ptp_dom[14][4][4];   //20170616新添时钟域号保存
	unsigned char ptp_esmcen[14][4];   //ptp esmcen .
	unsigned char ptp_sfp[14][4][3];
	unsigned char ptp_oplo[14][4][7];
	unsigned char ptp_ophi[14][4][7];
    unsigned char irigb_tzone[14]; 
    STR_STA1 sta1;
	unsigned char mcp_protocol;
	unsigned char gbtp_delay[2][8];
	unsigned char gbtp_pps_delay[2][16];
	unsigned char pps_sel[2];
	unsigned char tdev_en[2];
	unsigned char ptp_mtc1_ip[14][4][152];
	//unsigned char rbb_priority[9];    //#sb the back of reference source priority.
	
} file_content3; //第一部分配置信息由file_content保存，第二部分为扩展框，此为第三部分。
file_content3 conf_content3;

#define ALM_MASK_LEN   		6
#define MAX_PORT_NUM   		52
#define MAX_MAC_LEN    		12
#define MAX_IP_LEN     		8
#define FRAME_FRE_LEN  		2
#define DELAY_COM_LEN  		7
#define MAX_PRIO_LEN   		4
#define MAX_DOM_LEN    		3
#define MAX_OPTHR_LEN  		6
#define MAX_TIMES_LEN  		4
#define MAX_CLKCLS_LEN 		3
#define MAX_SYNCSSM_LEN 	13

#define PGE4S_ALM5_START    23
#define PGE4S_TALM5_START   22


#endif
