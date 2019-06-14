
#ifndef _MCP_DEF_H_
#define _MCP_DEF_H_


#define	BaseAddr1	(0x03600000)	//write
#define	BaseAddr2	(0x03600000 | 0x04000000)	//read

#define	REG16_FPGA(BaseAddr, Addr)       (BaseAddr + Addr)

#define ALMControl   REG16_FPGA(BaseAddr1,0x0022)

#define FPGA_READ   0x01
#define FPGA_WRITE  0x02

#define WdControl   REG16_FPGA(BaseAddr1,0x0020)
#define IndicateLED   REG16_FPGA(BaseAddr1,0x0030)



#define	TCP_NETWORK_CARD		"eth0"	//网卡


typedef unsigned char	Uint8;
typedef unsigned short 	Uint16;
typedef unsigned long	Ulong32;

typedef short int       Int16;
typedef char            Int8;
typedef long			Long32;

typedef float			Float32;
typedef double		Double64;



unsigned char mcp_date[11];
unsigned char mcp_time[9];
unsigned char password[9];

unsigned short atag;

unsigned char slot_type[24];
unsigned char fpga_ver[7];
int online_sta[24];
//
//! ref pge4s alarm 0x50.
//
int flag_alm[36][26];		  //20:ref1   21:ref2  22-35 PGE4S 
unsigned char alm_sta[37][14];//18:ref1   19:ref2  23-36 PGE4S
int flag_i_o;
int flag_i_p;

int timeout_cnt[MAX_CLIENT];

unsigned char saved_alms[SAVE_ALM_NUM][50];
int saved_alms_cnt;
int time_temp;

fd_set rset, allset;
int client_fd[MAX_CLIENT];
extern int qid;    /*消息队列标识符*/

int listenfd, maxfd, connectfd, sockfd;
int client_num, maxi, nready, count_data;
int sin_size; //地址信息结构体大小
int client_num;

unsigned char event_old[17][128];

int g_DrvLost;
int g_LeapFlag;
extern int _CMD_NODE_MAX;
extern int _NUM_RPT100S;
extern int NUM_REVT;

#endif

