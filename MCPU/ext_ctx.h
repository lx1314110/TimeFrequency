#ifndef	__EXT_CTX__
#define	__EXT_CTX__







#include "ext_type.h"







#define	EXT_DRV_NUM		2
#define	EXT_MGR_NUM		6
#define	EXT_OUT_NUM		30
#define	EXT_GRP_NUM		3


#define	EXT_1_OUT_LOWER		23
#define	EXT_1_OUT_UPPER		32

#define	EXT_1_PWR1			35
#define	EXT_1_PWR2			36

#define	EXT_1_MGR_PRI		37
#define	EXT_1_MGR_RSV		38

#define	EXT_1_OUT_OFFSET	(EXT_1_OUT_LOWER)

#define	EXT_2_OUT_LOWER		39
#define	EXT_2_OUT_UPPER		48

#define	EXT_2_PWR1			51
#define	EXT_2_PWR2			52

#define	EXT_2_MGR_PRI		53
#define	EXT_2_MGR_RSV		54

#define	EXT_2_OUT_OFFSET	(EXT_2_OUT_LOWER - 10)

#define	EXT_3_OUT_LOWER		55
#define	EXT_3_OUT_UPPER		66//58，59不使用

#define	EXT_3_PWR1			67
#define	EXT_3_PWR2			68

#define	EXT_3_MGR_PRI		69
#define	EXT_3_MGR_RSV		70

#define	EXT_3_OUT_OFFSET	(EXT_3_OUT_LOWER - 20)








#define	EXT_SAVE_PATH	"/var/p300file"
#define	EXT_SAVE_ADDR	0x1F0000








enum
{
    EXT_NONE	= 'O',
    EXT_PWR		= 'M',
    EXT_MGR		= 'f',
    EXT_OUT16	= 'J',
    EXT_OUT32	= 'a',
    EXT_DRV		= 'd',
    EXT_OUT32S	= 'h',
    EXT_OUT16S	= 'i'
};












struct extdrv
{
    /*
      1 主DRV
      2 备DRV
      3 通信故障
      4 未连接
    */
    u8_t drvPR[EXT_GRP_NUM];

    /*
      DRV和MGR之间的连接状态，1个字节
      1	未连接
      0	已连接
      0100Y3Y2Y1Y0
      Y0-Y2，分别表示DRV和MGR(1-3)之间的连接状态
    */
    u8_t drvAlm;

    /*版本信息*/
    u8_t drvVer[23];
};







struct extmgr
{
    /*
      1	主MGR
      2	备MGR
    */
    u8_t mgrPR;

    /*
      MGR和输出板卡之间的通讯告警，3个字节
      1	告警
      0	正常
      0100Y3Y2Y1Y0 0100Y7Y6Y5Y4 0100Y11Y10Y9Y8
      Y0-Y9，分别表示MGR和输出板卡(1-10)之间的通讯告警
      Y10	表示PWR1告警
      Y11	表示PWR2告警
      Y12	1槽位拔盘告警
      Y13	2槽位拔盘告警
      Y14	3槽位拔盘告警
      Y15	4槽位拔盘告警
      Y16	5槽位拔盘告警
      Y17	6槽位拔盘告警
      Y18	7槽位拔盘告警
      Y19	8槽位拔盘告警
      Y20	9槽位拔盘告警
      Y21	10槽位拔盘告警
      Y22	11槽位拔盘告警
      Y24	12槽位拔盘告警
      Y24	13槽位拔盘告警
      Y25	14槽位拔盘告警
    */
    u8_t mgrAlm[8];

    /*版本信息*/
    u8_t mgrVer[23];
};







struct extout
{
    /* 时钟等级 */
    u8_t outSsm[3];

    /*
      信号类型
      0：2.048Mhz
      1：2.048Mbit/s
      2：无输出
    */
    u8_t outSignal[17];

    /*
      第一个字节表示<mode>，第二个字节表示<out-sta>
      <mode> ::= 0|1，普通模式|主备模式
      <out-sta> ::= 0|1，主用|备用
    */
    u8_t outPR[4];

    /*
      告警，6个字节
      0100Y3Y2Y1Y0 0100Y7Y6Y5Y4 0100Y11Y10Y9Y8
      0100Y15Y14Y13Y12 0100Y19Y18Y17Y16 0100Y23Y22Y21Y20
      Y0		主钟10M输入告警
      Y1		备钟10M输入告警
      Y2~Y17	1~16（路/组）输出告警
    */
    u8_t outAlm[7];

    /*版本信息*/
    u8_t outVer[23];
};








struct extsave
{
    /*
      'E'	FLASH已保存配置
    */
    u8_t flashMark;

    /*
      1 主DRV
      2 备DRV
    */
    u8_t drvPR[EXT_DRV_NUM][EXT_GRP_NUM];

    /*
      1	主MGR
      2	备MGR
    */
    u8_t mgrPR[EXT_MGR_NUM];

    /* 时钟等级 */
    u8_t outSsm[EXT_OUT_NUM][3];

    /*
      信号类型
      0：2.048Mhz
      1：2.048Mbit/s
      2：无输出
    */
    u8_t outSignal[EXT_OUT_NUM][17];

    /*
      第一个字节表示<mode>，第二个字节表示<out-sta>
      <mode> ::= 0|1，普通模式|主备模式
      <out-sta> ::= 0|1，主用|备用
    */
    u8_t outPR[EXT_OUT_NUM][4];
    unsigned char onlineSta;
};








struct extctx
{
    //ext save
    struct extsave save;

    // MCP和DRV的通信告警
    // 1	告警
    // 0	正常
    u8_t extMcpDrvAlm[2];

    /* 2张DRV */
    struct extdrv drv[EXT_DRV_NUM];

    /* 板卡类型，O 无盘，M PWR，f MGR，J OUT16，a OUT32 */
    u8_t extBid[EXT_GRP_NUM][15];

    /* 6张MGR */
    struct extmgr mgr[EXT_MGR_NUM];

    /* 30张输出板卡 */
    struct extout out[EXT_OUT_NUM];

    u8_t old_drv_mgr_sta[32];
    u8_t new_drv_mgr_sta[32];
};








#endif//__EXT_CTX__


