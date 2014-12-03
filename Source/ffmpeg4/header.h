//*-----------------------------------------------------------------------------*/
//* Copyright(C) 2014, liujiquan Company All rights reserved. )
//* FileName :   header.h
//* Author   :   liujiquan
//* DateTime :   11/27/2014
//* Version  :   1.0
//* Comment  :   
//*-----------------------------------------------------------------------------*/
#ifndef __HEADER_H_2459134951B94A1895E650CFD52F4215__
#define __HEADER_H_2459134951B94A1895E650CFD52F4215__
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
#include "libswresample/swresample.h"
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

#pragma warning(disable : 4067)
/************************************************************************/
/* Define                                                               */
/************************************************************************/
#define		SDL_AUDIO_BUFFER_SIZE	1024
//#define		AUDIO_BUF_LEN			AVCODEC_MAX_AUDIO_FRAME_SIZE * 3 /2
#define		AUDIO_BUF_LEN			AVCODEC_MAX_AUDIO_FRAME_SIZE * 4
#define		MAX_QUEUE_SIZE (15 * 1024 * 1024)	// 队列的最大值
#define		MIN_FRAMES      5					// 队列的最大值
#define		MAX_QUEUE_NB	5					
#define		VIDEO_FRAME_QUEUE_SIZE	1			// 
#define		AV_NOSYNC_THRESHOLD		10000.0
/************************************************************************/
/* Struct                                                               */
/************************************************************************/
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
	SDL_Surface*	screen;
	SDL_Overlay*	bmp;
//	int				nWidth;
//	int				nHeight;
//	int				allocated;
	int				pts;		// 报告时间
	int				dts;		// 解压时间
	int				pos;		// 字节pos
	double			delayTime;	// 延迟时间
}VideoFrame, *PVideoFrame;

// Video Struct
typedef struct tag_VideoStreamUtil
{
	int					nVideoIndex;				// 视频流索引
	AVStream*			pVideoStream;				// 视频流
	AVCodecContext*		pVideoCodecCtx;			// 视频解码容器
	AVCodec*			pVideoCodec;					// 视频解码器
	int					nWidth;					// 视频宽
	int					nHeight;					// 视频高
	int					nShowWidth;				// 视频显示宽
	int					nShowHeight;				// 视频显示高
	double				dbVideoDelayTime;			// 视频每幀延迟时间
	SDL_Thread*			ShowVideo_ThreadID;		// 显示视频  pop
	SDL_Thread*			DecodeVideo_ThreadID;		// 解码视频数据 push
	PacketQueue			VideoPktQueue;			// 视频队列
	VideoFrame			VideoFrameArr[VIDEO_FRAME_QUEUE_SIZE];	// 解码幀
	int					VideoFrameSize;			// 解码幀大小
	int					VideoFrameDecodeIndex;		// 当前解码幀索引
	int					VideoFrameShowIndex;		// 当前显示幀索引
	SDL_mutex*			VideoFrame_mutex;			// 解码幀互斥变量
	SDL_cond*			VideoFrame_cond;			// 解码幀条件变量

	int					oldPTS;					// 前一个报告时间
	int					oldDTS;					// 前一个解压时间
	int					oldPos;					// 前一个字节索引

	double				videoClock;				// 视频的时间戳
}VideoStreamUtil;

// Audio Struct
typedef struct tag_AudioStreamUtil
{
	int					nAudioIndex;				// 音频流索引
	AVStream*			pAudioStream;				// 音频流
	AVCodecContext*		pAudioCodecCtx;			// 音频解码容器
	AVCodec*			pAudioCodec;					// 音频解码器 
	PacketQueue			AudioPktQueue;			// 音频队列
	uint8_t				audio_buf[AUDIO_BUF_LEN];	// 音频Buffer
	unsigned int		audio_buf_size;				// 音频Buffer当前总大小
	unsigned int		audio_buf_index;				// 音频Buffer已经解析的大小
	AVPacket			Audio_CurPacket;				// 音频包
	uint8_t *			audio_pkt_data;			// 音频包数据
	int					audio_pkt_size;			// 音频包数据大小

	enum AVSampleFormat	audio_src_fmt;			// Sample fmt
	enum AVSampleFormat audio_dest_fmt;				// Sample fmt
	int					audio_src_channels;		// 声道数目
	int					audio_dest_channels;		// 声道数目
	int64_t				audio_src_channel_layout;	// 声道设计
	int64_t				audio_dest_channel_layout;	// 声道设计
	int					audio_src_freq;			// 频率
	int					audio_dest_freq;			// 频率

	struct SwrContext*	swr_context;				// Conver Audio
	double				audioClock;				// 音频的时间戳
}AudioStreamUtil;

// 流媒体
typedef struct tag_StreamMedia
{
	bool				bPause;				// 暂停标志
	bool				bQuit;				// 退出标志
	bool				bVideoStream;			// 是否存在视频流
	bool				bAudioStream;			// 是否存在音频流
	AVFormatContext*	pFormatContext;		// 总容器
	AVPacket			flush_pkt;			// 刷新包
	SDL_Thread*			running_ThreadID;	// 运行线程ID
	VideoStreamUtil		videoStreamUtil;	// VedioUtil
	AudioStreamUtil		audioStreamUtil;	// AudioUtil
}StreamMedia, *PStreamMedia;

/************************************************************************/
/* Function                                                               */
/************************************************************************/
bool RelationHwnd(long hwnd);				// 使用窗体 记住在initizlize之前使用
bool initizlize();						// 环境初始化
bool Uninitialize();						// 环境释放
SDL_Surface* CreateSurface(int nWidth, int nHeight, int nFlags = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL);
SDL_Overlay* CreateWindow_(int nWidth, int nHeight, SDL_Surface* screen);
char* GetAVPixelFormatInfo(AVPixelFormat pix_fmt);
#endif//__HEADER_H_2459134951B94A1895E650CFD52F4215__