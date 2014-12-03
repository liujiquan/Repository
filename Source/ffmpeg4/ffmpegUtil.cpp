//*-----------------------------------------------------------------------------*/
//* Copyright(C) 2014, liujiquan Company All rights reserved. )
//* FileName :   Base64.cpp
//* Author   :   liujiquan
//* DateTime :   11/26/2014
//* Version  :   1.0
//* Comment  :   
//*-----------------------------------------------------------------------------*/
#include "ffmpegUtil.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/************************************************************************/
/* Global Member                                                        */
/************************************************************************/
StreamMedia			g_StreamMedia;		// 流媒体
// -----------------------------------------------------------//
// Function :   initizlize
// Return   :   bool
// Comment  :   环境初始化
// -----------------------------------------------------------//
bool initizlize()
{
	av_register_all();			// register all the , demuxers and  protocols
	avformat_network_init();	// initialization of network components
	avcodec_register_all();		// Register all the codecs, parsers and bitstream filters
	// loads the SDL dynamically linked library and initializes 
	if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER) != 0)
	{
		return false;
	}
		//设置SDL事件状态
	SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
	SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
	SDL_EventState(SDL_USEREVENT, SDL_IGNORE);

	return true;
}

// -----------------------------------------------------------//
// Function :   Uninitialize
// Return   :   bool
// Comment  :   环境释放
// -----------------------------------------------------------//
bool Uninitialize()
{
	SDL_Quit();
	return true;
}

// -----------------------------------------------------------//
// Function :   InitializeStreamMedia
// Param    :   StreamMedia* pStreamMedia
// Return   :   bool
// Comment  :   初始化流媒体
// -----------------------------------------------------------//
bool InitializeStreamMedia(PStreamMedia& pStreamMedia)
{
	pStreamMedia = NULL;
	pStreamMedia = (StreamMedia*)calloc(1, sizeof(StreamMedia));
	if(pStreamMedia == NULL)	return false;

	pStreamMedia->bQuit = false;			// 退出标志
	pStreamMedia->pFormatContext = NULL;	// 总容器
	pStreamMedia->pVideoStream = NULL;		// 视频流
	pStreamMedia->pAudioStream = NULL;		// 音频流
	pStreamMedia->nVideoIndex = -1;			// 视频流索引
	pStreamMedia->nAudioIndex = -1;			// 音频流索引

	pStreamMedia->pVideoCodecCtx = NULL;	// 视频解码容器
	pStreamMedia->pAudioCodecCtx = NULL;	// 音频解码容器
	pStreamMedia->pVideoCodec = NULL;		// 视频解码器
	pStreamMedia->pAudioCodec = NULL;		// 音频解码器

	pStreamMedia->dbVideoDelayTime = 0;		// 视频每幀延迟时间
	pStreamMedia->nWidth = 0;				// 视频显示宽
	pStreamMedia->nHeight = 0;				// 视频显示高
	pStreamMedia->pVideoMainFrame = NULL;	// 视频幀
	pStreamMedia->nShowWidth = 0;			// 视频显示宽
	pStreamMedia->nShowHeight = 0;			// 视频显示高


	memset(pStreamMedia->VideoFrameArr, 0x00, sizeof(VideoFrame)*VIDEO_FRAME_QUEUE_SIZE);	// 解码幀
	pStreamMedia->VideoFrameSize = 0;		// 解码幀大小
	pStreamMedia->VideoFrameCurIndex = 0;	// 当前解码幀索引
	pStreamMedia->VideoFrame_mutex = SDL_CreateMutex();// 解码幀互斥变量
	pStreamMedia->VideoFrame_cond = SDL_CreateCond();  // 解码幀条件变量
	pStreamMedia->ShowVideo_ThreadID = NULL;	// 显示视频  popID
	pStreamMedia->DecodeVideo_ThreadID = NULL;	// 解码视频数据 pushID
	pStreamMedia->Videobmp = NULL;

	pStreamMedia->audio_buf_size = 0;		// 音频Buffer当前总大小
	pStreamMedia->audio_buf_index = 0;		// 音频Buffer已经解析的大小
	memset(pStreamMedia->audio_buf, 0x00, sizeof(uint8_t) * AUDIO_BUF_LEN); // 音频Buffer
	pStreamMedia->audio_pkt_data = NULL;	// 音频包数据
	pStreamMedia->audio_pkt_size = 0;		// 音频包数据大小

	pStreamMedia->running_ThreadID = NULL;	// Read线程ID

	av_init_packet(&pStreamMedia->flush_pkt);// 刷新包
	pStreamMedia->flush_pkt.data = (uint8_t*)("Flush");

	return true;
}

// -----------------------------------------------------------//
// Function :   LoadStreamMedia
// Param    :   PStreamMedia& pStreamMedia
//              const char* szFilePath
// Return   :   bool
// Comment  :   载入一个流媒体
// -----------------------------------------------------------//
bool LoadStreamMedia(PStreamMedia& pStreamMedia, const char* szFilePath)
{
	if(pStreamMedia == NULL)		return false;
	if(szFilePath == NULL)			return false;

	pStreamMedia->pFormatContext = avformat_alloc_context();	// 总容器
	if(pStreamMedia->pFormatContext == NULL)			return false;
	// 1:Open an input stream and read the header
	if(avformat_open_input(&pStreamMedia->pFormatContext, szFilePath, NULL, NULL) != 0)
	{
		return false;
	}
	// 2:Read packets of a media file to get stream information
	if(av_find_stream_info(pStreamMedia->pFormatContext) < 0)	
	{
		return false;
	}
	// 3:VideoIndex/AudioIndex VideoStream/VideoStream
	FindCodecContextIndex(pStreamMedia->nVideoIndex, pStreamMedia->pFormatContext, AVMEDIA_TYPE_VIDEO);
	FindCodecContextIndex(pStreamMedia->nAudioIndex, pStreamMedia->pFormatContext, AVMEDIA_TYPE_AUDIO);
	if(pStreamMedia->nVideoIndex == -1 && pStreamMedia->nAudioIndex == -1)
	{
		return false;
	}
	pStreamMedia->pVideoStream = pStreamMedia->pFormatContext->streams[pStreamMedia->nVideoIndex];
	pStreamMedia->pAudioStream = pStreamMedia->pFormatContext->streams[pStreamMedia->nAudioIndex];
	// 4:dbVideoDelayTime
	double dbFrameRate = av_q2d( pStreamMedia->pVideoStream->r_frame_rate);
	double dbDuration = 1 / dbFrameRate;
	pStreamMedia->dbVideoDelayTime = dbDuration * 1000;
	// 5:CodecCtx Codec
	pStreamMedia->pVideoCodecCtx = pStreamMedia->pVideoStream->codec;							// 视频解码器容器
	pStreamMedia->pAudioCodecCtx = pStreamMedia->pAudioStream->codec;							// 音频解码器容器
	pStreamMedia->pVideoCodec = avcodec_find_decoder(pStreamMedia->pVideoCodecCtx->codec_id);	// 视频解码器
	pStreamMedia->pAudioCodec = avcodec_find_decoder(pStreamMedia->pAudioCodecCtx->codec_id);	// 音频解码器
	
	if(pStreamMedia->pVideoCodecCtx)
	{
		if(avcodec_open2(pStreamMedia->pVideoCodecCtx, pStreamMedia->pVideoCodec, NULL) != 0)	return false;
	}
	if(pStreamMedia->pAudioCodecCtx)
	{
		if(avcodec_open2(pStreamMedia->pAudioCodecCtx, pStreamMedia->pAudioCodec, NULL) != 0)	return false;
	}
	// 6: width/height
	pStreamMedia->nWidth = pStreamMedia->pVideoCodecCtx->width;			// 视频witdh
	pStreamMedia->nHeight = pStreamMedia->pVideoCodecCtx->height;		// 视频witdh
	// 7：VideoFrame
	pStreamMedia->pVideoMainFrame = avcodec_alloc_frame();				// 视频幀

	return true;
}

// -----------------------------------------------------------//
// Function :   PlayStreamMedia
// Param    :   StreamMedia* pStreamMedia
// Return   :   bool
// Comment  :   执行流媒体
// -----------------------------------------------------------//
bool PlayStreamMedia(StreamMedia* pStreamMedia) 
{
	if(pStreamMedia == NULL)	return false;
	pStreamMedia->running_ThreadID = SDL_CreateThread(Running_thread, pStreamMedia);

	return true;
}

// -----------------------------------------------------------//
// Function :   Running_thread
// Param    :   void* lpParam
// Return   :   int
// Comment  :   运行线程
// -----------------------------------------------------------//
int Running_thread(void* lpParam )
{
	StreamMedia* pStreamMedia = (StreamMedia*)lpParam;
	if(pStreamMedia == NULL)	return false;

	// 1:CreateWindow
//	SDL_Overlay* screen = CreateWindow_(pStreamMedia->nShowWidth, pStreamMedia->nShowHeight);
	pStreamMedia->Videobmp = CreateWindow_(pStreamMedia->nShowWidth, pStreamMedia->nShowHeight);
	// 2：StartVideo
	if(StartVideo(pStreamMedia) == false)		return false;
	// 3：StartAudio
//	if(StartAudio(pStreamMedia) == false)		return false;
	// 4:ReadFrame
	SDL_Event event;
	while(true)
	{
		/* if the queue are full, no need to read more */
		if (pStreamMedia->AudioPktQueue.size > MAX_QUEUE_SIZE||pStreamMedia->AudioPktQueue.nb_packets > MIN_FRAMES )
		{
			/* wait 10 ms */
			SDL_Delay(10);
			continue;
		}


		AVPacket packet;
		AVPacket* packet1 = & packet;

		int intRet = av_read_frame(pStreamMedia->pFormatContext, packet1);	// ReadFrame
		if(intRet < 0)	break;
		// 视频
		if(packet1->stream_index == pStreamMedia->nVideoIndex)
		{
			// 解码视频
/*			int got_picture;
			intRet = avcodec_decode_video2(pStreamMedia->pVideoCodecCtx, 
											pStreamMedia->pVideoMainFrame, 
											&got_picture, 
											packet1);
			if(got_picture)
			{
				// Show
				ShowWindow((AVPicture*)pStreamMedia->pVideoMainFrame, screen, 
					pStreamMedia->pVideoCodecCtx->pix_fmt, 
					pStreamMedia->nWidth, pStreamMedia->nHeight,
					pStreamMedia->nShowWidth, pStreamMedia->nShowHeight); 
				// 延迟
				SDL_Delay(pStreamMedia->dbVideoDelayTime);
				
			}
*/
			PacketQueue_push(&pStreamMedia->VideoPktQueue, packet1);
		}
		// 音频
		else if(packet1->stream_index == pStreamMedia->nAudioIndex)
		{
//			PacketQueue_push(&pStreamMedia->AudioPktQueue, packet1);
		}
		else
		{
			av_free_packet(packet1);
		}
		SDL_PollEvent(&event);
		if(event.type == SDL_QUIT)
		{
			ReleaseStreamMedia(pStreamMedia);// release
			break;
		}
	}
	PacketQueue_abort(&pStreamMedia->AudioPktQueue);
	PacketQueue_clear(&pStreamMedia->AudioPktQueue);

	return true;
}
// -----------------------------------------------------------//
// Function :   UnInitializeStreamMedia
// Param    :   PStreamMedia& pStreamMedia
// Return   :   bool
// Comment  :   Release流媒体
// -----------------------------------------------------------//
bool UnInitializeStreamMedia(PStreamMedia& pStreamMedia)
{
	if(pStreamMedia)
	{
		ReleaseStreamMedia(pStreamMedia);

		free(pStreamMedia);
		pStreamMedia = NULL;
	}

	return true;
}

bool ReleaseStreamMedia(PStreamMedia& pStreamMedia)
{
	if(pStreamMedia)
	{
		pStreamMedia->bQuit = true;
		if(pStreamMedia->pFormatContext)					// 总容器释放
		{
			avformat_free_context(pStreamMedia->pFormatContext);
			pStreamMedia->pFormatContext = NULL;
		}
		if(pStreamMedia->pVideoMainFrame)					// 视频幀
		{
			avcodec_free_frame(&pStreamMedia->pVideoMainFrame);
			pStreamMedia->pVideoMainFrame = NULL;
		}
		if(pStreamMedia->running_ThreadID)					// Read线程
		{
			SDL_KillThread(pStreamMedia->running_ThreadID);
			pStreamMedia->running_ThreadID = NULL;
		}
		if(pStreamMedia->ShowVideo_ThreadID)					// 视频线程
		{
			SDL_KillThread(pStreamMedia->ShowVideo_ThreadID);
			pStreamMedia->ShowVideo_ThreadID = NULL;
		}
		if(pStreamMedia->DecodeVideo_ThreadID)					// 视频线程
		{
			SDL_KillThread(pStreamMedia->DecodeVideo_ThreadID);
			pStreamMedia->DecodeVideo_ThreadID = NULL;
		}
		if(pStreamMedia->Videobmp)							// Video Window
		{
			SDL_FreeYUVOverlay(pStreamMedia->Videobmp);
			pStreamMedia->Videobmp = NULL;
		}
		PacketQueue_destory(&pStreamMedia->AudioPktQueue);	// 音频队列
	}

	return true;
}

// -----------------------------------------------------------//
// Function :   RelationHwnd
// Param    :   int hwnd
// Return   :   bool
// Comment  :   关联窗体
// -----------------------------------------------------------//
bool RelationHwnd(int hwnd)
{
	if(hwnd == 0)	return false;

	char sdl_var[64];    
	sprintf_s(sdl_var, "SDL_WINDOWID=%d", hwnd);    //主窗口句柄
	SDL_putenv(sdl_var);   
	char *myvalue = SDL_getenv("SDL_WINDOWID");   //让SDL取得窗口ID  

	return myvalue != NULL;
}

// -----------------------------------------------------------//
// Function :   FindCodecContextIndex
// Param    :   int& stream_idx
//              AVFormatContext * fmt_ctx
//              enum AVMediaType type
// Return   :   bool
// Comment  :   查找Stream index
// -----------------------------------------------------------//
bool FindCodecContextIndex(int& stream_idx, AVFormatContext * fmt_ctx, enum AVMediaType type)
{
	if(fmt_ctx == NULL)	return false;
	
	int nRet = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
	if(nRet != -1)	stream_idx = nRet;
	else			stream_idx = -1;

	return nRet == 0 ? true : false;
}

// -----------------------------------------------------------//
// Function :   CreateWindow_
// Param    :   int nWidth
//              int nHeight
//				int nFlags
// Return   :   SDL_Overlay*
// Comment  :   创建显示界面(必须malloc)
// -----------------------------------------------------------//
SDL_Overlay* CreateWindow_(int nWidth, int nHeight, int nFlags/* = 0*/)
{
	if(nWidth < 0 || nHeight < 0)	return false;

	SDL_Surface* screen = SDL_SetVideoMode(nWidth, nHeight, 0, 0);
	if(screen)
	{
		return SDL_CreateYUVOverlay(nWidth, nHeight, SDL_YV12_OVERLAY, screen);
	}

	return NULL;
}

// -----------------------------------------------------------//
// Function :   ShowWindow
// Param    :   AVPicture* pPicture		
//              SDL_Overlay* bmp		
//              enum PixelFormat foramt
//              int nWid
//              int nHei
//              int nShowWid
//              int nShowHei
// Return   :   bool
// Comment  :   显示一幀数据
// -----------------------------------------------------------//
bool ShowWindow(AVPicture* pPicture,  SDL_Overlay* bmp, enum PixelFormat foramt, int nWid, int nHei, int nShowWid, int nShowHei)
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

// -----------------------------------------------------------//
// Function :   StartVideo
// Param    :   StreamMedia* pStreamMedia
// Return   :   bool
// Comment  :   创建视频线程
// -----------------------------------------------------------//
bool StartVideo(StreamMedia* pStreamMedia)		// 
{
	if(pStreamMedia == NULL)					return false;
	
	PacketQueue_init(&pStreamMedia->VideoPktQueue);								// 初始化视频链表						
	PacketQueue_start(&pStreamMedia->VideoPktQueue, &pStreamMedia->flush_pkt);	// 刷新packet的初始化 

	pStreamMedia->ShowVideo_ThreadID = SDL_CreateThread(ShowVideo_thread, pStreamMedia);	// 显示线程
	pStreamMedia->DecodeVideo_ThreadID = SDL_CreateThread(DecodeVideo_thread, pStreamMedia);// 解码线程
	
	return pStreamMedia->DecodeVideo_ThreadID != NULL;
}

// -----------------------------------------------------------//
// Function :   StartAudio
// Param    :   StreamMedia* pStreamMedia
// Return   :   bool
// Comment  :   创建音频回调函数 初始化+打开音频设备 利用SDL_AudioSpec来设置
// -----------------------------------------------------------//
SDL_AudioSpec audio_spec, spec;
bool StartAudio(StreamMedia* pStreamMedia)
{	
	if(pStreamMedia == NULL)					return false;
	if(pStreamMedia->pAudioCodecCtx == NULL)	return false;

	audio_spec.freq = pStreamMedia->pAudioCodecCtx->sample_rate;	//频率
	switch(pStreamMedia->pAudioCodecCtx->sample_fmt)
	{
	case AV_SAMPLE_FMT_U8:		audio_spec.format = AUDIO_S8;		 break;
	case AV_SAMPLE_FMT_S16:		audio_spec.format = AUDIO_S16SYS;	 break;
	case AV_SAMPLE_FMT_S32:		audio_spec.format = AUDIO_S16SYS;	 break;
	default:
		audio_spec.format = AUDIO_S16SYS;	 break;
	}
	audio_spec.channels = pStreamMedia->pAudioCodecCtx->channels;	//
	audio_spec.size = 0;
	audio_spec.silence = 0;
	audio_spec.samples = SDL_AUDIO_BUFFER_SIZE;	   // 1024
	audio_spec.callback = audio_callback;
	audio_spec.userdata = pStreamMedia;

	if(SDL_OpenAudio(&audio_spec, &spec) != 0)
	{
		return false;
	}
	// 初始化音频列表
	pStreamMedia->audio_buf_size = 0;
	pStreamMedia->audio_buf_index = 0;
	memset(&pStreamMedia->Audio_CurPacket, 0x00, sizeof(AVPacket));
	PacketQueue_init(&pStreamMedia->AudioPktQueue);			// 初始化音频链表
	PacketQueue_start(&pStreamMedia->AudioPktQueue, &pStreamMedia->flush_pkt); // 刷新packet的初始化
	SDL_PauseAudio(0);	// pause call_back function
	
	return true;
}

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
	PacketQueue_push(quene, packet);
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
	pktList = (AVPacketList*)av_malloc(sizeof(pktList));
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
			av_free(packet);
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
	SDL_CondSignal(quene->cond);
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
		av_freep(pkt1);

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
		SDL_DestroyCond(quene->cond);
	if(quene->mutex)
		SDL_DestroyMutex(quene->mutex);

	return 0;
}

// -----------------------------------------------------------//
// Function :   audio_callback
// Param    :   void *userdata
//              Uint8 *stream
//              int len
// Return   :   void
// Comment  :   音频back函数
// -----------------------------------------------------------//
void audio_callback(void *userdata, Uint8 *stream, int len)	
{
	StreamMedia* pStreamMedia = (StreamMedia*)userdata;
	if(pStreamMedia == NULL)					return;
	if(pStreamMedia->pAudioCodecCtx == NULL)	return;
	double pts;

	while(len > 0 )
	{
		if(pStreamMedia->audio_buf_index >= pStreamMedia->audio_buf_size)
		{
			/* We have already sent all our data; get more */
			int audio_size = audio_decode_frame(pStreamMedia,// decode		
				pStreamMedia->audio_buf, 
				sizeof(pStreamMedia->audio_buf), 
				&pts);
			if(audio_size < 0)
			{
				/* If error, output silence */
				pStreamMedia->audio_buf_size = 1024;
				memset(pStreamMedia->audio_buf, 0, pStreamMedia->audio_buf_size);
			}
			else
			{
				pStreamMedia->audio_buf_size = audio_size;
			}

			pStreamMedia->audio_buf_index = 0;
		}
		int length = pStreamMedia->audio_buf_size - pStreamMedia->audio_buf_index;
		if(length > len)
		{
			length = len;
		}
//		memcpy(stream, pStreamMedia->audio_buf, length);
		len -= length;
		stream += length;
		pStreamMedia->audio_buf_index += length;
	}
}

// -----------------------------------------------------------//
// Function :   audio_decode_frame
// Param    :   StreamMedia* pStreamMedia
//              uint8_t* audio_buf
//              int nbufSize
//              double *pts_ptr
// Return   :   int
// Comment  :   解码函数
// -----------------------------------------------------------//
int audio_decode_frame(StreamMedia* pStreamMedia, uint8_t* audio_buf, int nbufSize, double *pts_ptr)
{
	int nRetLen = 0;
	int nDataSize = 0;
	AVPacket *packet = &pStreamMedia->Audio_CurPacket;
	pStreamMedia->audio_pkt_size = 0;

	while(true)
	{
		if(pStreamMedia->bQuit)	return -1;
			
		while(pStreamMedia->audio_pkt_size > 0)
		{
			nDataSize = nbufSize;
			nRetLen = avcodec_decode_audio3(pStreamMedia->pAudioCodecCtx,//解码
					(int16_t*)audio_buf, &nDataSize, packet);
			if(nRetLen < 0)
			{
				pStreamMedia->audio_pkt_size = 0;
				break;
			}
			pStreamMedia->audio_pkt_data += nRetLen;
			pStreamMedia->audio_pkt_size -= nRetLen;
			if(nDataSize <= 0)
			{
				continue;	// 暂时没有数据
			}

			return nRetLen;
		}
		if(packet->data)
		{
			av_free_packet(packet);		
		}
		if(pStreamMedia->bQuit)	return -1;
		if(PacketQueue_pop(&pStreamMedia->AudioPktQueue, packet, pStreamMedia->bQuit) < 0) // 获取packet
			return -1;
		// Flush buffers, should be called when seeking or when switching to a different stream.
		if(packet->data == pStreamMedia->flush_pkt.data)
		{
			avcodec_flush_buffers(pStreamMedia->pAudioCodecCtx);
		}
		pStreamMedia->audio_pkt_data = packet->data;
		pStreamMedia->audio_pkt_size = packet->size;
	}

	return -1;
}

// 视频显示线程	pop数据显示
int ShowVideo_thread(void* lpParam)
{
	StreamMedia* pStreamMedia = (StreamMedia*)lpParam;
	if(pStreamMedia == NULL)	return -1;

	while(true)
	{
		VideoFrame* videoFrame = NULL;
		if(pStreamMedia->bQuit)		break;

		if(pStreamMedia->VideoFrameSize == 0)	//No Data
		{
			SDL_Delay(1);
			continue;
		}
		else
		{
			// 取数据
			videoFrame = &pStreamMedia->VideoFrameArr[pStreamMedia->VideoFrameCurIndex];	
			SDL_Delay(pStreamMedia->dbVideoDelayTime);

			pStreamMedia->VideoFrameCurIndex++;
			if(pStreamMedia->VideoFrameCurIndex == VIDEO_FRAME_QUEUE_SIZE)
			{
				pStreamMedia->VideoFrameCurIndex = 0;
			}
			SDL_LockMutex(pStreamMedia->VideoFrame_mutex);
			pStreamMedia->VideoFrameSize--;
			SDL_UnlockMutex(pStreamMedia->VideoFrame_mutex);

			if(videoFrame->bmp)
			{
				SDL_Rect rect;
				rect.x = 0;
				rect.y = 0;
				rect.w = pStreamMedia->nShowWidth;
				rect.h = pStreamMedia->nShowHeight;

				SDL_DisplayYUVOverlay(videoFrame->bmp, &rect);
			}
		}
	}
	
	return 0;
}

// -----------------------------------------------------------//
// Function :   DecodeVideo_thread
// Param    :   void* lpParam
// Return   :   int
// Comment  :   视频刷新线程
// -----------------------------------------------------------//
int DecodeVideo_thread(void* lpParam)	
{
	StreamMedia* pStreamMedia = (StreamMedia*)lpParam;
	if(pStreamMedia == NULL)	return -1;

	AVFrame* pMemFrame = avcodec_alloc_frame();
	
	while(true)
	{
		if(pStreamMedia->bQuit)	break;
		AVPacket packet;
		// pop
		if(PacketQueue_pop(&pStreamMedia->VideoPktQueue, &packet, pStreamMedia->bQuit) < 0) // 获取packet
		{
			// No data
			break;
		}
		if(packet.data == pStreamMedia->flush_pkt.data)
		{
			avcodec_flush_buffers(pStreamMedia->pVideoCodecCtx);
			continue;
		}
		// Decode video frame
		int got_picture;
		int intRet = avcodec_decode_video2(pStreamMedia->pVideoCodecCtx, 
										pStreamMedia->pVideoMainFrame, 
										&got_picture, 
										&packet);
		av_free_packet(&packet);// free push/alloc
		if(got_picture)
		{
			if(FillPicture(pStreamMedia, pMemFrame) < 0)
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

	return 0;
}

// -----------------------------------------------------------//
// Function :   FillPicture
// Param    :   StreamMedia* pStreamMedia
//              AVFrame* pMemFrame
// Return   :   int
// Comment  :   填充一幀数据
// -----------------------------------------------------------//
int FillPicture(StreamMedia* pStreamMedia, AVFrame* pMemFrame)
{
	if(pStreamMedia == NULL)	return -1;

	VideoFrame* videoFrame = NULL;
	struct SwsContext* img_convert_ctx = 0;	// Convert
	AVPicture picture;
	AVPicture* pPicture = (AVPicture*)pMemFrame;

	/* wait until we have space for a new pic */
	SDL_LockMutex(pStreamMedia->VideoFrame_mutex);
	while (pStreamMedia->VideoFrameSize >= VIDEO_FRAME_QUEUE_SIZE ) 
	{
		SDL_CondWait(pStreamMedia->VideoFrame_cond, pStreamMedia->VideoFrame_mutex);
	}
	SDL_UnlockMutex(pStreamMedia->VideoFrame_mutex);
	// videoFrame
	videoFrame = &pStreamMedia->VideoFrameArr[pStreamMedia->VideoFrameCurIndex];
	
	if(videoFrame->bmp == NULL)
	{
		videoFrame->bmp = pStreamMedia->Videobmp;
	}
	if(videoFrame->bmp == NULL)	return -1;

	SDL_LockYUVOverlay(videoFrame->bmp);
	picture.data[0] = videoFrame->bmp->pixels[0];
	picture.data[1] = videoFrame->bmp->pixels[2];
	picture.data[2] = videoFrame->bmp->pixels[1];
	picture.linesize[0] = videoFrame->bmp->pitches[0];
	picture.linesize[1] = videoFrame->bmp->pitches[2];
	picture.linesize[2] = videoFrame->bmp->pitches[1];
	// init convert
	img_convert_ctx = sws_getCachedContext(img_convert_ctx,
		pStreamMedia->nWidth, pStreamMedia->nHeight, pStreamMedia->pVideoCodecCtx->pix_fmt, 
		pStreamMedia->nShowWidth, pStreamMedia->nShowHeight, PIX_FMT_YUV420P,SWS_X, 0, 0, 0);
	// convert
	sws_scale(img_convert_ctx, pPicture->data,  pPicture->linesize,
		0, pStreamMedia->nHeight,picture.data, picture.linesize);
	SDL_UnlockYUVOverlay(videoFrame->bmp);
	
	return 0;
}