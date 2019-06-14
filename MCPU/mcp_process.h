
#ifndef _MCP_PROCESS_H_
#define _MCP_PROCESS_H_


#include "ext_type.h"
#include "mcp_main.h"



extern int shake_hand_count;
extern int reset_network_card;


int send_tod(unsigned char *buf, unsigned char slot);
int get_tod_sta(unsigned char *buf, unsigned char slot);

extern int strncmp1(unsigned char *p1, unsigned char *p2, unsigned char n);
extern void strncpynew(unsigned char *str1, unsigned char *str2, int n);
extern void strncatnew(unsigned char *str1, unsigned char *str2, int n);





int current_client;
int g_lev;

int init_timer();
void init_pan(char slot);
int judge_length(unsigned char *buf);
void sendto_all_client(unsigned char *buf);
int judge_client_online(int *clt_num);
void judge_data(unsigned char *recvbuf, int len , int client);
int judge_buff(unsigned char *buf);

int IsLeapYear(int year);

void init_var(void);

int Open_FpgaDev(void);
void Close_FpgaDev(void);
int FpgaRead( const long paddr, unsigned short *DstAddr);
int FpgaWrite( const long paddr, unsigned short value);

int process_config();

int process_client(int client, char *recvbuf, int len);   //客户请求处理函数

int process_report(unsigned char *recv, int len);
int process_ralm(unsigned char *recv, int len);
int process_revt(unsigned char *recv, int len);
void rpt_alm_to_client(char slot, unsigned char *data);
void send_online_framing(int num, char type);
void get_alm(char slot, unsigned char *data);
int get_time(unsigned char *data);
int get_fpga_ver(unsigned char *data);

int send_pullout_alm(int num, int flag_cl);

void get_time_fromnet();

void get_data(char slot, unsigned char *data);

void rpt_rb(char slot, unsigned char *data);
void rpt_gbtp(char slot, unsigned char *data);
void rpt_out(char slot, unsigned char *data);
void rpt_mcp(unsigned char *data);
void rpt_test(char slot, unsigned char *data);
void rpt_drv(char slot, unsigned char *data);




void send_alm(char slot, int num, char type, int flag_cl);
void save_alm(char slot, unsigned char *data, int flag);
void rpt_alm_framing(char slot, unsigned char *ntfcncde,
                     unsigned char *almcde, unsigned char *alm);
void get_gbtp_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);
void get_gtp_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);
void get_btp_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);

void get_rb_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);
void get_TP16_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);
void get_OUT16_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);
void get_PTP_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);

void get_PGEIN_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);
void get_PGE4V_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);//
void get_tod_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);
void get_ntp_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);
void get_drv_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);
void get_mge_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);


void get_MCP_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);
void get_REF_alm_type(unsigned char *ntfcncde, unsigned char *almcde, int num);

int cmd_process(int client, _MSG_NODE *msg);
void respond_success(int client, unsigned char *ctag);
void respond_fail(int client, unsigned char *ctag, int type);
int get_format(char *format, unsigned char *ctag);
int if_a_string_is_a_valid_ipv4_address(const char *str);



void Init_filesystem();
void config_enet();

extern void save_config();

extern int API_Set_McpIp(unsigned char  *mcp_local_ip);
extern int API_Set_McpMask(unsigned char  *mcp_local_mask);
extern int API_Set_McpGateWay(unsigned char *mcp_local_gateway);
extern int API_Set_McpMacAddr(unsigned char *mcp_local_mac);
extern int API_Get_McpIp(unsigned char  *mcp_local_ip);
extern int API_Get_McpMask(unsigned char  *mcp_local_mask);
extern int API_Get_McpGateWay(unsigned char  *mcp_local_gateway);
extern int API_Get_McpMacAddr(unsigned char  *mcp_local_mac);



extern void sendtodown(unsigned char *buf, unsigned char slot);
void rpt_event_framing(unsigned char *event, char slot,
                       int flag_na, unsigned char *data, int flag_sa, unsigned char *reason);
void send_out_lev(int flag_lev, int num);
int get_freq_lev(int lev);
int get_out_now_lev(unsigned char *parg);
void get_out_now_lev_char(int lev );
void get_out_lev(int num, int flag);
int get_ref_tl(unsigned char c);
int send_ref_lev(int num);
int file_exists(char *filename);

int msgQ_create( const char *pathname, char proj_id );
void sendtodown_cli(unsigned char *buf, unsigned char slot, unsigned char *ctag);








int ext_issue_cmd(u8_t *buf, u16_t len, u8_t slot, u8_t *ctag);
void get_ext1_eqt(char slot, unsigned char *data);
void get_ext2_eqt(char slot, unsigned char *data);
void get_ext3_eqt(char slot, unsigned char *data);

void get_ext_alm(char slot, unsigned char *data);
void get_ext1_alm(char slot, unsigned char *data);
void get_ext2_alm(char slot, unsigned char *data);
void get_ext3_alm(char slot, unsigned char *data);

int ext_alm_cmp(u8_t ext_sid, u8_t *ext_new_data, struct extctx *ctx);
int ext_alm_evt(u8_t eid, u8_t sid, u8_t almid, u8_t *agcc1, u8_t *agcc2);
int ext_bpp_cfg(u8_t ext_sid, u8_t *ext_new_type, struct extctx *ctx);
void ext_bpp_evt(int aid, char bid);

int ext_drv_mgr_evt(struct extctx *ctx);

int ext_bid_cmp(u8_t ext_sid, u8_t *ext_new_data, struct extctx *ctx);

void ext_ssm_cfg(char *pSsm);


void mcpd_reset_network_card(void);

void timer_mcp(void * loop);
int Create_Thread(bool_t * loop);

void get_exta1_eqt(char slot, unsigned char *data);
void get_exta2_eqt(char slot, unsigned char *data);
void get_exta3_eqt(char slot, unsigned char *data);








#endif

