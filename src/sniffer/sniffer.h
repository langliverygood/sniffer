#ifndef _SNIFFER_H_
#define _SNIFFER_H_

#define BUFFER_MAX 2048
#define RING_BUFFER_SIZE 127
#define PACP_MAX_NUM 100

/***************************************************************/
/**函  数：sniffer_init *****************************************/
/* 说  明：初始化 sniffer ****************************************/
/* 参  数：无 ***************************************************/
/* 返回值：0 初始化成功********************************************/
/*      ：1 初始化失败********************************************/
/***************************************************************/
char sniffer_init();

/***************************************************************/
/**函  数：sniffer_start() **************************************/
/* 说  明：启动sniffer，新建两个线程 *******************************/
/* 参  数：无 ***************************************************/
/* 返回值：无 ****************************************************/
/***************************************************************/
void sniffer_start();

/***************************************************************/
/**函  数：sniffer_stop() **************************************/
/* 说  明：关闭sniffer，中止两个线程 *******************************/
/* 参  数：无 ***************************************************/
/* 返回值：无 ****************************************************/
/***************************************************************/
void sniffer_stop();

#endif
