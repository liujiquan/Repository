//*-----------------------------------------------------------------------------*/
//* Copyright(C) 2014, liujiquan Company All rights reserved. )
//* FileName :   header.cpp
//* Author   :   liujiquan
//* DateTime :   11/27/2014
//* Version  :   1.0
//* Comment  :   
//*-----------------------------------------------------------------------------*/
#include "header.h"

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
// Function :   RelationHwnd
// Param    :   long hwnd
// Return   :   bool
// Comment  :   关联窗体
// -----------------------------------------------------------//
bool RelationHwnd(long hwnd)
{
	if(hwnd == 0)	return false;

	char sdl_var[64];    
	sprintf_s(sdl_var, "SDL_WINDOWID=%d", hwnd);    //主窗口句柄
	SDL_putenv(sdl_var);   
	char *myvalue = SDL_getenv("SDL_WINDOWID");   //让SDL取得窗口ID  

	return myvalue != NULL;
}

SDL_Surface* CreateSurface(int nWidth, int nHeight, int nFlags/* = SDL_HWSURFACE | SDL_ASYNCBLIT | SDL_HWACCEL*/)
{
	if(nWidth < 0 || nHeight < 0)	return NULL;

	return SDL_SetVideoMode(nWidth, nHeight, 0, 0);
}

SDL_Overlay* CreateWindow_(int nWidth, int nHeight, SDL_Surface* screen)
{
	if(nWidth < 0 || nHeight < 0 || screen == NULL)	return NULL;

	return SDL_CreateYUVOverlay(nWidth, nHeight, SDL_YV12_OVERLAY, screen);
}

char* GetAVPixelFormatInfo(AVPixelFormat pix_fmt)
{
	switch (pix_fmt)
	{
	case	AV_PIX_FMT_NONE :				return "AV_PIX_FMT_NONE";
	case	AV_PIX_FMT_YUV420P:				return "AV_PIX_FMT_YUV420P";
	case	AV_PIX_FMT_YUYV422:				return "AV_PIX_FMT_YUYV422";
	case	AV_PIX_FMT_RGB24:				return "AV_PIX_FMT_RGB24";
	case	AV_PIX_FMT_BGR24:				return "AV_PIX_FMT_BGR24";
	case	AV_PIX_FMT_YUV422P:				return "AV_PIX_FMT_YUV422P";
	case	AV_PIX_FMT_YUV444P:				return "AV_PIX_FMT_YUV444P";
	case	AV_PIX_FMT_YUV410P:				return "AV_PIX_FMT_YUV410P";
	case	AV_PIX_FMT_YUV411P:				return "AV_PIX_FMT_YUV411P";
	case	AV_PIX_FMT_GRAY8:				return "AV_PIX_FMT_GRAY8";
	case	AV_PIX_FMT_MONOWHITE:			return "AV_PIX_FMT_MONOWHITE";
	case	AV_PIX_FMT_MONOBLACK:			return "AV_PIX_FMT_MONOBLACK";
	case	AV_PIX_FMT_PAL8:				return "AV_PIX_FMT_PAL8";
	case	AV_PIX_FMT_YUVJ420P:			return "AV_PIX_FMT_YUVJ420P";
	case	AV_PIX_FMT_YUVJ422P:			return "AV_PIX_FMT_YUVJ422P";
	case	AV_PIX_FMT_YUVJ444P:			return "AV_PIX_FMT_YUVJ444P";
	case	AV_PIX_FMT_XVMC_MPEG2_MC:		return "AV_PIX_FMT_XVMC_MPEG2_MC";
	case	AV_PIX_FMT_XVMC_MPEG2_IDCT:		return "AV_PIX_FMT_XVMC_MPEG2_IDCT";
	case	AV_PIX_FMT_UYVY422:				return "AV_PIX_FMT_UYVY422";
	case	AV_PIX_FMT_UYYVYY411:			return "AV_PIX_FMT_UYYVYY411";
	case	AV_PIX_FMT_BGR8:				return "AV_PIX_FMT_BGR8";
	case	AV_PIX_FMT_BGR4:				return "AV_PIX_FMT_BGR4";
	case	AV_PIX_FMT_BGR4_BYTE:			return "AV_PIX_FMT_BGR4_BYTE";
	case	AV_PIX_FMT_RGB8:				return "AV_PIX_FMT_RGB8";
	case	AV_PIX_FMT_RGB4:				return "AV_PIX_FMT_RGB4";
	case	AV_PIX_FMT_RGB4_BYTE:			return "AV_PIX_FMT_RGB4_BYTE";
	case	AV_PIX_FMT_NV12:				return "AV_PIX_FMT_NV12";
	case	AV_PIX_FMT_NV21:				return "AV_PIX_FMT_NV21";
	case	AV_PIX_FMT_ARGB:				return "AV_PIX_FMT_ARGB";
	case	AV_PIX_FMT_RGBA:				return "AV_PIX_FMT_RGBA";
	case	AV_PIX_FMT_ABGR:				return "AV_PIX_FMT_ABGR";
	case	AV_PIX_FMT_BGRA:				return "AV_PIX_FMT_BGRA";
		
	case	AV_PIX_FMT_GRAY16BE:			return "AV_PIX_FMT_GRAY16BE";
	case	AV_PIX_FMT_GRAY16LE:			return "AV_PIX_FMT_GRAY16LE";
	case	AV_PIX_FMT_YUV440P:				return "AV_PIX_FMT_YUV440P";
	case	AV_PIX_FMT_YUVJ440P:			return "AV_PIX_FMT_YUVJ440P";
	case	AV_PIX_FMT_YUVA420P:			return "AV_PIX_FMT_YUVA420P";
	case	AV_PIX_FMT_VDPAU_H264:			return "AV_PIX_FMT_VDPAU_H264";
	case	AV_PIX_FMT_VDPAU_MPEG1:			return "AV_PIX_FMT_VDPAU_MPEG1";
	case	AV_PIX_FMT_VDPAU_MPEG2:			return "AV_PIX_FMT_VDPAU_MPEG2";
	case	AV_PIX_FMT_VDPAU_WMV3:			return "AV_PIX_FMT_VDPAU_WMV3";
	case	AV_PIX_FMT_VDPAU_VC1:			return "AV_PIX_FMT_VDPAU_VC1";
	case	AV_PIX_FMT_RGB48BE:				return "AV_PIX_FMT_RGB48BE";
	case	AV_PIX_FMT_RGB48LE:				return "AV_PIX_FMT_RGB48LE";

	case	AV_PIX_FMT_RGB565BE:			return "AV_PIX_FMT_RGB48LE";
	case	AV_PIX_FMT_RGB565LE:			return "AV_PIX_FMT_RGB565LE";
	case	AV_PIX_FMT_RGB555BE:			return "AV_PIX_FMT_RGB555BE";
	case	AV_PIX_FMT_RGB555LE:			return "AV_PIX_FMT_RGB555LE";

	case	AV_PIX_FMT_BGR565BE:			return "AV_PIX_FMT_BGR565BE";
	case	AV_PIX_FMT_BGR565LE:			return "AV_PIX_FMT_BGR565LE";
	case	AV_PIX_FMT_BGR555BE:			return "AV_PIX_FMT_BGR555BE";
	case	AV_PIX_FMT_BGR555LE:			return "AV_PIX_FMT_BGR555LE";

	case	AV_PIX_FMT_VAAPI_MOCO:			return "AV_PIX_FMT_VAAPI_MOCO";
	case	AV_PIX_FMT_VAAPI_IDCT:			return "AV_PIX_FMT_VAAPI_IDCT";
	case	AV_PIX_FMT_VAAPI_VLD:			return "AV_PIX_FMT_VAAPI_VLD";

	case	AV_PIX_FMT_YUV420P16LE:			return "AV_PIX_FMT_YUV420P16LE";
	case	AV_PIX_FMT_YUV420P16BE:			return "AV_PIX_FMT_YUV420P16BE";
	case	AV_PIX_FMT_YUV422P16LE:			return "AV_PIX_FMT_YUV422P16LE";
	case	AV_PIX_FMT_YUV422P16BE:			return "AV_PIX_FMT_YUV422P16BE";
	case	AV_PIX_FMT_YUV444P16LE:			return "AV_PIX_FMT_YUV444P16LE";
	case	AV_PIX_FMT_YUV444P16BE:			return "AV_PIX_FMT_YUV444P16BE";
	case	AV_PIX_FMT_VDPAU_MPEG4:			return "AV_PIX_FMT_VDPAU_MPEG4";
	case	AV_PIX_FMT_DXVA2_VLD:			return "AV_PIX_FMT_DXVA2_VLD";

	case	AV_PIX_FMT_RGB444LE:			return "AV_PIX_FMT_RGB444LE";
	case	AV_PIX_FMT_RGB444BE:			return "AV_PIX_FMT_RGB444BE";
	case	AV_PIX_FMT_BGR444LE:			return "AV_PIX_FMT_BGR444LE";
	case	AV_PIX_FMT_BGR444BE:			return "AV_PIX_FMT_BGR444BE";
	case	AV_PIX_FMT_GRAY8A:				return "AV_PIX_FMT_GRAY8A";
	case	AV_PIX_FMT_BGR48BE:				return "AV_PIX_FMT_BGR48BE";
	case	AV_PIX_FMT_BGR48LE:				return "AV_PIX_FMT_BGR48LE";

	case	AV_PIX_FMT_YUV420P9BE:			return "AV_PIX_FMT_YUV420P9BE";
	case	AV_PIX_FMT_YUV420P9LE:			return "AV_PIX_FMT_YUV420P9LE";
	case	AV_PIX_FMT_YUV420P10BE:			return "AV_PIX_FMT_YUV420P10BE";
	case	AV_PIX_FMT_YUV420P10LE:			return "AV_PIX_FMT_YUV420P10LE";
	case	AV_PIX_FMT_YUV422P10BE:			return "AV_PIX_FMT_YUV422P10BE";
	case	AV_PIX_FMT_YUV422P10LE:			return "AV_PIX_FMT_YUV422P10LE";
	case	AV_PIX_FMT_YUV444P9BE:			return "AV_PIX_FMT_YUV444P9BE";
	case	AV_PIX_FMT_YUV444P9LE:			return "AV_PIX_FMT_YUV444P9LE";
	case	AV_PIX_FMT_YUV444P10BE:			return "AV_PIX_FMT_YUV444P10BE";
	case	AV_PIX_FMT_YUV444P10LE:			return "AV_PIX_FMT_YUV444P10LE";
	case	AV_PIX_FMT_YUV422P9BE:			return "AV_PIX_FMT_YUV422P9BE";
	case	AV_PIX_FMT_YUV422P9LE:			return "AV_PIX_FMT_YUV422P9LE";
	case	AV_PIX_FMT_VDA_VLD:				return "AV_PIX_FMT_VDA_VLD";
	}

	return "NULL";
}