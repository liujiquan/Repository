//*-----------------------------------------------------------------------------*/
//* Copyright(C) 2014, liujiquan Company All rights reserved. )
//* FileName :   queue.h
//* Author   :   liujiquan
//* DateTime :   11/27/2014
//* Version  :   1.0
//* Comment  :   
//*-----------------------------------------------------------------------------*/
#ifndef __QUEUE_H_2459134951B94A1895E650CFD52F4215__
#define __QUEUE_H_2459134951B94A1895E650CFD52F4215__
#pragma once
#include "header.h"

/************************************************************************/
/* Function                                                               */
/************************************************************************/
void PacketQueue_init(PacketQueue* quene);										// init
void PacketQueue_start(PacketQueue* quene, AVPacket* packet);					// 刷新packet的初始化
bool PacketQueue_push_flush(PacketQueue* quene,  AVPacket* packet);					// 刷新包的判断
bool PacketQueue_push(PacketQueue* quene,  AVPacket* packet);					// 入栈
int  PacketQueue_pop(PacketQueue* queue,  AVPacket* packet, bool bQuit);		// 出栈
int	 PacketQueue_destory(PacketQueue* quene);									// 销毁
void PacketQueue_abort(PacketQueue* quene);										// 中断
void PacketQueue_clear(PacketQueue* quene);										// 清空


#endif//__QUEUE_H_2459134951B94A1895E650CFD52F4215__