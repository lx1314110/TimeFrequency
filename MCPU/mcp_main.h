
#ifndef _MCP_MAIN_H
#define  _MCP_MAIN_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
//#include <linux/if.h>
#include <net/route.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include <fcntl.h>

#include <errno.h>
#include "version.h"




#define PORT 5000               //服务器端口  
#define BACKLOG 5               //listen队列中等待的连接数  
#define MAXRECVDATA 1024
#define MAXDATASIZE 256        //缓冲区大小 
#define SENDBUFSIZE 168
//#define _CMD_NODE_MAX 70    //命令条数//modified 2013-4-27
//#define _CMD_NODE_MAX ((int)(sizeof(g_cmd_fun)/sizeof(_CMD_NODE)))
#define PROJ_ID 32
#define PATH_NAME	"/var/"
#define MAX_CLIENT 11

//#define _NUM_RPT100S 45  //
#define _NUM_REVT  48      //

#define SAVE_ALM_NUM 50

#define CONFIG_FILE "/usr/config.txt"

#define CR 0x0D       //回车
#define LF 0x0A       //换行
#define SP 0x20	//空格

#define MCP_SLOT 0x75 //即u
#define GBTP1_SLOT 0x71 //q
#define GBTP2_SLOT 0x72 //r
#define RB1_SLOT 0x6f     //o
#define RB2_SLOT 0x70     //p

#ifndef INT_EXTINT0
#define INT_EXTINT0     0x00000000
#endif
#ifndef INT_EXTINT1
#define INT_EXTINT1     0x00000001
#endif

#define FPGA_DEV "/dev/p210fpga"
#define FILE_DEV "/var/p300file"

#define NO_INFO 0
#define DEBUG_NET_INFO 1
#define API_DEF 1
#define NET_INFO	"<NET-INFO>"
#define NET_SAVE	"<NET-SAVE>"

#define TYPE_MN "MN"
#define TYPE_MJ "MJ"
#define TYPE_CL "CL"
#define TYPE_CR "CR"
#define TYPE_WA "WA"

#define TYPE_MN_A "* "
#define TYPE_MJ_A "**"
#define TYPE_CL_A "A "
#define TYPE_CR_A "*C"
#define TYPE_WA_A "*W"

#define COMMA_1 ","
#define COMMA_3 ",,,"
#define COMMA_7 ",,,,,,,"
#define COMMA_8 ",,,,,,,,"
#define COMMA_9 ",,,,,,,,,"



#define GBTP_G "G,00"
#define GBTP_B "B,00"
#define GBTP_GPS "GPS,00"
#define GBTP_BD  "BD,00"
#define GBTP_GLO "GLO,00"
#define GBTP_GAL "GAL,00"

struct mymsgbuf
{
    long msgtype;
    unsigned char slot;
    int len;
    unsigned char ctag[7];
    unsigned char ctrlstring[SENDBUFSIZE];
};


//网管下发命令结构体
typedef struct
{
    unsigned char cmd[16];
    int cmd_len;
    unsigned char solt;
    unsigned char ctag[7];
    unsigned char data[MAXDATASIZE];
} _MSG_NODE, *P_MSG_NODE;

typedef struct
{
    unsigned char cmd[14];
    int cmd_len;
    int  (*cmd_fun)(_MSG_NODE *, int);
    int  (*rpt_fun)(int , _MSG_NODE * );
} _CMD_NODE, *P_CMD_NODE;

typedef struct
{
    //
    //!RPT100S cmd 增加到12
    //
    unsigned char cmd[12];
    int cmd_len;
    unsigned char cmd_num[4];
    int  (*rpt_fun)(char , unsigned char *);
} _RPT100S_NODE;

typedef struct
{
    unsigned char cmd_num[4];
    int  (*rpt_fun)(char , unsigned char *);
} _REVT_NODE;

extern _CMD_NODE g_cmd_fun[];
extern _RPT100S_NODE RPT100S_fun[];
extern _REVT_NODE REVT_fun[];
extern int print_switch;
extern char TOD_STA;
#endif

