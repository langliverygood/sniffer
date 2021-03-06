#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>

#include "ring_buffer.h"
#include "write_pcap.h"
#include "sniffer.h"

static pthread_t sniffer_write_pcap_td, sniffer_td; /* 程标识符 */
static char sniffer_write_pcap_td_exist;            /* 标记，写pcap文件线程是否存在 */
static char sniffer_td_exist; 			            /* 标记，sniffer线程是否存在 */
static int sock;                                    /* 标记，socket是否建立 */
static char sock_exsit;
static ring_buffer_p rb_p;
static char file_name[128];
static struct pcap_file_header pcap_h;
static struct packete_header packet_h;

/***************************************************************/
/**函  数：sniffer_init *****************************************/
/* 说  明：初始化 sniffer ****************************************/
/* 参  数：无 ***************************************************/
/* 返回值：0 初始化成功********************************************/
/*      ：1 初始化失败********************************************/
/***************************************************************/
char sniffer_init()
{
	if(!sock_exsit)
	{
		sock_exsit = 1;
		if((sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0)
		{
			sock_exsit = 0;
			fprintf(stdout, "Create socket error, please try to run as an administrator\n");
			return 1;
		}
	}
	
	pcap_h.magic = 0xa1b2c3d4;
	pcap_h.version_major = 0x0200;
	pcap_h.version_minor = 0x0400;
	pcap_h.thiszone = 0x0;
	pcap_h.sigfigs = 0x0;
	pcap_h.snaplen = 0xffffffff;
	pcap_h.linktype = 0x1;
	
	return 0;
}

/***************************************************************/
/**函  数：thread_sniffer_write_pcap ****************************/
/* 说  明：该线程函数从ring_buffer中读出数据并写入pacp文件中 ****/
/* 参  数：无 ***************************************************/
/* 返回值：NULL *************************************************/
/***************************************************************/
static void *thread_sniffer_write_pcap()
{
	int pcap_num, packet_num, cnt, len;
	char file_name_tmp[128];
	char *read_ptr;
	FILE *fp;
	struct tm *tm1;
    time_t secs;
    
    pthread_detach(pthread_self()); 						  /* 设置线程的分离状态 */
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);      /* 设置线程可被其他线程cansel */
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); /* 设置线程收到cansel信号时立刻退出 */

    pcap_num = PACP_MAX_NUM + 1;
    while(1)
    {
		if(pcap_num >= PACP_MAX_NUM) // 保证每个文件夹最多有PACP_MAX_NUM个pcap文件
		{
			pcap_num = 0;
			packet_num = PACKET_MAX_NUM + 1;
			memset(file_name_tmp, 0, sizeof(file_name_tmp));
			secs = time(NULL);
			tm1 = localtime(&secs);
			strftime(file_name_tmp, sizeof(file_name_tmp), "%Y-%m-%d-%H-%M-%S", tm1);
		}
		
		if(packet_num >= PACKET_MAX_NUM) // 保证pcap文件最多有PACKET_MAX_NUM个包
		{
			sprintf(file_name, "%s/%s--%d.pcap", file_name_tmp, file_name_tmp, pcap_num);
			if(create_pcap_file(file_name, pcap_h) != 0)
			{
				printf("Create %s failed!\n", file_name);
				continue;
			}
			pcap_num++;
			packet_num = 0;
		}
		
		fp = fopen(file_name, "a+");
		if(fp == NULL)
		{
			printf("Open %s failed!\n", file_name);
			continue;
		}
		cnt = 0;
		while(packet_num <= PACKET_MAX_NUM)
		{
			if(rb_can_read(rb_p))
			{
				read_ptr = get_read_address(rb_p);
				len = ((struct packete_header *)read_ptr)->len + sizeof(struct packete_header);
				fwrite((const void *)read_ptr, 1, len, fp);
				rb_read_out(rb_p);
				packet_num++;
			}
			else
			{
				cnt++;
			}
			if(cnt >= 5)// 若长期不可读，则该线程休眠1s
			{
				sleep(1);
				cnt = 0;
			}
		}
		fclose(fp);
	}
	
	return NULL;
}

/***************************************************************/
/**函  数：thread_sniffer() *************************************/
/* 说  明：该线程函数将sniffer捕捉的数据经过简单处理，写入ring_buffer */
/* 参  数：无 ***************************************************/
/* 返回值：NULL *************************************************/
/***************************************************************/
static void *thread_sniffer()
{
	int n_read;        
	char buffer[BUFFER_MAX];
	char *write_ptr;
	struct timeval tv;

	pthread_detach(pthread_self()); 						  /* 设置线程的分离状态 */
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);      /* 设置线程可被其他线程cansel */
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); /* 设置线程收到cansel信号时立刻退出 */
	
	while(1) 
	{
		n_read = recvfrom(sock, buffer, BUFFER_MAX, 0, NULL, NULL);
		gettimeofday(&tv, NULL);
		
		if(n_read < 42) 
		{
			printf("Incomplete header, packet corrupt\n");
			continue;
		}
		
		if(rb_can_write(rb_p))
		{
			write_ptr = get_write_address(rb_p); // 得到待写入的空间地址
			packet_h.timestamp_high = tv.tv_sec;
			packet_h.timestamp_low = tv.tv_usec;
			packet_h.caplen = n_read;
			packet_h.len = n_read;
			memcpy(write_ptr, &packet_h, sizeof(packet_h)); // 写入packet头部
			memcpy(write_ptr + sizeof(packet_h), buffer, n_read); // 写入捕捉的数据包
			rb_write_in(rb_p); // 移动写指针
		}
	}
	
	return NULL;
}

/***************************************************************/
/**函  数：sniffer_start() **************************************/
/* 说  明：启动sniffer，新建两个线程 *******************************/
/* 参  数：无 ***************************************************/
/* 返回值：无 ****************************************************/
/***************************************************************/
void sniffer_start()
{
	if(rb_p == NULL)
	{
		rb_create(RING_BUFFER_SIZE, BUFFER_MAX, &rb_p);
	}

	if(sniffer_td_exist == 0)
	{
		sniffer_td_exist = 1;
		if(pthread_create(&sniffer_td, NULL, thread_sniffer, NULL) != 0)
		{
			sniffer_td_exist = 0;
			printf("sniffer thread failed to start!\n");
			return;
		}
	}
	
	if(sniffer_write_pcap_td_exist == 0)
	{
		sniffer_write_pcap_td_exist = 1;
		if(pthread_create(&sniffer_write_pcap_td, NULL, thread_sniffer_write_pcap, NULL) != 0)
		{
			sniffer_write_pcap_td_exist = 0;
			printf("Write_pcap thread failed to start!\n");
			return;
		}
	}
	
	pthread_join(sniffer_td, NULL);
	pthread_join(sniffer_write_pcap_td, NULL);

	return;
}

/***************************************************************/
/**函  数：sniffer_stop() **************************************/
/* 说  明：关闭sniffer，中止两个线程 *******************************/
/* 参  数：无 ***************************************************/
/* 返回值：无 ****************************************************/
/***************************************************************/
void sniffer_stop()
{
	if(sniffer_td_exist)
	{
		pthread_cancel(sniffer_td);
	}
	if(sniffer_write_pcap_td_exist)
	{
		pthread_cancel(sniffer_write_pcap_td);
	}
	sniffer_td_exist = 0;
	sniffer_write_pcap_td_exist = 0;
	if(rb_p != NULL)
	{
		rb_delete(&rb_p);
	}
	if(sock_exsit)
	{
		sock_exsit = 0;
		close(sock);
	}
		
	return;
}
