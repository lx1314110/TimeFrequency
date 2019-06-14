#include <sys/resource.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>


//#include <linux/tcp.h>
#include <netinet/tcp.h>


#include "ext_alloc.h"
#include "ext_global.h"
#include "mcp_main.h"
#include "mcp_process.h"
#include "mcp_def.h"
#include "mcp_save_struct.h"
#include "mcp_set_struct.h"
//#include "memwatch.h"
//#include "memwatch.h"




int qid;
int print_switch = 0;
char TOD_STA = '2';
static bool_t Loop_Mark = false;

void log()
{
    printf("****************************************************\n");
    printf("*                                                  *\n");
    printf("*                                                  *\n");
    printf("*                P300 TF EXT                       *\n");
    printf("*                                                  *\n");
    printf("*v%s.%s.%s.%s_%c                        \n",VER_MAJOR,VER_MINOR,VER_REVIS,VER_DATE,VER_STATE);
    printf("*                                                  *\n");
    printf("****************************************************\n");
	
}

void black_door(int sig)
{
	printf("+++++signal is %d\n",sig);
	TOD_STA = sig+'0';
}


int main(int argc , char **argv)
{
    //
    //!Socket Attribute Setting. 
    //
    int opt = 1;
    //
    //!Bind  The  Local IP and Port Parameters.
    //
    struct sockaddr_in server;
    //
    //!Process pid Getting.
    //
    pid_t pid;
	//
	//! Priority Setting.
	//
    int prio;

    int i, j, len_rpt;
    ssize_t len;
    int tmp, temp;
	
    //
    //! Tcp Receive Buffer and Data Swap Buffer. 
    unsigned char recvbuf[MAXRECVDATA];
    unsigned char rpt_buf[MAXDATASIZE];

    //const char chOpt=1;
    //int nErr;
    struct timeval timeout = {0, 100000};

    //int flag_slotOIsMaster =0; //2014-1-27 rb备用板是否为主用状态；1启用备槽
    printf("sizeof(file_content)=%d\n", (int)sizeof(file_content));
	printf("sizeof(file_content3)=%d\n",(int)sizeof(file_content3));
	
	//
    //Get self-process pid. 
    //
    pid = getpid();
	//
	//Set self-process priority.
	//
    if (setpriority(PRIO_PROCESS, pid, -12) != 0)
    {
        printf("%s%s\n", "MCPU", "Can't set ntp's priority!");
    }

	//
	//Record The Last error.
    errno = 0;

	//
	//!Get Self-Process Priority.
    prio = getpriority(PRIO_PROCESS, pid);
    if ( 0 == errno )
    {
        printf("MCPU priority is %d\n", prio);
    }
	
    FpgaWrite( WdControl, 0x00 );
    FpgaWrite( WdControl, 0x01 );
    log();

	
    Init_filesystem();
    initializeContext(&gExtCtx);
    init_var();
    config_enet();
    Open_FpgaDev();
    //
    //Timer 120s Check The Setting or not Return Corrected.
    init_timer();

    qid = msgQ_create(PATH_NAME, PROJ_ID);
	
	//
	//! Create The Thread of deal RB Master/Slave inter 60 s.
	//
	if(Create_Thread(&Loop_Mark) == false)
	{
		printf("<MCPU ERR> %s\n", "Create RB Mastr Slave Thread Failed!");
		return(-1);
	}
#if 0
if(SIG_ERR ==  signal(1,black_door))
	{
		perror("++++++signal1\n");
	}
	if(SIG_ERR ==  signal(2,black_door))
	{
		perror("++++++signal2\n");
	}
	if(SIG_ERR ==  signal(3,black_door))
	{
		perror("++++++signal3\n");
	}
	if(SIG_ERR ==  signal(4,black_door))
	{
		perror("++++++signal4\n");
	}
	if(SIG_ERR ==  signal(5,black_door))
	{
		perror("++++++signal5\n");
	}
	if(SIG_ERR ==  signal(6,black_door))
	{
		perror("++++++signal6\n");
	}
	if(SIG_ERR ==  signal(7,black_door))
	{
		perror("++++++signal7\n");
	}
#endif	
#if DEBUG_NET_INFO
    printf("%s:init completed,create socket...\n", NET_INFO);
#endif


    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Creating socket failed.");
        exit(1);
    }

    //允许多客户端连接
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));       //设置socket属性
    setsockopt(listenfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(listenfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    // if(nErr==-1)
    //{
    // perror("TCP_NODELAY error\n");
    //return ;
    //}

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    // server.sin_addr.s_addr = inet_addr("192.168.1.10");
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listenfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1)
    {
        //调用bind绑定地址
        perror("Bind error.");
        exit(1);
    }

    if (listen(listenfd, BACKLOG) == -1)
    {
        //调用listen开始监听
        perror("listen() error\n");
        exit(1);
    }

#if DEBUG_NET_INFO
    printf("%s:listening...\n", NET_INFO);
#endif

    //
    //!初始化select
    //
    maxfd = listenfd;
    maxi = -1;
    for(client_num = 0; client_num < MAX_CLIENT; client_num++)
    {
        client_fd[client_num] = -1;
    }
	//
	//!
	//
    FD_ZERO(&allset);           //清空
    FD_SET(listenfd, &allset);  //将监听socket加入select检测的描述符集合
    print_switch = 1;
	
	
    while (1)
    {
        struct sockaddr_in addr;
        rset = allset;
		
		//  int select(int nfds, fd_set *readfds, fd_set *writefds,fd_set *exceptfds, struct timeval *timeout);				  
		//! timeout == NULL:block the tcp connected.
        nready = select(maxfd + 1, &rset, NULL, NULL, NULL);    //调用select

        //
        //! select error,continue;
		//if(nready <= 0)
        //{
        //    perror("select error\n");    
        //    continue;
        //}
		//else 
		if (FD_ISSET(listenfd, &rset))  						//检测是否有新客户端请求
        {

		   //fprintf(stderr,"f:Accept a connection.\n");
            sin_size = sizeof(struct sockaddr_in);
            if ((connectfd =  accept(listenfd, (struct sockaddr *)&addr, (socklen_t *) & sin_size)) == -1)
            {
                continue;
            }

            //将新客户端的加入数组
            for (client_num = 0; client_num < MAX_CLIENT; client_num++)
            {
                if (client_fd[client_num] < 0)
                {
                    client_fd[client_num] = connectfd; //保存客户端描述符

                    //if(client_num == 0)
                    //{
                        //init_pan();
                    //}
#if DEBUG_NET_INFO
                    if(print_switch == 0)
                    {
                        printf("%s:Got a new Client<%d>\n", NET_INFO, client_num);
                    }
#endif
                 break;
                }
            }

            if (client_num >= MAX_CLIENT)
            {
                printf("Too many clients\n");
				
                close(connectfd);
            }
            else
            {
                FD_SET(connectfd, &allset);
                if (connectfd > maxfd)
                {
                    maxfd = connectfd;    //确认maxfd是最大描述符
                }
                if (client_num > maxi)    //数组最大元素值
                {
                    maxi = client_num;
                }
                if (--nready <= 0)
                {
                    continue;
                }
            }//如果没有新客户端连接，继续循环
        }

        for (client_num = 0; client_num <= maxi; client_num++)
        {
            if ((sockfd = client_fd[client_num]) < 0)    //如果客户端描述符小于0，则没有客户端连接，检测下一个
            {
                continue;
            }

            // 有客户连接，检测是否有数据
            if (FD_ISSET(sockfd, &rset))
            {
                memset(recvbuf, '\0', MAXRECVDATA);
				//len = recv(sockfd, recvbuf, MAXDATASIZE, 0);
                if ((len = recv(sockfd, recvbuf, MAXDATASIZE, 0)) <= 0)
                {				
					close(sockfd);				
					FD_CLR(sockfd, &allset);
					
                    client_fd[client_num] = -1;

#if DEBUG_NET_INFO
                    if(print_switch == 0)
                    {
                        printf("%s:client<%d>closed. \n", NET_INFO, client_num);
                    }
#endif
                }
			//	else if (len < 0)
			//	{

			//	}
				else
                {
#if 1
                   if(len < 8)
                    {
                        /*<OL:xxx;>*/
                        /*@@{   00000000 00000000 00000000*/
                        /*OL:oo?;*/
                        //判断取盘信息
                        if(strncmp1("OL", recvbuf, 2) == 0)
                        {
                            for(i = 0; i < 3; i++)
                            {
                                for(j = 0; j < 8; j++)
                                {
                                    tmp = ((recvbuf[3 + i] >> j) & 0x01);
                                    if(tmp == 0x00) //not online
                                    {
                                        temp = i * 8 + j;
                                        //printf("not online temp:%d\n ",temp);
                                        if(temp < 0 || temp > 23)
                                        {
                                            printf("<not online>temp out of range\n");
                                        }
                                        if(online_sta[i * 8 + j] == 0x01)
                                        {
                                            //send not online
                                            send_online_framing(temp, 'O');

                                            /*取盘后，删除以前上报数据*/
                                            if((temp) < 14)
                                            {
                                                memset(&rpt_content.slot[temp], '\0', sizeof(out_content));
                                            }
                                            else if(temp == 14)
                                            {
                                                memset(&rpt_content.slot_o, '\0', sizeof(rb_content));
                                                //flag_slotOIsMaster = 0;
                                            }
                                            else if(temp == 15)
                                            {
                                                memset(&rpt_content.slot_p, '\0', sizeof(rb_content));

                                            }
                                            else if(temp == 16)
                                            {
                                                memset(&rpt_content.slot_q, '\0', sizeof(gbtp_content));
                                            }
                                            else if(temp == 17)
                                            {
                                                memset(&rpt_content.slot_r, '\0', sizeof(gbtp_content));
                                            }
                                            memset(alm_sta[temp], 0, 14);

                                            send_pullout_alm(temp + 24, 0);
                                        }
                                        online_sta[temp] = tmp;
                                        slot_type[temp] = 'O';
                                    }
                                    else   //online
                                    {
                                        temp = i * 8 + j;
                                        if(temp < 0 || temp > 23)
                                        {
                                            printf("<online>temp out of range\n");
                                        }
                                        if(online_sta[temp] == 0x00)
                                        {
                                            if(temp == 22)
                                            {
                                                send_online_framing(temp, 'Q');
                                                slot_type[temp] = 'Q';
                                            }
                                            else if((temp == 18) || (temp == 19))
                                            {
                                                //send online
                                                send_online_framing(temp, 'M');
                                                slot_type[temp] = 'M';
                                                //init_pan(temp);
                                            }
                                            //else if((temp==20)||(temp==21))
                                            else if(temp == 20)
                                            {
                                                //send online
                                                send_online_framing(temp, 'N');
                                                slot_type[temp] = 'N';
                                                init_pan(20);
                                            }
                                            else if((temp == 16) || (temp == 17))
                                            {
                                                //send online
                                                //send_online_framing(temp,'L');
                                                //slot_type[temp]='L';
                                            }
                                            else if((temp == 14) || (temp == 15))
                                            {
                                                //send online
                                                //	send_online_framing(temp,'K');
                                                //	slot_type[temp]='K';
                                                //init_pan((char)(temp+'a'));
                                                //if(temp==14)
                                                //printf("online\n");
                                            }
                                            if(temp >= 0 && temp < 24)
                                            {
                                                send_pullout_alm(temp + 24, 1);
                                            }

                                        }
                                        online_sta[temp] = tmp;
                                    }
                                }
                            }

                        }
                        /*
                        if(0 == online_sta[15])//2014-1-27 rb主框出问题时启用备用框
                        {
                        	if(0 == flag_slotOIsMaster && slot_type[14]!= 'O')
                        	{
                        		//printf("set-main:main\n");
                        		sendtodown("SET-MAIN:000000::MAIN;",'o');
                        		flag_slotOIsMaster = 1;
                        	}
                        }
                        else
                        {
                        	if(1 == flag_slotOIsMaster && slot_type[15]!= 'O')
                        	{
                        		flag_slotOIsMaster = 0;
                        		sendtodown("SET-MAIN:000000::STBY;",'o');
                        	}
                        }
                        */
                    }

                    else
                    {
                        for(i = 0, j = 0; i < 1024; i++)
                        {
                            if(recvbuf[i] == ';')
                            {
                                len_rpt = i - j + 1;
                                memset(rpt_buf, '\0', sizeof(unsigned char) * MAXDATASIZE);
                                memcpy(rpt_buf, &recvbuf[j], len_rpt);
                                judge_data(rpt_buf, len_rpt, client_fd[client_num]);
                                j = i + 1;
                            }
                        }
                    }
#endif
                }

                if (--nready <= 0)
                {
                    break;
                }
            }
        }

#if 1
        if(1 == reset_network_card)
        {
            if(0 == shake_hand_count)
            {
               //printf("phy reset\r\n");
			   mcpd_reset_network_card();
            }

            shake_hand_count = 0;
            reset_network_card = 0;
        }
#endif
    }

    close(listenfd);            //关闭服务器监听socket
    Close_FpgaDev();
    return 0;
}




