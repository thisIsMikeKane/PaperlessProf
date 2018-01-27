/*************************************************************************************************
*
* File: MagnifierSample.cpp
*
* Description: Implements a simple control that magnifies the screen, using the 
* Magnification API.
*
* The magnification window is quarter-screen by default but can be resized.
* To make it full-screen, use the Maximize button or double-click the caption
* bar. To return to partial-screen mode, click on the application icon in the 
* taskbar and press ESC. 
*
* In full-screen mode, all keystrokes and mouse clicks are passed through to the
* underlying focused application. In partial-screen mode, the window can receive the 
* focus. 
*
* Multiple monitors are not supported.
*
* 
* Requirements: To compile, link to Magnification.lib. The sample must be run with 
* elevated privileges.
*
* The sample is not designed for multimonitor setups.
* 
*  This file is part of the Microsoft WinfFX SDK Code Samples.
* 
*  Copyright (C) Microsoft Corporation.  All rights reserved.
* 
* This source code is intended only as a supplement to Microsoft
* Development Tools and/or on-line documentation.  See these other
* materials for detailed information regarding Microsoft code samples.
* 
* THIS CODE AND INFORMATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
* PARTICULAR PURPOSE.
* 
*************************************************************************************************/

// Ensure that the following definition is in effect before winuser.h is included.
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501    
#endif

#include <windows.h>
#include <wincodec.h>
#include <ShellScalingAPI.h>
#include <magnification.h>

// For simplicity, the sample uses a constant magnification factor.
#define RESTOREDWINDOWSTYLES WS_SIZEBOX | WS_SYSMENU | WS_CLIPCHILDREN | WS_CAPTION

// Global variables and strings.
HINSTANCE           hInst;
const TCHAR         WindowClassName[]= TEXT("MirrorWindow");
const TCHAR         WindowTitle[]= TEXT("Screen Mirror");
const UINT          timerInterval = 16; // close to the refresh rate @60hz
HWND                hwndMag; // Handle of the area to be magnified
HWND                hwndHost; // Handle of the area mirror
RECT                magWindowRect;
RECT                hostWindowRect;
RECT				sourceRect;
FLOAT				magfactor = 1;

// Forward declarations.
ATOM                RegisterHostWindowClass(HINSTANCE hInstance);
BOOL                SetupMagnifier(HINSTANCE hinst);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void CALLBACK       UpdateMagWindow(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
void                GoFullScreen();
void                GoPartialScreen();
BOOL                isFullScreen = FALSE;

//
// FUNCTION: WinMain()
//
// PURPOSE: Entry point for the application.
//
int APIENTRY WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE /*hPrevInstance*/,
                     _In_ LPSTR     /*lpCmdLine*/,
                     _In_ int       nCmdShow)
{
	LPWSTR *szArglist;
	int nArgs;

	// Parse command line arguments if there are 8 
	//  st sb sl sr wt wb wl wr
	szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
	if ((NULL == szArglist) || (nArgs != 92))	{
		// Set bounds of the area to be magnified
		sourceRect.top = GetSystemMetrics(SM_CYSCREEN) * 3 / 4;
		sourceRect.bottom = GetSystemMetrics(SM_CYSCREEN);  // bottom quarter of screen
		sourceRect.left = 0;
		sourceRect.right = GetSystemMetrics(SM_CXSCREEN);

		// Set bounds of the window that hosts the magnified image
		hostWindowRect.top = 0;
		hostWindowRect.bottom = 0 - -2520;
		hostWindowRect.left = 0;
		hostWindowRect.right = 5691 - 1211;

		/*
		// Set bounds of the area to be magnified
		sourceRect.top = GetSystemMetrics(SM_CYSCREEN) * 3 / 4;
		sourceRect.bottom = GetSystemMetrics(SM_CYSCREEN);  // bottom quarter of screen
		sourceRect.left = 0;
		sourceRect.right = GetSystemMetrics(SM_CXSCREEN);

		// Set bounds of the window that hosts the magnified image
		hostWindowRect.top = 0;
		hostWindowRect.bottom = GetSystemMetrics(SM_CYSCREEN) / 4;  // top quarter of screen
		hostWindowRect.left = 0;
		hostWindowRect.right = GetSystemMetrics(SM_CXSCREEN);
		*/
	} else {
		// Set bounds of the area to be magnified
		sourceRect.top		= _wtoi(szArglist[1]);
		sourceRect.bottom	= _wtoi(szArglist[2]);
		sourceRect.left		= _wtoi(szArglist[3]);
		sourceRect.right	= _wtoi(szArglist[4]);

		// Set bounds of the window that hosts the magnified image
		hostWindowRect.top		= _wtoi(szArglist[5]);
		hostWindowRect.bottom	= _wtoi(szArglist[6]);
		hostWindowRect.left		= _wtoi(szArglist[7]);
		hostWindowRect.right	= _wtoi(szArglist[8]);
	}

	if (FALSE == MagInitialize())
    {
        return 0;
    }
    if (FALSE == SetupMagnifier(hInstance))
    {
        return 0;
    }

    ShowWindow(hwndHost, nCmdShow);
    UpdateWindow(hwndHost);

    // Create a timer to update the control.
    UINT_PTR timerId = SetTimer(hwndHost, 0, timerInterval, UpdateMagWindow);

    // Main message loop.
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Shut down.
    KillTimer(NULL, timerId);
    MagUninitialize();
    return (int) msg.wParam;
}

//
// FUNCTION: HostWndProc()
//
// PURPOSE: Window procedure for the window that hosts the magnifier control.
//
LRESULT CALLBACK HostWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) 
    {
    case WM_SYSCOMMAND:
		return DefWindowProc(hWnd, message, wParam, lParam);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    case WM_SIZE:
        if ( hwndMag != NULL )
        {
            GetClientRect(hWnd, &magWindowRect);
            // Resize the control to fill the window.
            SetWindowPos(hwndMag, NULL, 
                magWindowRect.left, magWindowRect.top, magWindowRect.right, magWindowRect.bottom, 0);

			// Pass the source rectangle to the magnifier control.
			MagSetWindowSource(hwndMag, sourceRect);

			// Set the magnification factor.
			magfactor = float(magWindowRect.right - magWindowRect.left) / float(sourceRect.right - sourceRect.left);
			MAGTRANSFORM matrix;
			memset(&matrix, 0, sizeof(matrix));
			matrix.v[0][0] = magfactor;
			matrix.v[1][1] = magfactor;
			matrix.v[2][2] = 1.0f;

			MagSetWindowTransform(hwndMag, &matrix);
        }
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;  
}

//
//  FUNCTION: RegisterHostWindowClass()
//
//  PURPOSE: Registers the window class for the window that contains the magnification control.
//
ATOM RegisterHostWindowClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex = {};

    wcex.cbSize = sizeof(WNDCLASSEX); 
    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = HostWndProc;
    wcex.hInstance      = hInstance;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(1 + COLOR_BTNFACE);
    wcex.lpszClassName  = WindowClassName;

    return RegisterClassEx(&wcex);
}

//
// FUNCTION: SetupMagnifier
//
// PURPOSE: Creates the windows and initializes magnification.
//
BOOL SetupMagnifier(HINSTANCE hinst)
{
    // Create the host window.
    RegisterHostWindowClass(hinst);
    hwndHost = CreateWindowEx(WS_EX_TOPMOST | WS_EX_LAYERED, 
        WindowClassName, WindowTitle, 
        RESTOREDWINDOWSTYLES,
        0, 0, hostWindowRect.right, hostWindowRect.bottom, NULL, NULL, hInst, NULL);
    if (!hwndHost)
    {
        return FALSE;
    }

    // Make the window opaque.
    SetLayeredWindowAttributes(hwndHost, 0, 255, LWA_ALPHA);

    // Create a magnifier control that fills the client area.
    GetClientRect(hwndHost, &magWindowRect);
    hwndMag = CreateWindow(WC_MAGNIFIER, TEXT("MagnifierWindow"), 
        WS_CHILD | MS_SHOWMAGNIFIEDCURSOR | WS_VISIBLE,
        magWindowRect.left, magWindowRect.top, magWindowRect.right, magWindowRect.bottom, hwndHost, NULL, hInst, NULL );
    if (!hwndMag)
    {
        return FALSE;
    }

	// Pass the source rectangle to the magnifier control.
	BOOL ret = MagSetWindowSource(hwndMag, sourceRect);

    // Set the magnification factor.
    MAGTRANSFORM matrix;
    memset(&matrix, 0, sizeof(matrix));
    matrix.v[0][0] = magfactor;
    matrix.v[1][1] = magfactor;
    matrix.v[2][2] = 1.0f;

    ret = ret | MagSetWindowTransform(hwndMag, &matrix);

    return ret;  
}


//
// FUNCTION: UpdateMagWindow()
//
// PURPOSE: Updates the window. Called by a timer.
//
void CALLBACK UpdateMagWindow(HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR /*idEvent*/, DWORD /*dwTime*/)
{

	// Pass the source rectangle to the magnifier control.
	MagSetWindowSource(hwndMag, sourceRect);

    // Force redraw.
    InvalidateRect(hwndMag, NULL, TRUE);
}