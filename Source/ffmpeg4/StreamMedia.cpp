#include "StreamMedia.h"

//*-----------------------------------------------------------------------------*/
//* Copyright(C) 2014, liujiquan Company All rights reserved. )
//* FileName :   streamMedia.cpp
//* Author   :   liujiquan
//* DateTime :   11/27/2014
//* Version  :   1.0
//* Comment  :   
//*-----------------------------------------------------------------------------*/
CStreamMediaUtil::CStreamMediaUtil(long hwnd/* = NULL*/)
{
	m_ParentHwnd = NULL;
	m_Hwnd = hwnd;
	m_bRunning = false;
	InitializeEnv();
}

CStreamMediaUtil::~CStreamMediaUtil()
{
	UnInitializeEnv();
}

// 初始化流媒体
bool CStreamMediaUtil::InitializeEnv()
{
	// 1:分配内存
	m_pStreamMedia = NULL;
	m_pStreamMedia = (StreamMedia*)calloc(1, sizeof(StreamMedia));
	if(m_pStreamMedia == NULL)	return false;

	bool bRet = false;
	// 2:关联MFC窗体
	if(m_Hwnd != 0)
	{
		bRet = RelationHwnd(m_Hwnd);		
	}
	// 3:初始化FFMPEG+SDL环境
	bRet = initizlize();	

	return bRet;
}

// 释放流媒体
bool CStreamMediaUtil::UnInitializeEnv()
{
	if(m_pStreamMedia == NULL)	return false;
	
	// 1：释放资源
	free(m_pStreamMedia);
	m_pStreamMedia = NULL;
	// 2:释放FFMPEG+SDL环境
	Uninitialize();

	return true;
}

// Reset流媒体
bool CStreamMediaUtil::InitializeStreamMedia()						
{
	if(m_pStreamMedia == NULL)	return false;
	// Main
	m_pStreamMedia->bPause = false;									// 暂停标志
	m_pStreamMedia->bQuit = false;									// 退出标志
	m_pStreamMedia->bVideoStream = false;							// 是否存在视频流
	m_pStreamMedia->bAudioStream = false;							// 是否存在音频流
	m_pStreamMedia->pFormatContext = avformat_alloc_context();		// 总容器分配资源
	av_init_packet(&m_pStreamMedia->flush_pkt);						// 刷新包
	m_pStreamMedia->flush_pkt.data = (uint8_t*)("Flush");
	m_pStreamMedia->running_ThreadID = NULL;						// 运行线程ID
	// 视频
	m_pStreamMedia->videoStreamUtil.nVideoIndex = -1;				// 视频流索引
	m_pStreamMedia->videoStreamUtil.pVideoStream = NULL;			// 视频流
	m_pStreamMedia->videoStreamUtil.pVideoCodecCtx = NULL;		// 视频解码容器
	m_pStreamMedia->videoStreamUtil.pVideoCodec = NULL;			// 视频解码器
	m_pStreamMedia->videoStreamUtil.nWidth = 0;					// 视频宽
	m_pStreamMedia->videoStreamUtil.nHeight = 0;					// 视频高
	m_pStreamMedia->videoStreamUtil.dbVideoDelayTime = 0;			// 视频每幀延迟时间
	m_pStreamMedia->videoStreamUtil.ShowVideo_ThreadID = NULL;		// 显示视频  pop
	m_pStreamMedia->videoStreamUtil.DecodeVideo_ThreadID = NULL;	// 解码视频数据 push
	memset(m_pStreamMedia->videoStreamUtil.VideoFrameArr, 0x00, sizeof(VideoFrame) * VIDEO_FRAME_QUEUE_SIZE);// 解码幀
	for(int i = 0; i < VIDEO_FRAME_QUEUE_SIZE; i++)
	{
		m_pStreamMedia->videoStreamUtil.VideoFrameArr[i].bmp = NULL;
		m_pStreamMedia->videoStreamUtil.VideoFrameArr[i].screen = NULL;
		m_pStreamMedia->videoStreamUtil.VideoFrameArr[i].pts = 0;		// 报告时间
		m_pStreamMedia->videoStreamUtil.VideoFrameArr[i].dts = 0;		// 解压时间
		m_pStreamMedia->videoStreamUtil.VideoFrameArr[i].pos = 0;		// 字节pos
		m_pStreamMedia->videoStreamUtil.VideoFrameArr[i].delayTime = 0.0;// 延迟时间
	}
	m_pStreamMedia->videoStreamUtil.VideoFrameSize = 0;				// 解码幀大小
	m_pStreamMedia->videoStreamUtil.VideoFrameDecodeIndex = 0;			// 当前解码幀索引
	m_pStreamMedia->videoStreamUtil.VideoFrameShowIndex = 0;			// 当前显示幀索引
	m_pStreamMedia->videoStreamUtil.VideoFrame_mutex = SDL_CreateMutex();// 解码幀互斥变量
	m_pStreamMedia->videoStreamUtil.VideoFrame_cond = SDL_CreateCond();	// 解码幀条件变量
	m_pStreamMedia->videoStreamUtil.oldPTS = 0;						// 前一个报告时间
	m_pStreamMedia->videoStreamUtil.oldDTS = 0;						// 前一个解压时间
	m_pStreamMedia->videoStreamUtil.oldPos = 0;						// 前一个字节索引
	m_pStreamMedia->videoStreamUtil.videoClock = 0;

	// 音频
	m_pStreamMedia->audioStreamUtil.nAudioIndex = -1;							// 音频流索引
	m_pStreamMedia->audioStreamUtil.pAudioStream = NULL;						// 音频流
	m_pStreamMedia->audioStreamUtil.pAudioCodecCtx = NULL;					// 音频解码容器
	m_pStreamMedia->audioStreamUtil.pAudioCodec = NULL;						// 音频解码器 
	m_pStreamMedia->audioStreamUtil.audio_buf_size = 0;						// 音频Buffer当前总大小
	m_pStreamMedia->audioStreamUtil.audio_buf_index = 0;						// 音频Buffer已经解析的大小
	m_pStreamMedia->audioStreamUtil.audio_pkt_size = 0;						// 音频包数据大小
	m_pStreamMedia->audioStreamUtil.audio_pkt_data = NULL;					// 音频包数据
	m_pStreamMedia->audioStreamUtil.audio_src_fmt = AV_SAMPLE_FMT_NONE;			// Sample fmt	
	m_pStreamMedia->audioStreamUtil.audio_dest_fmt = AV_SAMPLE_FMT_NONE;		// Sample fmt
	m_pStreamMedia->audioStreamUtil.audio_src_channels = 0;					// 声道数目
	m_pStreamMedia->audioStreamUtil.audio_dest_channels = 0;					// 声道数目
	m_pStreamMedia->audioStreamUtil.audio_src_channel_layout = 0;				// 声道设计
	m_pStreamMedia->audioStreamUtil.audio_dest_channel_layout = 0;				// 声道设计
	m_pStreamMedia->audioStreamUtil.audio_src_freq = 0;						// 频率
	m_pStreamMedia->audioStreamUtil.audio_dest_freq = 0;						// 频率
	m_pStreamMedia->audioStreamUtil.swr_context = NULL;						// Conver Audio
	m_pStreamMedia->audioStreamUtil.audioClock = 0;

	return true;
}

// 关闭流媒体
bool CStreamMediaUtil::ReleaseStreamMedia()							
{
	if(m_pStreamMedia == NULL)	return false;

	// 视频
	if(m_pStreamMedia->bVideoStream)
	{
		if(m_pStreamMedia->videoStreamUtil.pVideoCodecCtx)
		{
			avcodec_close(m_pStreamMedia->videoStreamUtil.pVideoCodecCtx);
		}
//		SDL_LockMutex(m_pStreamMedia->videoStreamUtil.VideoFrame_mutex);
		SDL_CondSignal(m_pStreamMedia->videoStreamUtil.VideoFrame_cond);
//		SDL_UnlockMutex(m_pStreamMedia->videoStreamUtil.VideoFrame_mutex);
		if(m_pStreamMedia->videoStreamUtil.ShowVideo_ThreadID)			// 显示视频  pop
		{
			SDL_KillThread(m_pStreamMedia->videoStreamUtil.ShowVideo_ThreadID);
			m_pStreamMedia->videoStreamUtil.ShowVideo_ThreadID = NULL;
		} 
		if(m_pStreamMedia->videoStreamUtil.DecodeVideo_ThreadID)		// 解码视频数据 push
		{
			SDL_KillThread(m_pStreamMedia->videoStreamUtil.DecodeVideo_ThreadID);
			m_pStreamMedia->videoStreamUtil.DecodeVideo_ThreadID = NULL;
		}
		for(int i = 0; i < VIDEO_FRAME_QUEUE_SIZE; i++)
		{
			VideoFrame* pVideframe = &m_pStreamMedia->videoStreamUtil.VideoFrameArr[i];
			if(pVideframe->bmp)
			{
				SDL_FreeYUVOverlay(pVideframe->bmp);
				pVideframe->bmp = NULL;
			}
			if(pVideframe->screen)
			{
				SDL_FreeSurface(pVideframe->screen);
				pVideframe->screen = NULL;
			}
		}
		if(m_pStreamMedia->videoStreamUtil.VideoFrame_mutex)					// 解码幀互斥变量
		{
			SDL_DestroyMutex(m_pStreamMedia->videoStreamUtil.VideoFrame_mutex);
			m_pStreamMedia->videoStreamUtil.VideoFrame_mutex = NULL;
		}	
		if(m_pStreamMedia->videoStreamUtil.VideoFrame_cond)						// 解码幀条件变量
		{
			SDL_DestroyCond(m_pStreamMedia->videoStreamUtil.VideoFrame_cond);
			m_pStreamMedia->videoStreamUtil.VideoFrame_cond = NULL;
		}
	}
	if(m_pStreamMedia->bAudioStream)
	{
		if(m_pStreamMedia->audioStreamUtil.pAudioCodecCtx)
		{
			avcodec_close(m_pStreamMedia->audioStreamUtil.pAudioCodecCtx);
		}
		// This function shuts down audio processing and closes the audio device
		if(m_pStreamMedia->audioStreamUtil.swr_context)
		{
			swr_free(&m_pStreamMedia->audioStreamUtil.swr_context);
			m_pStreamMedia->audioStreamUtil.swr_context = NULL;
		}
	}
	// 总容器
	if(m_pStreamMedia->pFormatContext)			// 总容器
	{
		// Free an AVFormatContext and all its streams 
		// 必须最后调用不然前面的释放函数会出问题
		avformat_close_input(&m_pStreamMedia->pFormatContext);
		avformat_free_context(m_pStreamMedia->pFormatContext);
		m_pStreamMedia->pFormatContext = NULL;		
	}
	if(m_pStreamMedia->running_ThreadID)		// Read线程
	{
		SDL_KillThread(m_pStreamMedia->running_ThreadID);
		m_pStreamMedia->running_ThreadID = NULL;
	}

	return true;
}

// -----------------------------------------------------------//
// Function :   CStreamMediaUtil::LoadStreamMedia
// Param    :  const char* szFilePath          
// Return   :   bool
// Comment  :   载入一个流媒体
// -----------------------------------------------------------//
bool CStreamMediaUtil::LoadStreamMedia(const char* szFilePath)
{
	if(szFilePath == NULL)			return false;
	if(m_pStreamMedia == NULL)		return false;

	InitializeStreamMedia();
	VideoStreamUtil* pVideoStreamUtil = &m_pStreamMedia->videoStreamUtil;
	AudioStreamUtil* pAudioStreamUtil = &m_pStreamMedia->audioStreamUtil;

	m_pStreamMedia->pFormatContext = avformat_alloc_context();	// 总容器
	if(m_pStreamMedia->pFormatContext == NULL)			return false;
	// 1:Open an input stream and read the header
	if(avformat_open_input(&m_pStreamMedia->pFormatContext, szFilePath, NULL, NULL) != 0)
	{
		return false;
	}
	// 2:Read packets of a media file to get stream information
	if(av_find_stream_info(m_pStreamMedia->pFormatContext) < 0)	
	{
		return false;
	}
	//输入信息错误写入
	av_dump_format(m_pStreamMedia->pFormatContext, 0, szFilePath, 0); 
	// 设置信息
	char szInfo[MAX_PATH*10] = {0};
	SetDialogInfo("MainInfo", "FileName", m_pStreamMedia->pFormatContext->filename);
	sprintf(szInfo, "%s", m_pStreamMedia->pFormatContext->iformat->long_name);
	SetDialogInfo("MainInfo", "Format", szInfo);
	memset(szInfo, 0x00, MAX_PATH*10);
	sprintf(szInfo, "%.4f s", ((float)m_pStreamMedia->pFormatContext->duration)/1000/1000);
	SetDialogInfo("MainInfo", "Duration", szInfo);
	sprintf(szInfo, "%.4f s", ((float)m_pStreamMedia->pFormatContext->start_time)/1000/1000);
	SetDialogInfo("MainInfo", "starttime", szInfo);
	sprintf(szInfo, "%.4f kb/s", ((float)m_pStreamMedia->pFormatContext->bit_rate)/1000);
	SetDialogInfo("MainInfo", "bitrate", szInfo);

	// 3:VideoIndex/AudioIndex VideoStream/VideoStream
	FindCodecContextIndex(pVideoStreamUtil->nVideoIndex, m_pStreamMedia->pFormatContext, AVMEDIA_TYPE_VIDEO);
	FindCodecContextIndex(pAudioStreamUtil->nAudioIndex, m_pStreamMedia->pFormatContext, AVMEDIA_TYPE_AUDIO);
	if(pVideoStreamUtil->nVideoIndex != -1)
	{
		m_pStreamMedia->bVideoStream = true;		// 视频
	}
	if(pAudioStreamUtil->nAudioIndex != -1)
	{
		m_pStreamMedia->bAudioStream = true;		// 音频
	}
	// 没有音频视频
	if(!m_pStreamMedia->bVideoStream && !m_pStreamMedia->bAudioStream)
	{
		return false;
	}
	// 视频
	if(pVideoStreamUtil->nVideoIndex != -1)
	{
		pVideoStreamUtil->pVideoStream = m_pStreamMedia->pFormatContext->streams[pVideoStreamUtil->nVideoIndex];
		pVideoStreamUtil->pVideoCodecCtx = pVideoStreamUtil->pVideoStream->codec;							// 视频解码器容器
		pVideoStreamUtil->pVideoCodec = avcodec_find_decoder(pVideoStreamUtil->pVideoCodecCtx->codec_id);	// 视频解码器
		if(pVideoStreamUtil->pVideoCodecCtx)
		{
			if(avcodec_open2(pVideoStreamUtil->pVideoCodecCtx, pVideoStreamUtil->pVideoCodec, NULL) != 0)	return false;
		}
		// 6: width/height
		pVideoStreamUtil->nWidth = pVideoStreamUtil->pVideoCodecCtx->width;			// 视频witdh
		pVideoStreamUtil->nHeight = pVideoStreamUtil->pVideoCodecCtx->height;		// 视频witdh
		if(pVideoStreamUtil->nShowHeight <= 0 || pVideoStreamUtil->nShowWidth <= 0)
		{
			pVideoStreamUtil->nShowWidth = pVideoStreamUtil->pVideoCodecCtx->width;
			pVideoStreamUtil->nShowHeight = pVideoStreamUtil->pVideoCodecCtx->height;
		}
		// 4:dbVideoDelayTime
		double dbDuration;
		double dbFrameRate = av_q2d( pVideoStreamUtil->pVideoStream->r_frame_rate);
		if(strstr(m_pStreamMedia->pFormatContext->iformat->name, "mpegts"))
		{
			double timebase = 90*1000;
			dbDuration = timebase / dbFrameRate / 100000;
		}
		else
		{
			dbDuration = 1 / dbFrameRate;
		}
		pVideoStreamUtil->dbVideoDelayTime = dbDuration * 1000;
		if(pVideoStreamUtil->dbVideoDelayTime < 0)	
		{
			pVideoStreamUtil->dbVideoDelayTime = 0;
		}

		sprintf(szInfo, "%s", pVideoStreamUtil->pVideoCodec->long_name);
		SetDialogInfo("Video", "CodecInfo", szInfo);
		sprintf(szInfo, "%s", GetAVPixelFormatInfo(pVideoStreamUtil->pVideoCodecCtx->pix_fmt));
		SetDialogInfo("Video", "pix-fmt", szInfo);
		sprintf(szInfo, "%dx%d", pVideoStreamUtil->nWidth, pVideoStreamUtil->nHeight);
		SetDialogInfo("Video", "wid/hei", szInfo);
	}
	// 音频
	if(pAudioStreamUtil->nAudioIndex != -1)
	{
		pAudioStreamUtil->pAudioStream = m_pStreamMedia->pFormatContext->streams[pAudioStreamUtil->nAudioIndex];
		// 5:CodecCtx Codec
		pAudioStreamUtil->pAudioCodecCtx = pAudioStreamUtil->pAudioStream->codec;							// 音频解码器容器
		pAudioStreamUtil->pAudioCodec = avcodec_find_decoder(pAudioStreamUtil->pAudioCodecCtx->codec_id);	// 音频解码器
		if(pAudioStreamUtil->pAudioCodecCtx)
		{
			if(avcodec_open2(pAudioStreamUtil->pAudioCodecCtx, pAudioStreamUtil->pAudioCodec, NULL) != 0)	return false;
		}
		sprintf(szInfo, "%s", pAudioStreamUtil->pAudioCodec->name);
		SetDialogInfo("Audio", "name", szInfo);
	}
	StreamInfo info;
	memset(&info, 0x00, sizeof(StreamInfo));
	info.Maxts = m_pStreamMedia->pFormatContext->duration /1000 /1000;
	SetBaseInfo(&info);

	return true;
}
// -----------------------------------------------------------//
// Function :   CStreamMediaUtil::PlayStreamMedia
// Param    :   
// Return   :   bool
// Comment  :   执行流媒体
// -----------------------------------------------------------//
bool CStreamMediaUtil::PlayStreamMedia() 
{
	if(m_pStreamMedia == NULL)		return false;
	m_pStreamMedia->running_ThreadID = SDL_CreateThread(play_thread_, this);

	return true;
}

// 设置显示的宽高
bool CStreamMediaUtil::SetShowInfo(int nWidth, int nHeight, HWND hwnd)		
{
	if(m_pStreamMedia == NULL)		return false;
	if(nWidth <=0 || nHeight <= 0)	return false;
	VideoStreamUtil* pVideoStreamUtil = &m_pStreamMedia->videoStreamUtil;

	pVideoStreamUtil->nShowWidth = nWidth;
	pVideoStreamUtil->nShowHeight = nHeight;

	m_ParentHwnd = hwnd;

	return true;
}

void CStreamMediaUtil::Pause()
{
	if(m_pStreamMedia->bPause)	// 继续
	{
		SDL_PauseAudio(0);
		m_pStreamMedia->bPause = false;
	}
	else			// 暂停
	{
		SDL_PauseAudio(1);
		m_pStreamMedia->bPause = true;
	}
}

void CStreamMediaUtil::Stop()
{
	if(m_pStreamMedia)
	{
		m_pStreamMedia->bQuit = true;
	}
}

void CStreamMediaUtil::SetDialogInfo(char* szMainType, char* szSubType, char* szInfo)
{
	if(szMainType == NULL || szSubType == NULL || szInfo == NULL)	return;
	if(m_ParentHwnd == NULL)	return;

	DialogInfo info;
	memset(&info, 0x00, sizeof(info));
	strcpy_s(info.szType, szMainType);
	strcpy_s(info.szSubType, szSubType);
	strcpy_s(info.szInfo, szInfo);

	SendMessage(m_ParentHwnd, WM_SETBASEINFO_MSG, (WPARAM)&info, 0);
}

void CStreamMediaUtil::SetBaseInfo(StreamInfo* pInfo)
{
	if(pInfo == NULL)	return;
	SendMessage(m_ParentHwnd, WM_SETINFO_MSG, (WPARAM)pInfo, 0);
}
// -----------------------------------------------------------//
// Function :   CStreamMediaUtil::FindCodecContextIndex
// Param    :   int& stream_idx
//              AVFormatContext * fmt_ctx
//              enum AVMediaType type
// Return   :   bool
// Comment  :   查找Stream index
// -----------------------------------------------------------//
bool CStreamMediaUtil::FindCodecContextIndex(int& stream_idx, AVFormatContext * fmt_ctx, enum AVMediaType type)
{
	if(fmt_ctx == NULL)	return false;
	
	int nRet = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
	if(nRet != -1)	stream_idx = nRet;
	else			stream_idx = -1;

	return nRet == 0 ? true : false;
}

// ShowWindow_
bool CStreamMediaUtil::ShowWindow_(AVPicture* pPicture,  SDL_Overlay* bmp, enum PixelFormat foramt, int nWid, int nHei, int nShowWid, int nShowHei)
{
	SDL_Rect rect;
	AVPicture picture;
	struct SwsContext* img_convert_ctx = 0;	// Convert

	SDL_LockYUVOverlay(bmp);
	picture.data[0] = bmp->pixels[0];
	picture.data[1] = bmp->pixels[2];
	picture.data[2] = bmp->pixels[1];
	picture.linesize[0] = bmp->pitches[0];
	picture.linesize[1] = bmp->pitches[2];
	picture.linesize[2] = bmp->pitches[1];
	// init convert
	img_convert_ctx = sws_getCachedContext(img_convert_ctx,
		nWid, nHei, foramt, 
		nShowWid, nShowHei, PIX_FMT_YUV420P,SWS_X, 0, 0, 0);
	// convert
	sws_scale(img_convert_ctx, pPicture->data,  pPicture->linesize,
		0, nHei,picture.data, picture.linesize);
	SDL_UnlockYUVOverlay(bmp);
	// show
	rect.x = 0;
	rect.y = 0;
	rect.h = nShowHei;
	rect.w = nShowWid;

	SDL_DisplayYUVOverlay(bmp, &rect);

	return true;
}

// 运行线程
int  CStreamMediaUtil::play_thread_(void* lpParam)
{
	CStreamMediaUtil* _this = (CStreamMediaUtil*)lpParam;
	if(_this)
	{
		_this->play_thread();
	}

	return 0;
}

int  CStreamMediaUtil::play_thread()		
{
	if(m_pStreamMedia == NULL)	return -1;
	VideoStreamUtil* pVideoStreamUtil = &m_pStreamMedia->videoStreamUtil;
	AudioStreamUtil* pAudioStreamUtil = &m_pStreamMedia->audioStreamUtil;

	if(!m_pStreamMedia->bVideoStream && !m_pStreamMedia->bAudioStream)	
	{
		return -1;
	}
	if(open_stream_component() == false)	
	{
		return -1;
	}
	// 4:ReadFrame
	SDL_Event event;
	m_bRunning = true;
	while(true)
	{
		if(m_pStreamMedia->bQuit == true)	break;
		/* if the queue are full, no need to read more */
		if(m_pStreamMedia->bVideoStream && m_pStreamMedia->bAudioStream)
		{
			if (pVideoStreamUtil->VideoPktQueue.size + pAudioStreamUtil->AudioPktQueue.size> MAX_QUEUE_SIZE || pVideoStreamUtil->VideoPktQueue.nb_packets + pAudioStreamUtil->AudioPktQueue.nb_packets > MAX_QUEUE_NB )
			{
				/* wait 10 ms */
				SDL_Delay(10);
				continue;
			}
		}
		else if(m_pStreamMedia->bVideoStream)
		{
			if (pVideoStreamUtil->VideoPktQueue.size > MAX_QUEUE_SIZE || pVideoStreamUtil->VideoPktQueue.nb_packets > MAX_QUEUE_NB )
			{
				/* wait 10 ms */
				SDL_Delay(10);
				continue;
			}
		}
		else if(m_pStreamMedia->bAudioStream)
		{
			if (pAudioStreamUtil->AudioPktQueue.size > MAX_QUEUE_SIZE || pAudioStreamUtil->AudioPktQueue.nb_packets > MAX_QUEUE_NB )
			{
				/* wait 10 ms */
				SDL_Delay(10);
				continue;
			}
		}
		else
		{
			break;
		}

		AVPacket packet;
		AVPacket* packet1 = &packet;
		//：av_read_frame 解析的packet1 是分配了内存的最终都要av_free_packet
		int intRet = av_read_frame(m_pStreamMedia->pFormatContext, packet1);	// ReadFrame
		if(intRet < 0)	
		{
			// EOF
			if(intRet == AVERROR_EOF ||	url_feof(m_pStreamMedia->pFormatContext->pb))
			{
				break;
			}
			// ERROR
			if(m_pStreamMedia->pFormatContext->pb && m_pStreamMedia->pFormatContext->pb->error)
			{
				break;
			}
			continue;
		}
		// 视频
		if( packet1->stream_index == pVideoStreamUtil->nVideoIndex)
		{
			PacketQueue_push(&pVideoStreamUtil->VideoPktQueue, packet1);
		}
		// 音频
		else if(packet1->stream_index == pAudioStreamUtil->nAudioIndex)
		{
			PacketQueue_push(&pAudioStreamUtil->AudioPktQueue, packet1);
		}
		else
		{
			av_free_packet(packet1);
		}
		SDL_PollEvent(&event);
		if(event.type == SDL_QUIT)
		{
			break;
		}
	}
	close_stream_component();
	m_pStreamMedia->running_ThreadID = NULL;
	ReleaseStreamMedia();	// 释放资源
	
	m_bRunning = false;

	return true;
}

bool CStreamMediaUtil::open_stream_component()
{
	if(m_pStreamMedia->bVideoStream)
	{
		if(StartVideo() == false)		return false;
	}
	if(m_pStreamMedia->bAudioStream)
	{
		if(StartAudio() == false)		return false;
	}

	return true;
}
bool CStreamMediaUtil::close_stream_component()
{
	if(m_pStreamMedia->bVideoStream)
	{
		m_pStreamMedia->videoStreamUtil.pVideoStream->discard = AVDISCARD_ALL;
		PacketQueue_abort(&m_pStreamMedia->videoStreamUtil.VideoPktQueue);
		PacketQueue_clear(&m_pStreamMedia->videoStreamUtil.VideoPktQueue);
		PacketQueue_destory(&m_pStreamMedia->videoStreamUtil.VideoPktQueue);
	}
	if(m_pStreamMedia->bAudioStream)
	{
		m_pStreamMedia->audioStreamUtil.pAudioStream->discard = AVDISCARD_ALL;
		PacketQueue_abort(&m_pStreamMedia->audioStreamUtil.AudioPktQueue);	
		PacketQueue_clear(&m_pStreamMedia->audioStreamUtil.AudioPktQueue);
		PacketQueue_destory(&m_pStreamMedia->audioStreamUtil.AudioPktQueue);
		SDL_CloseAudio();
	}

	return true;
}

bool CStreamMediaUtil::StartAudio()
{
	if(m_pStreamMedia == NULL)						return false;
	AudioStreamUtil* pAudioStreamUtil = &m_pStreamMedia->audioStreamUtil;
	if(pAudioStreamUtil->pAudioCodecCtx == NULL)	return false;
	// wanted_spec期望值   spec 最终值
	SDL_AudioSpec wanted_spec, spec;
	int64_t wanted_channel_layout = 0;	// 声道设计
	int wanted_channels = 0;			// 声道个数	
	const int next_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};

	for(int i = 0; i < 10; i++)
	{
		wanted_channels = i;
		wanted_channel_layout = av_get_default_channel_layout(wanted_channels);
		const char* szName = av_get_channel_name(wanted_channels);
		const char* szDesc = av_get_channel_description(wanted_channels);
		AtlTrace("channels:%d	channel_layout:%d	Name:%s		Desc:%s\n",
			wanted_channels, 
			wanted_channel_layout, 
			szName, 
			szDesc);
	}

	for(int j = 0; j < 10; j++)
	{
		wanted_channel_layout = j;
		wanted_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
	}
	// 匹配声道-声道设计 例如1-4 2-3 4-263 8-1599
	wanted_channels = pAudioStreamUtil->pAudioCodecCtx->channels;
	wanted_channel_layout = pAudioStreamUtil->pAudioCodecCtx->channel_layout;

	// 
	if(wanted_channel_layout == 0 || wanted_channels != av_get_channel_layout_nb_channels(wanted_channel_layout))
	{
		wanted_channel_layout = av_get_default_channel_layout(wanted_channels);
		wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
	}
	wanted_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
	// 设置期望音频配置
	wanted_spec.channels = wanted_channels;
	wanted_spec.freq = pAudioStreamUtil->pAudioCodecCtx->sample_rate;
//	wanted_spec.format = pAudioStreamUtil->pAudioCodecCtx->sample_fmt;
	wanted_spec.format = AUDIO_U16SYS;//AUDIO_S16SYS;
	wanted_spec.samples = SDL_AUDIO_BUFFER_SIZE;
	wanted_spec.size = 0;
	wanted_spec.silence = 0;
	wanted_spec.callback = audio_callback;
	wanted_spec.userdata = m_pStreamMedia;

	while(SDL_OpenAudio(&wanted_spec, &spec) != 0)
	{
		wanted_spec.channels = next_channels[FFMIN(7, wanted_spec.channels)];
		if(wanted_spec.channels == 0)
		{
			return false;
		}
		wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
	}
	SDL_PauseAudio(0);	// pause call_back function
	if(spec.channels != wanted_spec.channels)
	{	
		wanted_channel_layout = av_get_default_channel_layout(spec.channels);
		if(wanted_channel_layout == 0)
		{
			return false;
		}
	}
	// Spec赋值
	pAudioStreamUtil->audio_src_channels = pAudioStreamUtil->audio_dest_channels = spec.channels;
	pAudioStreamUtil->audio_src_channel_layout = pAudioStreamUtil->audio_dest_channel_layout = wanted_channel_layout;
	pAudioStreamUtil->audio_src_fmt = pAudioStreamUtil->audio_dest_fmt = AV_SAMPLE_FMT_S16;
//	pAudioStreamUtil->audio_src_fmt = pAudioStreamUtil->audio_dest_fmt = (AVSampleFormat)spec.format;
	pAudioStreamUtil->audio_src_freq = pAudioStreamUtil->audio_dest_freq = spec.freq;

	// 初始化音频列表
	pAudioStreamUtil->audio_buf_size = 0;
	pAudioStreamUtil->audio_buf_index = 0;
	memset(&pAudioStreamUtil->Audio_CurPacket, 0x00, sizeof(AVPacket));
	PacketQueue_init(&pAudioStreamUtil->AudioPktQueue);			// 初始化音频链表
	PacketQueue_start(&pAudioStreamUtil->AudioPktQueue, &m_pStreamMedia->flush_pkt); // 刷新packet的初始化
	
	return true;
}

// 启动视频
bool CStreamMediaUtil::StartVideo()			
{
	if(m_pStreamMedia == NULL)	return false;
	VideoStreamUtil* pVideoStreamUtil = &m_pStreamMedia->videoStreamUtil;

	PacketQueue_init(&pVideoStreamUtil->VideoPktQueue);								// 初始化视频链表						
	PacketQueue_start(&pVideoStreamUtil->VideoPktQueue, &m_pStreamMedia->flush_pkt);	// push刷新包

	pVideoStreamUtil->ShowVideo_ThreadID = SDL_CreateThread(ShowVideo_thread_, this);	// 显示线程
	pVideoStreamUtil->DecodeVideo_ThreadID = SDL_CreateThread(DecodeVideo_thread_, this);// 解码线程
	
	return m_pStreamMedia->videoStreamUtil.DecodeVideo_ThreadID != NULL &&  pVideoStreamUtil->ShowVideo_ThreadID != NULL;
}

// 视频显示线程	pop数据显示
int CStreamMediaUtil::ShowVideo_thread_(void* lpParam)
{
	CStreamMediaUtil* _this = (CStreamMediaUtil*)lpParam;
	if(_this)
	{
		_this->ShowVideo_thread();
	}

	return 0;
}

int CStreamMediaUtil::ShowVideo_thread()
{
	if(m_pStreamMedia == NULL)	return -1;
	VideoStreamUtil* pVideoStreamUtil = &m_pStreamMedia->videoStreamUtil;

	while(true)
	{
		VideoFrame* videoFrame = NULL;
		if(m_pStreamMedia->bQuit)		break;
		if(m_pStreamMedia->bPause)//暂停
		{
			continue;
		}
		while(pVideoStreamUtil->VideoFrameSize == 0)	//No Data
		{
			SDL_CondWait(pVideoStreamUtil->VideoFrame_cond, pVideoStreamUtil->VideoFrame_mutex);
//			SDL_Delay(1);
		}
		// 取数据
		if(pVideoStreamUtil->VideoFrameShowIndex == VIDEO_FRAME_QUEUE_SIZE)
		{
			pVideoStreamUtil->VideoFrameShowIndex = 0;
		}
		videoFrame = &pVideoStreamUtil->VideoFrameArr[pVideoStreamUtil->VideoFrameShowIndex];	
		pVideoStreamUtil->VideoFrameShowIndex++;
		if(videoFrame->bmp)
		{
			SDL_Rect rect;
			rect.x = 0;
			rect.y = 0;
			rect.w = pVideoStreamUtil->nShowWidth;
			rect.h = pVideoStreamUtil->nShowHeight;
//			SDL_Delay(pVideoStreamUtil->dbVideoDelayTime);
//			SDL_Delay(videoFrame->delayTime);
			int delay = pVideoStreamUtil->dbVideoDelayTime;
			double diff = videoFrame->pts - m_pStreamMedia->audioStreamUtil.audioClock;
			if(fabs(diff) < AV_NOSYNC_THRESHOLD)
			{
				if(diff <= -delay) 
				{
					delay = 0.01;       //如果 视频显示过慢，离音频 过于远 则 显示时间为10ms
				} 
				else if(diff >= delay)
				{
					delay = 2 * delay;  //如果 视频显示过快 则停留 两帧的时间
				}
			}
			if (m_pStreamMedia->bVideoStream == false )
			{
				//这里是刷新时间间隔
				Sleep(pVideoStreamUtil->dbVideoDelayTime);
			}
			else
			{
				Sleep(delay);
			}

			SDL_DisplayYUVOverlay(videoFrame->bmp, &rect);

			StreamInfo info;
			memset(&info, 0x00, sizeof(StreamInfo));
			info.dts = videoFrame->dts;
			SetBaseInfo(&info);
		}
		SDL_LockMutex(pVideoStreamUtil->VideoFrame_mutex);
		pVideoStreamUtil->VideoFrameSize--;
		SDL_CondSignal(pVideoStreamUtil->VideoFrame_cond);
		SDL_UnlockMutex(pVideoStreamUtil->VideoFrame_mutex);
	}
	pVideoStreamUtil->ShowVideo_ThreadID = NULL;

	return 0;
}

void CStreamMediaUtil::video_refresh(int n)
{

}

// 视频刷新线程	解码填充数据
int CStreamMediaUtil::DecodeVideo_thread_(void* lpParam)
{
	CStreamMediaUtil* _this = (CStreamMediaUtil*)lpParam;
	if(_this)
	{
		_this->DecodeVideo_thread();
	}

	return 0;
}

int CStreamMediaUtil::DecodeVideo_thread()
{
	if(m_pStreamMedia == NULL)	return -1;
	VideoStreamUtil* pVideoStreamUtil = &m_pStreamMedia->videoStreamUtil;

	AVFrame* pMemFrame = avcodec_alloc_frame();
	
	while(true)
	{
		if(m_pStreamMedia->bQuit)	break;
		if(m_pStreamMedia->bPause)//暂停
		{
			continue;
		}
		AVPacket packet;
		// pop
		if(PacketQueue_pop(&pVideoStreamUtil->VideoPktQueue, &packet, m_pStreamMedia->bQuit) < 0) // 获取packet
		{
			// No data
			break;
		}
		if(packet.data == m_pStreamMedia->flush_pkt.data)
		{
			avcodec_flush_buffers(pVideoStreamUtil->pVideoCodecCtx);
			continue;
		}
		// Decode video frame
		int got_picture;
		int intRet = avcodec_decode_video2(pVideoStreamUtil->pVideoCodecCtx, 
										pMemFrame, 
										&got_picture, 
										&packet);
		av_free_packet(&packet);// free push/alloc
//		AtlTrace("Presentation timestamp%d	",packet.pts);
//		AtlTrace("Decompression timestamp%d	", packet.dts);
//		AtlTrace("pos%d\n", packet.pos);
		if(got_picture)
		{
			pVideoStreamUtil->videoClock += pVideoStreamUtil->dbVideoDelayTime;
			if(FillPicture(pMemFrame) < 0)
			{
				break;
			}
		}
		else 
		{
			continue;
		}
	}
	avcodec_free_frame(&pMemFrame);
	pVideoStreamUtil->DecodeVideo_ThreadID = NULL;

	return 0;
}

// 填充一幀数据
int CStreamMediaUtil::FillPicture(AVFrame* pMemFrame)		
{
	if(m_pStreamMedia == NULL)	return -1;
	VideoStreamUtil* pVideoStreamUtil = &m_pStreamMedia->videoStreamUtil;

	VideoFrame* videoFrame = NULL;
	struct SwsContext* img_convert_ctx = 0;	// Convert
	AVPicture picture;
	AVPicture* pPicture = (AVPicture*)pMemFrame;

	// 数据已经满了 等待
//	SDL_LockMutex(pVideoStreamUtil->VideoFrame_mutex);
	while (pVideoStreamUtil->VideoFrameSize >= VIDEO_FRAME_QUEUE_SIZE ) 
	{
		SDL_CondWait(pVideoStreamUtil->VideoFrame_cond, pVideoStreamUtil->VideoFrame_mutex);
	}
//	SDL_UnlockMutex(pVideoStreamUtil->VideoFrame_mutex);
	// videoFrame
	if(pVideoStreamUtil->VideoFrameDecodeIndex >= VIDEO_FRAME_QUEUE_SIZE) 
	{
		pVideoStreamUtil->VideoFrameDecodeIndex = 0;
	}
	videoFrame = &pVideoStreamUtil->VideoFrameArr[pVideoStreamUtil->VideoFrameDecodeIndex];
	pVideoStreamUtil->VideoFrameDecodeIndex++;
	// Create Screen
	if(videoFrame->bmp == NULL)
	{
		videoFrame->screen = CreateSurface(pVideoStreamUtil->nShowWidth, pVideoStreamUtil->nShowHeight);
		videoFrame->bmp = CreateWindow_(pVideoStreamUtil->nShowWidth, pVideoStreamUtil->nShowHeight, videoFrame->screen );
	}
	if(videoFrame->bmp == NULL)	return -1;
	videoFrame->pts = pMemFrame->pkt_pts;	// 报告时间
	videoFrame->dts = pMemFrame->pkt_dts;	// 解压时间
	videoFrame->pos = pMemFrame->pkt_pos;	// 字节pos
	videoFrame->delayTime = (double)videoFrame->dts - (double)pVideoStreamUtil->oldDTS;
	pVideoStreamUtil->oldPTS = videoFrame->pts;	// 前一个报告时间
	pVideoStreamUtil->oldDTS = videoFrame->dts;	// 前一个解压时间
	pVideoStreamUtil->oldPos = videoFrame->pos;	// 前一个字节索引

	SDL_LockYUVOverlay(videoFrame->bmp);
	picture.data[0] = videoFrame->bmp->pixels[0];
	picture.data[1] = videoFrame->bmp->pixels[2];
	picture.data[2] = videoFrame->bmp->pixels[1];
	picture.linesize[0] = videoFrame->bmp->pitches[0];
	picture.linesize[1] = videoFrame->bmp->pitches[2];
	picture.linesize[2] = videoFrame->bmp->pitches[1];
	// init convert
	img_convert_ctx = sws_getCachedContext(img_convert_ctx,
		pVideoStreamUtil->nWidth, pVideoStreamUtil->nHeight, pVideoStreamUtil->pVideoCodecCtx->pix_fmt, 
		pVideoStreamUtil->nShowWidth, pVideoStreamUtil->nShowHeight, PIX_FMT_YUV420P,SWS_X, 0, 0, 0);
	// convert
	sws_scale(img_convert_ctx, pPicture->data,  pPicture->linesize,
		0, pVideoStreamUtil->nHeight,picture.data, picture.linesize);
	SDL_UnlockYUVOverlay(videoFrame->bmp);
	videoFrame->pts = pVideoStreamUtil->videoClock;
	// 解码一个数据 唤醒显示线程
	SDL_LockMutex(pVideoStreamUtil->VideoFrame_mutex);
	SDL_CondSignal(pVideoStreamUtil->VideoFrame_cond);	
	pVideoStreamUtil->VideoFrameSize++;
	SDL_UnlockMutex(pVideoStreamUtil->VideoFrame_mutex);

	return 0;
}


void CStreamMediaUtil::audio_callback(void *userdata, Uint8 *stream, int len)	
{
	StreamMedia* pStreamMedia = (StreamMedia*)userdata;
	if(pStreamMedia == NULL)						return;
	AudioStreamUtil* pAudioStreamUtil = &pStreamMedia->audioStreamUtil;
	if(pAudioStreamUtil == NULL)					return;
	if(pAudioStreamUtil->pAudioCodecCtx == NULL)	return;
	double pts;

	//len是由SDL传入的SDL缓冲区的大小，如果这个缓冲未满，我们就一直往里填充数据 
	while(len > 0 )
	{
		// audio_buf_index 和 audio_buf_size 
		// 标示我们自己用来放置解码出来的数据的缓冲区，
		// 这些数据待copy到SDL缓冲区， 当audio_buf_index >= audio_buf_size
		// 的时候意味着我们的缓冲为空，没有数据可供copy，
		// 这时候需要调用audio_decode_frame来解码出更多的桢数据
		if(pAudioStreamUtil->audio_buf_index >= pAudioStreamUtil->audio_buf_size)
		{
			/* We have already sent all our data; get more */
//			int audio_size = audio_decode_frame(pStreamMedia,// decode		
//				pAudioStreamUtil->audio_buf, 
//				sizeof(pAudioStreamUtil->audio_buf), 
//				&pts);
			int audio_size = audio_decode_frame(pStreamMedia, &pts);// decode	
			// audioDecodeSize < 0 标示没能解码出数据，我们默认播放静音
			if(audio_size < 0)
			{
				/* If error, output silence */
				pAudioStreamUtil->audio_buf_size = SDL_AUDIO_BUFFER_SIZE;
				memset(pAudioStreamUtil->audio_buf, 0, pAudioStreamUtil->audio_buf_size);
			}
			else
			{
				pAudioStreamUtil->audio_buf_size = audio_size;
			}

			pAudioStreamUtil->audio_buf_index = 0;
		}
		//查看stream可用空间，决定一次copy多少数据，剩下的下次继续copy
		int length = pAudioStreamUtil->audio_buf_size - pAudioStreamUtil->audio_buf_index;
		if(length > len)
		{
			length = len;
		}
		memcpy(stream, pAudioStreamUtil->audio_buf + pAudioStreamUtil->audio_buf_index, length);
		len -= length;
		stream += length;
		pAudioStreamUtil->audio_buf_index += length;
	}
}

int CStreamMediaUtil::audio_decode_frame(StreamMedia* pStreamMedia, double* ptsPtr)
{
	int nRetLen = 0;
	int nDataSize = 0;
	AudioStreamUtil* pAudioStreamUtil = &pStreamMedia->audioStreamUtil;
	AVPacket *packet = &pAudioStreamUtil->Audio_CurPacket;
	pAudioStreamUtil->audio_pkt_size = 0;
	AVFrame* pMemFrame = avcodec_alloc_frame();
	int got_frame = 0;
	int resampled_data_size = 0;
	double pts;

	while(true)
	{
		if(pStreamMedia->bQuit)	
		{
			return -1;
		}	
		while(pAudioStreamUtil->audio_pkt_size > 0)
		{
			// 返回Packet大小
			nRetLen = avcodec_decode_audio4(pAudioStreamUtil->pAudioCodecCtx,//解码
					pMemFrame, &got_frame, packet);
			if(nRetLen < 0)
			{
				pAudioStreamUtil->audio_pkt_size = SDL_AUDIO_BUFFER_SIZE;
				break;
			}
			if(got_frame == 0)
			{
				continue;	// 没有解码出数据
			}
			// 解码数据大小
			int decoded_data_size = av_samples_get_buffer_size(NULL,
				pMemFrame->channels, 
				pMemFrame->nb_samples, 
				AVSampleFormat(pMemFrame->format), 1);

			pts = pStreamMedia->audioStreamUtil.audioClock;
			*ptsPtr = pts;
			if (strstr(pStreamMedia->pFormatContext->iformat->name,"mpegts")!= NULL)
			{
				double time_base = 90 * 1000;
				int n = 2 * pStreamMedia->audioStreamUtil.pAudioCodecCtx->channels;
				double rate_size = decoded_data_size / n;
				pStreamMedia->audioStreamUtil.audioClock +=  rate_size * time_base/ pStreamMedia->audioStreamUtil.pAudioCodecCtx->sample_rate /100000 * 1000;
			}
			else
			{
				int n = 2 * pStreamMedia->audioStreamUtil.pAudioCodecCtx->channels;
				double rate_size = decoded_data_size / n;
				pStreamMedia->audioStreamUtil.audioClock +=  rate_size / pStreamMedia->audioStreamUtil.pAudioCodecCtx->sample_rate * 1000;
			}

			
			// 每个sample的字节数
			int nSampleBytes = av_get_bytes_per_sample(AVSampleFormat(pMemFrame->format));
			if(decoded_data_size <= 0)	return -1;
			// 声道设计
			int64_t decode_channel_layout = pMemFrame->channel_layout;
			if(pMemFrame->channel_layout != 0 && (pMemFrame->channels == av_get_channel_layout_nb_channels(pMemFrame->channel_layout)))
			{
				decode_channel_layout = pMemFrame->channel_layout;
			}
			else
			{
				decode_channel_layout = av_get_default_channel_layout(pMemFrame->channels);
			}
			int wanted_nb_samples = 0;	// smaples.
			wanted_nb_samples = pMemFrame->nb_samples;
			// 比较
			if( pMemFrame->format != pAudioStreamUtil->audio_src_fmt||
				decode_channel_layout != pAudioStreamUtil->audio_src_channel_layout ||
				pMemFrame->channels != pAudioStreamUtil->audio_src_channels ||
				pMemFrame->sample_rate != pAudioStreamUtil->audio_src_freq ||
				(wanted_nb_samples != pMemFrame->nb_samples && !pAudioStreamUtil->swr_context))
			{
				if (pAudioStreamUtil->swr_context) 
				{
					swr_free(&pAudioStreamUtil->swr_context);
				}
				pAudioStreamUtil->swr_context = swr_alloc_set_opts(NULL,
					pAudioStreamUtil->audio_dest_channel_layout, 
					pAudioStreamUtil->audio_dest_fmt,
					pAudioStreamUtil->audio_dest_freq,
					decode_channel_layout,			
					AVSampleFormat(pMemFrame->format),
					pMemFrame->sample_rate,
					0, NULL);
				if(pAudioStreamUtil->swr_context == NULL || swr_init(pAudioStreamUtil->swr_context) < 0)	
				{
						break;
				}
//				pAudioStreamUtil->audio_src_channels = pAudioStreamUtil->pAudioCodecCtx->channels;
//				pAudioStreamUtil->audio_src_channel_layout = decode_channel_layout;
//				pAudioStreamUtil->audio_src_fmt = pAudioStreamUtil->pAudioCodecCtx->sample_fmt;
//				pAudioStreamUtil->audio_src_freq = pAudioStreamUtil->pAudioCodecCtx->sample_rate;
			}
			if(pAudioStreamUtil->swr_context)
			{
				const uint8_t** in = (const uint8_t**)pMemFrame->extended_data;
				DECLARE_ALIGNED(16,uint8_t,audio_buf2)[AVCODEC_MAX_AUDIO_FRAME_SIZE * 4];
				memset(audio_buf2, 0x00, sizeof(audio_buf2) *  sizeof(uint8_t));
				uint8_t* out[] = {audio_buf2};

				if(wanted_nb_samples != pMemFrame->nb_samples)
				{
					int sample_delta = wanted_nb_samples - pMemFrame->nb_samples;
					sample_delta = sample_delta*pAudioStreamUtil->audio_dest_freq;
					sample_delta = sample_delta/pMemFrame->sample_rate;

					int compensation_distance = wanted_nb_samples * pAudioStreamUtil->audio_dest_freq;
					compensation_distance = compensation_distance / pMemFrame->sample_rate;
	
					if(swr_set_compensation(pAudioStreamUtil->swr_context,sample_delta,compensation_distance) < 0 )
					{
						break;
					}	
				}
				int inSamplesCount = pMemFrame->nb_samples;
				int outSamplesCount = sizeof(audio_buf2);
				outSamplesCount = outSamplesCount / pAudioStreamUtil->audio_dest_channels;
				int nbPerSample = av_get_bytes_per_sample(pAudioStreamUtil->audio_dest_fmt);
				outSamplesCount = outSamplesCount / nbPerSample;
					
				int len2 = swr_convert(pAudioStreamUtil->swr_context, 
					out, outSamplesCount,
					in, inSamplesCount);
				if(len2 < 0)	break;
				if(len2 == outSamplesCount)
				{
					swr_init(pAudioStreamUtil->swr_context);
				}
				resampled_data_size = len2 * pAudioStreamUtil->audio_dest_channels * nbPerSample;
				// 复制解码后的数据
				memcpy(pAudioStreamUtil->audio_buf, audio_buf2, resampled_data_size);
		
				return resampled_data_size;
			}
			else
			{
				pAudioStreamUtil->audio_pkt_data += nRetLen;
				pAudioStreamUtil->audio_pkt_size -= nRetLen;
				// 复制解码后的数据
				memcpy(pAudioStreamUtil->audio_buf, pMemFrame->data[0], decoded_data_size);
		
				return decoded_data_size;
			}
		}
		if(packet->data)
		{
			av_free_packet(packet);		
		}
		if(pStreamMedia->bPause == false)
		{
			memset(packet, 0, sizeof(AVPacket));
			if(PacketQueue_pop(&pAudioStreamUtil->AudioPktQueue, packet, pStreamMedia->bQuit) < 0) // 获取packet
				return -1;
			// Flush buffers, should be called when seeking or when switching to a different stream.
			if(packet->data == pStreamMedia->flush_pkt.data)
			{
				avcodec_flush_buffers(pAudioStreamUtil->pAudioCodecCtx);
			}
			pAudioStreamUtil->audio_pkt_data = packet->data;
			pAudioStreamUtil->audio_pkt_size = packet->size;
		}
		
	}
	avcodec_free_frame(&pMemFrame);
	pMemFrame = NULL;

	return -1;
}

 int av_getframe_size(AVFrame* pFrame)
 {
	 if(pFrame == NULL)		return -1;

	 int nChannels = pFrame->channels;	// channels 声道
	 int nSamples = pFrame->nb_samples;	// samples
	 AVSampleFormat fmt  = AVSampleFormat(pFrame->format);
	 switch(fmt)
	 {
		case AV_SAMPLE_FMT_NONE:return 0;	// none
		case AV_SAMPLE_FMT_U8P:         ///< unsigned 8 bits, planar
		case AV_SAMPLE_FMT_U8:	return 	nChannels * nSamples * 1;		// 8bits
		case AV_SAMPLE_FMT_S16P:        ///< signed 16 bits, planar
		case AV_SAMPLE_FMT_S16:	return 	nChannels * nSamples * 2;		// 16bits
		case AV_SAMPLE_FMT_S32P:        ///< signed 32 bits, planar
		case AV_SAMPLE_FMT_S32:	return 	nChannels * nSamples * 4;		// 16bits
		case AV_SAMPLE_FMT_FLTP:        ///< float, planar
		case AV_SAMPLE_FMT_FLT:	return  nChannels * nSamples * 4;		// float
		case AV_SAMPLE_FMT_DBLP:        ///< double, planar
		case AV_SAMPLE_FMT_DBL:	return  nChannels * nSamples * 8;		// float
	 }
	 
	 return 0;
 }

