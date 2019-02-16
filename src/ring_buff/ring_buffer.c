#include <stdio.h>
#include <malloc.h>

#include "ring_buffer.h"

/***************************************************************/
/**函  数：rb_create ********************************************/
/* 说  明：初始化ring_buffer，申请空间 *****************************/
/* 参  数：element_num 元素的个数 *********************************/
/* 参  数：element_size 每个元素的大小 *****************************/
/* 参  数：ring_buffer 结构体的二级指针 ****************************/
/* 返回值：无 ****************************************************/
/***************************************************************/
void rb_create(int element_num, int element_size, ring_buffer_p *rb_p)
{
	*rb_p = malloc(sizeof(ring_buffer_s) + (element_num + 1) * element_size);//由于队满的条件，会浪费一个空间，所以多申请一个
	(*rb_p)->element_num = element_num;
	(*rb_p)->element_size = element_size;
	(*rb_p)->write_p = 0;
	(*rb_p)->read_p = 0;
	
	return;
}

/***************************************************************/
/**函  数：rb_can_write *****************************************/
/* 说  明：判断当前ring_buffer是否可写 ****************************/
/* 参  数：ring_buffer 结构体指针 ********************************/
/* 返回值：1 可写 ***********************************************/
/*      ：0 不可写 *********************************************/
/**************************************************************/
char rb_can_write(ring_buffer_p rb_p)
{
	if((rb_p->write_p + 1) % rb_p->element_num == rb_p->read_p)
	{
		return 0;
	}
	
	return 1;
}

/***************************************************************/
/**函  数：rb_write_in ******************************************/
/* 说  明：移动写指针 ********************************************/
/* 参  数：ring_buffer 结构体指针 ********************************/
/* 返回值：无 ***************************************************/
/**************************************************************/
void rb_write_in(ring_buffer_p rb_p)
{
	rb_p->write_p = (rb_p->write_p + 1) % rb_p->element_num;
	
	return;
}

/***************************************************************/
/**函  数：rb_can_read ******************************************/
/* 说  明：判断当前ring_buffer是否可读 ****************************/
/* 参  数：ring_buffer 结构体指针 ********************************/
/* 返回值：1 可读 ***********************************************/
/*      ：0 不可读 *********************************************/
/**************************************************************/
char rb_can_read(ring_buffer_p rb_p)
{
	if(rb_p->write_p == rb_p->read_p)
	{
		return 0;
	}
	
	return 1;
}

/***************************************************************/
/**函  数：rb_read_out ******************************************/
/* 说  明：移动读指针 ********************************************/
/* 参  数：ring_buffer 结构体指针 ********************************/
/* 返回值：无 ***************************************************/
/**************************************************************/
void rb_read_out(ring_buffer_p rb_p)
{
	rb_p->read_p = (rb_p->read_p + 1) % rb_p->element_num;
	
	return;
}

/***************************************************************/
/**函  数：get_write_address ************************************/
/* 说  明：获得待写入的空间地址 ************************************/
/* 参  数：ring_buffer 结构体指针 ********************************/
/* 返回值：待写入的空间地址 ***************************************/
/**************************************************************/
char *get_write_address(ring_buffer_p rb_p)
{
	char *p;
	
	p = (char *)rb_p;
	p += (sizeof(ring_buffer_s) + rb_p->element_size * rb_p->write_p);
	
	return p;
}

/***************************************************************/
/**函  数：get_read_address *************************************/
/* 说  明：获得待读出的空间地址 ************************************/
/* 参  数：ring_buffer 结构体指针 ********************************/
/* 返回值：待读出的空间地址 ***************************************/
/**************************************************************/
char *get_read_address(ring_buffer_p rb_p)
{
	char *p;
	
	p = (char *)rb_p;
	p += (sizeof(ring_buffer_s) + rb_p->element_size * rb_p->read_p);
	
	return p;
}

/***************************************************************/
/**函  数：rb_delete ********************************************/
/* 说  明：销毁ring_buffer，释放空间 ******************************/
/* 参  数：ring_buffer 结构体的二级指针 ***************************/
/* 返回值：无  **************************************************/
/**************************************************************/
void rb_delete(ring_buffer_p *rb_p)
{
	free(*rb_p);
	*rb_p = NULL;
	
	return;
}