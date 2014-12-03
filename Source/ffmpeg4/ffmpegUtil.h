//*-----------------------------------------------------------------------------*/
//* Copyright(C) 2014, liujiquan Company All rights reserved. )
//* FileName :   Base64.h
//* Author   :   liujiquan
//* DateTime :   11/26/2014
//* Version  :   1.0
//* Comment  :   
//*-----------------------------------------------------------------------------*/
#ifndef __FFMPEGUTIL_H_2459134951B94A1895E650CFD52F4215__
#define __FFMPEGUTIL_H_2459134951B94A1895E650CFD52F4215__
#pragma once
/************************************************************************/
/* Header                                                               */
/************************************************************************/
extern "C"
{
//FFMPEG
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"
#include "libavutil/mathematics.h"
#include "inttypes.h"
//SDL	
#include "SDL.h"
#include "SDL_thread.h"
};
#undef main
#pragma comment(lib,"avformat.lib")
#pragma comment(lib,"avcodec.lib")
#pragma comment(lib,"avdevice.lib")
#pragma comment(lib,"avfilter.lib")
#pragma comment(lib,"avutil.lib")
#pragma comment(lib,"postproc.lib")
#pragma comment(lib,"swresample.lib")
#pragma comment(lib,"swscale.lib")
#pragma comment(lib,"SDL.lib")
/************************************************************************/
/* Define                                                               */
/************************************************************************/
#define		SDL_AUDIO_BUFFER_SIZE	1024
#define		AUDIO_BUF_LEN			AVCODEC_MAX_AUDIO_FRAME_SIZE * 3 /2
#define		MAX_QUEUE_SIZE (15 * 1024 * 1024)	// 队列的最大值
#define		MIN_FRAMES      5					// 队列的最大值
#define		VIDEO_FRAME_QUEUE_SIZE	1			// 
/************************************************************************/
/* Struct                                                               */
/************************************************************************/
//视频可以直接解码 音频不能直接解码 -
//必须一个一个 【入栈-解码-出栈】
//Packet队列
typedef struct tag_PacketQueue
{
	AVPacketList*	header_pkt;	// header
	AVPacketList*	tail_pkt;	// tail
	int				nb_packets;	// Packet 个数
	int				size;		// Packet 大小
	SDL_mutex*		mutex;		// 互斥量
	SDL_cond*		cond;		// 条件
}PacketQueue;
// 一幀视频解码后的数据
typedef struct tag_VideoFrame
{
	SDL_Overlay*	bmp;
	int				nWidth;
	int				nHeight;
	int				allocated;
}VideoFrame, *PVideoFrame;

// 流媒体
typedef struct tag_StreamMedia
{
	bool				bQuit;				// 退出标志

	AVFormatContext*	pFormatContext;		// 总容器
	AVStream*			pVideoStream;		// 视频流
	AVStream*			pAudioStream;		// 音频流
	int					nVideoIndex;		// 视频流索引
	int					nAudioIndex;		// 音频流索引

	AVCodecContext*		pVideoCodecCtx;		// 视频解码容器
	AVCodecContext*		pAudioCodecCtx;		// 音频解码容器
	AVCodec*			pVideoCodec;		// 视频解码器
	AVCodec*			pAudioCodec;		// 音频解码器
	 
	PacketQueue			AudioPktQueue;		// 音频队列
	uint8_t				audio_buf[AUDIO_BUF_LEN];// 音频Buffer
	unsigned int		audio_buf_size;		// 音频Buffer当前总大小
	unsigned int		audio_buf_index;	// 音频Buffer已经解析的大小
	AVPacket			Audio_CurPacket;	// 音频包
	uint8_t *			audio_pkt_data;		// 音频包数据
	int					audio_pkt_size;		// 音频包数据大小

	SDL_Overlay*		Videobmp;
	PacketQueue			VideoPktQueue;		// 视频队列
	double				dbVideoDelayTime;	// 视频每幀延迟时间
	int					nWidth;				// 视频宽
	int					nHeight;			// 视频高
	int					nShowWidth;			// 视频显示宽
	int					nShowHeight;		// 视频显示高
	AVFrame*			pVideoMainFrame;	// 视频幀
	VideoFrame			VideoFrameArr[VIDEO_FRAME_QUEUE_SIZE];	// 解码幀
	int					VideoFrameSize;		// 解码幀大小
	int					VideoFrameCurIndex;	// 当前解码幀索引
	SDL_mutex*			VideoFrame_mutex;   // 解码幀互斥变量
	SDL_cond*			VideoFrame_cond;    // 解码幀条件变量
	SDL_Thread*			ShowVideo_ThreadID;	// 显示视频  pop
	SDL_Thread*			DecodeVideo_ThreadID;//解码视频数据 push

	AVPacket			flush_pkt;			// 刷新包

	SDL_Thread*			running_ThreadID;	// 运行线程ID

}StreamMedia, *PStreamMedia;

/************************************************************************/
/* Function                                                             */
/************************************************************************/
bool initizlize();																// 环境初始化
bool Uninitialize();															// 环境释放
// 流媒体
bool InitializeStreamMedia(PStreamMedia& pStreamMedia);							// 初始化流媒体
bool UnInitializeStreamMedia(PStreamMedia& pStreamMedia);						// Release流媒体
bool ReleaseStreamMedia(PStreamMedia& pStreamMedia);
bool LoadStreamMedia(PStreamMedia& pStreamMedia, const char* szFilePath);		// 载入一个流媒体
bool PlayStreamMedia(StreamMedia* pStreamMedia);								// 执行流媒体
// 其他
bool RelationHwnd(int hwnd);
bool FindCodecContextIndex(int& stream_idx, 
	AVFormatContext * fmt_ctx, enum AVMediaType type);							// 查找StreamIndex
// 视频
SDL_Overlay* CreateWindow_(int nWidth, int nHeight, int nFlags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL);// 创建显示界面
bool ShowWindow(AVPicture* pPicture,  SDL_Overlay* bmp, 
	enum PixelFormat foramt, int nWid, int nHei, int nShowWid, int nShowHei);	// 显示一幀数据
// 启动音频视频
bool StartAudio(StreamMedia* pStreamMedia);		// 创建音频回调函数										// 初始化音频处理
bool StartVideo(StreamMedia* pStreamMedia);		// 创建视频线程
// 队列操作
void PacketQueue_init(PacketQueue* quene);										// init
void PacketQueue_start(PacketQueue* quene, AVPacket* packet);					// 刷新packet的初始化
bool PacketQueue_push(PacketQueue* quene,  AVPacket* packet);					// 入栈
int  PacketQueue_pop(PacketQueue* queue,  AVPacket* packet, bool bQuit);		// 出栈
int	 PacketQueue_destory(PacketQueue* quene);									// 销毁
void PacketQueue_abort(PacketQueue* quene);										// 中断
void PacketQueue_clear(PacketQueue* quene);										// 清空
void audio_callback(void *userdata, Uint8 *stream, int len);					// 音频back函数
int audio_decode_frame(StreamMedia* pStreamMedia,								// 解码函数
	uint8_t* audio_buf, int nbufSize, double *pts_ptr);							

int Running_thread(void* lpParam );		// 运行线程
int ShowVideo_thread(void* lpParam);	// 视频显示线程	pop数据显示
int DecodeVideo_thread(void* lpParam);	// 视频刷新线程	解码填充数据
int FillPicture(StreamMedia* pStreamMedia, AVFrame* pMemFrame); // 填充一幀数据

#endif//__FFMPEGUTIL_H_2459134951B94A1895E650CFD52F4215__