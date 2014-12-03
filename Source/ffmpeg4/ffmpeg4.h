
// ffmpeg4.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// Cffmpeg4App:
// See ffmpeg4.cpp for the implementation of this class
//

class Cffmpeg4App : public CWinApp
{
public:
	Cffmpeg4App();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern Cffmpeg4App theApp;