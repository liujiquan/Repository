//*-----------------------------------------------------------------------------*/
//* Copyright(C) 2014, liujiquan Company All rights reserved. )
//* FileName :   queue.cpp
//* Author   :   liujiquan
//* DateTime :   11/27/2014
//* Version  :   1.0
//* Comment  :   
//*-----------------------------------------------------------------------------*/
#include "Queue.h"


// -----------------------------------------------------------//
// Function :   PacketQueue_init
// Param    :   PacketQueue* quene
// Return   :   void
// Comment  :   初始化音频链表[初始化互斥变量，条件变量]
// -----------------------------------------------------------//
void PacketQueue_init(PacketQueue* quene)	
{
	memset(quene, 0x00, sizeof(PacketQueue));
	quene->mutex = SDL_CreateMutex();	// 创建互斥变量
	quene->cond = SDL_CreateCond();		// 创建条件变量
}

// -----------------------------------------------------------//
// Function :   PacketQueue_start
// Param    :   PacketQueue* quene
//				AVPacket* packet
// Return   :   void
// Comment  :   刷新packet的初始化
// -----------------------------------------------------------//
void PacketQueue_start(PacketQueue* quene, AVPacket* packet)
{
	SDL_LockMutex(quene->mutex);
	PacketQueue_push_flush(quene, packet);
	SDL_UnlockMutex(quene->mutex);
}

// 刷新包push
bool PacketQueue_push_flush(PacketQueue* quene,  AVPacket* packet)
{
	if(packet == 0 || quene == 0)	return false;
	
	AVPacketList* pktList;
	pktList = (AVPacketList*)av_malloc(sizeof(AVPacketList));
	if(pktList == NULL)		
	{
		return false;
	}

	pktList->pkt = *packet;
	pktList->next = NULL;
	
	// 入栈
	if(quene->header_pkt == NULL)// 链表为空
	{
		quene->header_pkt = quene->tail_pkt = pktList;	// head=tail=pktList
	}
	else
	{
		quene->tail_pkt->next = pktList;				// tailData->next = pktList
		quene->tail_pkt = pktList;						// pktList作为新的tail
	}
	quene->nb_packets ++;		// 个数
	quene->size += packet->size;// 大小
	// 发送结束信号
	SDL_CondSignal(quene->cond);
	
	return 0;
}

// -----------------------------------------------------------//
// Function :   PacketQueue_push
// Param    :   PacketQueue* quene
//              AVPacket* packet
// Return   :   bool
// Comment  :   入栈
// -----------------------------------------------------------//
bool PacketQueue_push(PacketQueue* quene,  AVPacket* packet)		
{
	if(packet == 0 || quene == 0)	return false;
	// new一个AVPacketList数据
	AVPacketList* pktList;
	if(av_dup_packet(packet) < 0)	// packet内存有问题
	{
		return false;
	}
	pktList = (AVPacketList*)av_malloc(sizeof(AVPacketList));
	if(pktList == NULL)		
	{
		return false;
	}
	pktList->pkt = *packet;
	pktList->next = NULL;
	
	SDL_LockMutex(quene->mutex);
	// 入栈
	if(quene->header_pkt == NULL)// 链表为空
	{
		quene->header_pkt = quene->tail_pkt = pktList;	// head=tail=pktList
	}
	else
	{
		quene->tail_pkt->next = pktList;				// tailData->next = pktList
		quene->tail_pkt = pktList;						// pktList作为新的tail
	}
	quene->nb_packets ++;		// 个数
	quene->size += packet->size;// 大小
	// 发送结束信号
	SDL_CondSignal(quene->cond);
	SDL_UnlockMutex(quene->mutex);

	return true;
}

// -----------------------------------------------------------//
// Function :   PacketQueue_pop
// Param    :   StreamMedia* pStreamMedia
//              AVPacket* packet
//				bool bQuit
// Return   :   int 
// Comment  :   出栈
// -----------------------------------------------------------//
int  PacketQueue_pop(PacketQueue* queue,  AVPacket* packet, bool bQuit)		
{
	if(packet == NULL)						return false;
	if(queue == NULL)						return false;

	AVPacketList* pktList;
	int intRet = -1;

	SDL_LockMutex(queue->mutex);
	while(true)
	{
		if(bQuit)
		{
			intRet = -1;
			break;
		}
		pktList = queue->header_pkt;					// header
		if(pktList)
		{
			// 1):栈非空 pop操作
			queue->header_pkt = pktList->next;	// header->next作为新的header
			if(queue->header_pkt == NULL)
			{
				// pop后栈空
				queue->tail_pkt = NULL;
			}
			queue->nb_packets--;
			queue->size -= pktList->pkt.size;
			*packet = pktList->pkt;
			av_free(pktList);
			pktList = NULL;

			intRet = 1;
			break;
		}
		else
		{
			// 2)栈为空 等待push
			SDL_CondWait(queue->cond, queue->mutex);	// 等待SDL_CondSignal
		}
	}
	SDL_UnlockMutex(queue->mutex);
	
	return intRet;
}

// -----------------------------------------------------------//
// Function :   PacketQueue_abort
// Param    :   PacketQueue* quene
// Return   :   void
// Comment  :   中断
// -----------------------------------------------------------//
void PacketQueue_abort(PacketQueue* quene) 
{
	SDL_LockMutex(quene->mutex);
	SDL_CondBroadcast(quene->cond);
	SDL_UnlockMutex(quene->mutex);
}

// -----------------------------------------------------------//
// Function :   PacketQueue_clear
// Param    :   PacketQueue* quene
// Return   :   void
// Comment  :   清空
// -----------------------------------------------------------//
void PacketQueue_clear(PacketQueue* quene)
{
	AVPacketList* pkt1, *pkt2;
	SDL_LockMutex(quene->mutex);
	for(pkt1 = quene->header_pkt; pkt1 != NULL;)
	{
		pkt2 = pkt1->next;
		av_free_packet(&pkt1->pkt);
		av_free(pkt1);
		pkt1 = NULL;

		pkt1 = pkt2;
	}
	quene->header_pkt = quene->tail_pkt = NULL;
	quene->nb_packets = 0;
	quene->size = 0;

	SDL_UnlockMutex(quene->mutex);
}

// -----------------------------------------------------------//
// Function :   PacketQueue_destory
// Param    :   PacketQueue* quene
// Return   :   
// Comment  :   销毁
// -----------------------------------------------------------//
int	 PacketQueue_destory(PacketQueue* quene)						 
{
	if(quene == NULL)	return -1;
	if(quene->cond)
	{
		SDL_DestroyCond(quene->cond);
		quene->cond = NULL;
	}
	if(quene->mutex)
	{
		SDL_DestroyMutex(quene->mutex);
		quene->mutex = NULL;
	}

	return 0;
}