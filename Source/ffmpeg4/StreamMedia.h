//*-----------------------------------------------------------------------------*/
//* Copyright(C) 2014, liujiquan Company All rights reserved. )
//* FileName :   StreamMedia.h
//* Author   :   liujiquan
//* DateTime :   11/27/2014
//* Version  :   1.0
//* Comment  :   
//*-----------------------------------------------------------------------------*/
#ifndef __STREAMMEDIA_H_2459134951B94A1895E650CFD52F4215__
#define __STREAMMEDIA_H_2459134951B94A1895E650CFD52F4215__
#pragma once
#include "header.h"
#include "Queue.h"
#include <afx.h>
#include <atltrace.h>
#define		WM_SETBASEINFO_MSG		WM_USER+2000	// 设置基础信息消息
#define		WM_SETINFO_MSG			WM_USER+2001	// 设置时间信息
#define		WM_SETCURTIME_MSG		WM_USER+2002	// 设置时间信息
typedef struct tag_DialogInfo
{
	char	szType[MAX_PATH];
	char	szSubType[MAX_PATH];
	char	szInfo[MAX_PATH*10];
}DialogInfo;
struct StreamInfo
{
	int				Maxts;		// 总时间
	int				pts;		// 报告时间
	int				dts;		// 解压时间
	int				pos;		// 字节pos
};
//CStreamMediaUtil
class CStreamMediaUtil
{
public:
	HWND			m_ParentHwnd;
	bool			m_bRunning;
private:
	long			m_Hwnd;
	StreamMedia*	m_pStreamMedia;
public:
	CStreamMediaUtil(long hwnd = NULL);
	~CStreamMediaUtil();
public:
	bool InitializeEnv();								// 初始化环境
	bool UnInitializeEnv();								// 释放环境
	bool InitializeStreamMedia();						// 初始化流媒体
	bool ReleaseStreamMedia();							// 释放流媒体
	
	bool LoadStreamMedia(const char* szFilePath);		// 载入一个流媒体
	bool PlayStreamMedia();								// 执行流媒体
	bool SetShowInfo(int nWidth, int nHeight, HWND hwnd);// 设置显示的宽高
	void Stop();
	void Pause();
	void SetDialogInfo(char* szMainType, char* szSubType, char* szInfo);
	void SetBaseInfo(StreamInfo* pInfo);

	bool FindCodecContextIndex(int& stream_idx, AVFormatContext * fmt_ctx, enum AVMediaType type);										// 查找StreamIndex
	bool ShowWindow_(AVPicture* pPicture,  SDL_Overlay* bmp, enum PixelFormat foramt, int nWid, int nHei, int nShowWid, int nShowHei);	// 显示一幀数据
	
	static int  play_thread_(void* lpParam );			// 主线程
	int  play_thread();									

	bool open_stream_component();						// 打开视频音频组件
	bool close_stream_component();						// 关闭视频音频组件
	bool StartVideo();								// 1.1:启动视频
	bool StartAudio();								// 2.1:启动音频函数	
	static int ShowVideo_thread_(void* lpParam);			// 1.2:显示视频帧 
	int ShowVideo_thread();				
	void video_refresh(int n);  
	static int DecodeVideo_thread_(void* lpParam);		// 1.3:解码视频幀 
	int DecodeVideo_thread();
	int FillPicture(AVFrame* pMemFrame);				// 1.4.1:填充一幀数据

	static void audio_callback(void *userdata, Uint8 *stream, int len);	// 2.2视频callback函数
	static int audio_decode_frame(StreamMedia* pStreamMedia, double* ptsPtr);//2.3 解码音频函数
};

//int audio_decode_frame(StreamMedia* pStreamMedia,								// 解码函数
//	uint8_t* audio_buf, int nbufSize, double *pts_ptr);	
static int av_getframe_size(AVFrame* pFrame);		// 得到音频Frame大小
#endif//__STREAMMEDIA_H_2459134951B94A1895E650CFD52F4215__