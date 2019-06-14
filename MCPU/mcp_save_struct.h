#ifndef _MCP_SAVE_STRUCT_H_
#define _MCP_SAVE_STRUCT_H_

#pragma pack(1)

typedef struct
{
    unsigned char sy_ver[40];
    unsigned char tp16_en[17];
    unsigned char tp16_tod[4];

    unsigned char out_lev[3];
    unsigned char out_typ[17];
    unsigned char out_sta;
    unsigned char out_mode;


    unsigned char ptp_ip[4][16];
    unsigned char ptp_mac[4][18];
    unsigned char ptp_gate[4][16];
    unsigned char ptp_mask[4][16];
    unsigned char ptp_dns1[4][16];
    unsigned char ptp_dns2[4][16];

    /*<eable>,<delaytype>,<multicast>,<enp>,<step>,
    <sync>,<announce>,<delayreq>,<pdelayreq>,
    <delaycom>,<linkmode>
    delete  the port*/
    unsigned char ptp_en[4];
    unsigned char ptp_delaytype[4];
    unsigned char ptp_multicast[4];
    unsigned char ptp_enp[4];
    unsigned char ptp_step[4];
    unsigned char ptp_sync[4][3];
    unsigned char ptp_announce[4][3];
    unsigned char ptp_delayreq[4][3];
    unsigned char ptp_pdelayreq[4][3];
    unsigned char ptp_delaycom[4][9];
    unsigned char ptp_linkmode[4];
    unsigned char ptp_out[4];
	unsigned char ptp_clkcls[4][4];
	//unsigned char ptp_dom[4];

    unsigned char ref_prio1[9];
    unsigned char ref_prio2[9];
    unsigned char ref_mod1[4];
    unsigned char ref_mod2[4];

    unsigned char ref_ph[128];
    unsigned char ref_sa[9];
    unsigned char ref_typ1[9];
    unsigned char ref_typ2[9];
    unsigned char ref_tl[9];
    unsigned char ref_en[9];
    unsigned char ref_ssm_en[9];

    unsigned char rs_en[5];
    unsigned char rs_tzo[5];

    unsigned char ppx_mod[17];

    unsigned char tod_en[17];
    unsigned char tod_br;
    unsigned char tod_tzo;

    unsigned char igb_en[5];
    unsigned char igb_rat[5];
    unsigned char igb_max[5];
    unsigned char igb_tzone;

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
    unsigned char ntp_ms[26];
	unsigned char ntp_leap[4];
	unsigned char ntp_mcu[2];
	unsigned char ntp_pmac[12][6];
	unsigned char ntp_pip[12][5];
	unsigned char ntp_pnetmask[12][4];
	unsigned char ntp_pgateway[12][4];
    unsigned char ntp_portEn[5];
    unsigned char ntp_portStatus[5];
    unsigned char ref_2mb_lvlm;
} rpt_out_content;

typedef struct
{
    /*	<responsemessage>::=“<aid>,<type>,[<version-MCU>],[<version-FPGA>],
    [<version-CPLD>],[<version-PCB>],<ref>,<priority>,<mask>,<leapmask>,<leap>,<leaptag>,
    [<delay>],[<delay>],[<delay>],[<delay>],[<delay>],[<delay>],[<delay>],[<delay>],
    <we>,<out>,<type-work>,<msmode>,<trmode>”*/
    unsigned char rb_ref;
    unsigned char rb_prio[9];
    unsigned char rb_mask[9];
    unsigned char rb_leapmask[6];
    unsigned char rb_leap[3];
    unsigned char rb_leaptag;
    unsigned char rf_dey[8][9];/*<deley>X8*/
    unsigned char rf_tzo[4];/*<we>,<out>*/
    unsigned char rb_typework[8];
    unsigned char rb_msmode[5];
    unsigned char rb_trmode[5];
    unsigned char sy_ver[40];
    unsigned char rb_inp[10][128];
    unsigned char rb_tl;
    unsigned char rb_sa;
    unsigned char rb_tl_2mhz;

    unsigned char tod_buf[4];
	unsigned char rb_phase[2][9];
	unsigned char rb_ph[7][10];// #17 #18 #1 time #2 time #1 fre #2 fre
	
} rpt_rb_content;

typedef struct
{
    /*<response message >::= “
    <aid>,<type>,[<version-MCU>],[<version-FPGA>], [<version-CPLD>],[<version-PCB>],
    <ppssta>,<sourcetype>,<pos_mode>,<elevation>, <format>, <acmode>, <mask>,
    <qq-fix>,<hhmmss.ss>,<ddmm.mmmmmm>,<ns>,<dddmm.mmmmmm>,<we>,<saaaaa.aa>,
    <vvvv>,<tttttttttt>[,<sat>,<qq>,<pp>,<ss>,<h>]*”*/
    unsigned char sy_ver[40];
    unsigned char gb_sta;
    unsigned char gb_tod[4];
    unsigned char gb_pos_mode[5];
    unsigned char gb_elevation[9];
    unsigned char gb_format[6];
    unsigned char gb_acmode[4];
    unsigned char gb_mask[3];/*<mask>*/
    unsigned char gb_ori[70];/*<qq>,<hhmmss.ss>,<ddmm.mmmmmm>,<ns>,<dddmm.mmmmmm>,<we>,<saaaaa.aa>,
	<vvvv>,<tttttttttt>*/
    //unsigned char gb_rev[256];
    unsigned char gb_rev_g[256];	/*<sat>,<qq>[,<pp>,<ss>,<h>]**/
    unsigned char gb_rev_b[256];
	unsigned char gb_rev_glo[256];	/*<sat>,<qq>[,<pp>,<ss>,<h>]**/
    unsigned char gb_rev_gal[256];
    unsigned char bdzb[8];
	unsigned char staen;
	unsigned char rec_type[12];
	unsigned char rec_ver[40];
	unsigned char leapnum[4];
	unsigned char leapalm[3];
	unsigned char delay[8];
	unsigned char pps_out_delay[16];
	
} rpt_gbtp_content;

typedef struct
{
    rpt_out_content slot[14];
    rpt_rb_content slot_o, slot_p;
    rpt_gbtp_content slot_q, slot_r;
} rpt_all_content;


typedef struct
{
    unsigned char ptp_prio1[14][4][5];//14个槽位，4个端口，5个字节保存信息。
    unsigned char ptp_prio2[14][4][5];
    unsigned char ptp_mtc_ip[14][4][152];//保存单播ip
    unsigned char ptp_dom[14][4][4];//20170616 add
    unsigned char esmc_en[14][4];
	unsigned char ptp_ver[14][4][10];
	unsigned char sfp_type[14][4][3];
	unsigned char sfp_oplo[14][4][7];
	unsigned char sfp_ophi[14][4][7];
	unsigned char time_source[14][4][5];
	unsigned char clock_class[14][4][4];
	unsigned char time_acc[14][4][5];
	unsigned char synce_ssm[14][4][14];
} file_ptp_mtc; //第一部分配置信息由file_content保存，第二部分为扩展框，此为第三部分。


#pragma pack()

rpt_all_content rpt_content;
file_ptp_mtc rpt_ptp_mtc;
//
//define esmc enable,ptp_domain.
//
//unsigned char esmc_en[14][4];
//unsigned char ptp_dom[14][4][4];

void sta1_alm(unsigned char *sendbuf, unsigned char slot);
void sta1_portnum(unsigned char *sendbuf, unsigned char slot);
void sta1_net(unsigned char *sendbuf, unsigned char slot);
void sta1_cpu(unsigned char *sendbuf, unsigned char slot);
void sta1_in_out(unsigned char *sendbuf, unsigned char slot);

void set_sta_ip(unsigned const char *ip, unsigned char solt);
void set_sta_gateway(unsigned char *ip, unsigned char solt);
void set_sta_netmask(unsigned char *ip, unsigned char solt);
//extern int _NUM_REVT; 


#endif

