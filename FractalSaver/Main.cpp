#include <windows.h>
#include <scrnsave.h>
#include "FractalSaver.h"

#define TIMER 1

////////////////////////////////////////////////////////////////////////////////
// ScreenSaver Infrastructure

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LONG lParam) 
{
	switch(message) 
	{ 
	case WM_INITDIALOG:
		// Retrieve the application name from the .rc file.  
		LoadString(hMainInstance, idsAppName, szAppName, 80 * sizeof(TCHAR)); 

		// Retrieve the .ini (or registry) file name. 
		LoadString(hMainInstance, idsIniFile, szIniFile, MAXFILELEN * sizeof(TCHAR)); 
		break;

	case WM_COMMAND: 
		break;

	case WM_CLOSE:
		EndDialog(hDlg, FALSE);
		return TRUE;

	case WM_DESTROY:
		break;
	}

	return FALSE; 
} 

LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT message, WPARAM wParam, LONG lParam) 
{
	static HDC hDC;
	static bool bFirstRun = true;

	switch(message) 
	{ 
	case WM_CREATE: 
		// Retrieve the application name from the .rc file. 
		LoadStringA(hMainInstance, idsAppName, (LPSTR)szAppName, 40 * sizeof(TCHAR)); 

		// Retrieve the .ini (or registry) file name. 
		LoadStringA(hMainInstance, idsIniFile, (LPSTR)szIniFile, MAXFILELEN * sizeof(TCHAR));

		// Set the timer to 16ms (60fps)
		SetTimer(hWnd, TIMER, 16, NULL);

		CFractalSaver::CreateInstance();
		break;

	case WM_TIMER:
		if( bFirstRun )
		{
			// init device and objects
			if( !CFractalSaver::GetInstance()->InitDirect3D(hWnd) || !CFractalSaver::GetInstance()->CreateRenderObjects() )
			{
				PostQuitMessage(1);
			}

			bFirstRun = false;
			break;
		}
	
		CFractalSaver::GetInstance()->FrameAdvance();
		break;

	case WM_ERASEBKGND:
		// The WM_ERASEBKGND message is issued before the 
		// WM_TIMER message, allowing the screen saver to 
		// paint the background as appropriate. 

		break;

	case WM_DESTROY:  
		// When the WM_DESTROY message is issued, the screen saver 
		// must destroy any of the timers that were set at WM_CREATE 
		// time and release additional memory allocated at startup. 
		KillTimer(hWnd, TIMER);

		CFractalSaver::DestroyInstance();
		break;
	} 

	// DefScreenSaverProc processes any messages ignored by ScreenSaverProc. 
	return DefScreenSaverProc(hWnd, message, wParam, lParam); 
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst) 
{	
	return TRUE; 
}