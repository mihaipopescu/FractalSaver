#include <windows.h>
#include <scrnsave.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <fstream>
#include <CommCtrl.h>
#include "resource.h"
#include "ScreenSaverConfig.h"
#include "MIDIPlayer.h"


// Define Windows timers
#define TIMER_RENDER 1
#define TIMER_TEMPO 2

extern const char ** g_instruments;

const char* g_predefc[] = 
{
    "0.1+0.7i",	"-0.74543-0.11301i", "-0.12+0.74i",
    "-0.15652-1.03225i", "-0.194+0.6557i",
    "0.3325+0.5753176i",
    "-1.756+0.000i","-1.1416+0.2434i",
    "0.2617+0.0014i","0.3760+0.1481i",
    "0.40566+0.33679i",
    "-0.3737+0.6597i",
    "-0.581+0.6i", "-0.687+0.312i", 
    "-0.7543+0.113i","0.11+0.66i", 
    "-0.11+0.6557i","0.27334+0.00742i",
    "-0.391-0.587i",
    "0.0+0.0i","-0.80350195-0.17592593i"
};

// Escape Time;Escape Angle;Distance Estimation;Curvature Estimation;Statistic;Gaussian Integers;Orbit Traps;Finite Attractors;
const char *g_colalg[] = {"Escape Time","Escape Angle","Distance Estimation","Curvature Estimation","Statistic","Gaussian Integers","Finite Attractors"};

//These forward declarations are just for readability,
//so the big three functions can come first 
//
// http://www.cityintherain.com/greensquare.html
//

void InitGL(HWND hWnd, HDC & hDC, HGLRC & hRC);
void CloseGL(HWND hWnd, HDC hDC, HGLRC hRC);
void SetupAnimation(int Width, int Height);
void CleanupAnimation();
void OnTimer(HDC hDC);
bool InitPalettePickerData(const char *fn, HWND hw);
void DestroyPalettePickerData(HWND hw);


// globals
int Width, Height;
CScreenSaverConfig g_Config;
CMIDIPlayer g_MIDIPlayer;
RGBQUAD *g_selected_palette = NULL;



//////////////////////////////////////////////////
////   INFRASTRUCTURE -- THE THREE FUNCTIONS   ///
//////////////////////////////////////////////////

// Screen Saver Procedure
LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HDC hDC;
    static HGLRC hRC;
    static RECT rect;
    static UINT uTimer;

    switch ( message ) 
    {
    case WM_CREATE: 
        // get window dimensions
        GetClientRect( hWnd, &rect );
        Width = rect.right;         
        Height = rect.bottom;

        g_Config.Load();

        // setup OpenGL, then animation
        InitGL( hWnd, hDC, hRC );
        //SetupAnimation(Width, Height);

        // Set a timer for the screen saver window using the 
        // redraw rate stored in Regedit.ini.
        uTimer = SetTimer(hWnd, TIMER_RENDER, (SPEED_MAX - SPEED_MIN + g_Config.myRefreshTime)*10, NULL);
        break;

    case WM_DESTROY:
        KillTimer( hWnd, TIMER_RENDER );
        //CleanupAnimation();
        CloseGL( hWnd, hDC, hRC );
        break;

    case WM_TIMER:
        OnTimer(hDC); //animate !
        break;
    }

    return DefScreenSaverProc( hWnd, message, wParam, lParam );
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch ( message ) 
    {
    case WM_INITDIALOG:
        {
            // Resize screen saver dialog to 1024x600
            //MoveWindow(hDlg, 0, 0, 1024, 612 + GetSystemMetrics(SM_CYCAPTION), TRUE);

            // Load saved configuration
            g_Config.Load();

            // Init GUI
            SendDlgItemMessage(hDlg, IDC_CYCLEFT, BM_SETCHECK, !g_Config.myCycleRight ? BST_CHECKED : BST_UNCHECKED, 0);
            SendDlgItemMessage(hDlg, IDC_CYCRIGHT, BM_SETCHECK, g_Config.myCycleRight ? BST_UNCHECKED : BST_CHECKED, 0);

            if( !g_MIDIPlayer.Init(g_Config) )
            {
                EnableWindow(GetDlgItem(hDlg, IDC_MUTE), FALSE);
                g_Config.myMute = true; // force mute as we don't have a midi playback device
            }

            if( g_Config.myMute )
            {
                SendDlgItemMessage(hDlg, IDC_MUTE, BM_SETCHECK, BST_CHECKED, 0);
                EnableWindow(GetDlgItem(hDlg, IDC_PLAYMUS), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_INSTRUMENT), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_TEMPO), FALSE);
            }
            
            // Initialize scrollbar controls
            HWND hw = GetDlgItem(hDlg, ID_SPEED); 
            SetScrollRange(hw, SB_CTL, SPEED_MIN, SPEED_MAX, FALSE); 
            SetScrollPos(hw, SB_CTL, g_Config.myRefreshTime, TRUE);

            hw = GetDlgItem(hDlg, IDC_TEMPO);
            SetScrollRange(hw, SB_CTL, TEMPO_MIN, TEMPO_MAX, FALSE); 
            SetScrollPos(hw, SB_CTL, g_Config.myMusicTempo, TRUE);

            // Init predefined constants combo box
            hw = GetDlgItem(hDlg, IDC_PRECONST);

            for(int i = 0; i < sizeof(g_predefc) / sizeof(char*); i++)
                SendMessage(hw, CB_ADDSTRING, 0, (LPARAM)g_predefc[i]);

            // Default constant
            SendMessage(hw, WM_SETTEXT, 0, (LPARAM)g_Config.get_formated_constant().c_str());			

            // Init coloring algorithm combo box
            hw = GetDlgItem(hDlg, IDC_COLALG);

            for(int i = 0; i < sizeof(g_colalg) / sizeof(char*); i++)
                SendMessage(hw, CB_ADDSTRING, 0, (LPARAM)g_colalg[i]);

            // Select algorithm 
            SendMessage(hw, CB_SETCURSEL, g_Config.myColoringAlgorithm, 0);

            // Instrument combo init
            hw = GetDlgItem(hDlg, IDC_INSTRUMENT);

            for(size_t i = 0; i < g_MIDIPlayer.get_instrument_count(); i++)
                SendMessage(hw, CB_ADDSTRING, 0, (LPARAM)g_MIDIPlayer.get_instrument_name(i));

            // Select default instrument
            SendMessage(hw, CB_SETCURSEL, g_Config.myInstrument, 0);

            // Get palette listbox item
            hw = GetDlgItem(hDlg, IDC_PALST);

            // TODO destroy user data !!!
            if( !InitPalettePickerData(g_Config.myPaletteFile.c_str(), hw) )
            {
                MessageBox(hDlg, "Palette data not found!", "FractalSaver", MB_OK | MB_ICONERROR);
                PostQuitMessage(1);
                return FALSE;
            }

            // Selects the palette
            SendDlgItemMessage(hDlg, IDC_PALST, LB_SETCURSEL, g_Config.myPaletteIndex, 0);
            g_selected_palette = (RGBQUAD*)SendDlgItemMessage(hDlg, IDC_PALST, LB_GETITEMDATA, g_Config.myPaletteIndex, NULL);

            hw = GetDlgItem(hDlg, IDC_PAINTPROGRESS);
            SendMessage(hw, PBM_SETSTEP, (WPARAM) 1, 0);
            SendMessage(hw, PBM_SETPOS, 50, 0);
        }
        return TRUE;

    case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

            if( lpdis->itemID<0 )
                break;

            // Get item rectangle
            RECT rcItem = lpdis->rcItem;

            // Gets hdc for the item
            HDC hdc = lpdis->hDC;

            // Save these values to restore them when done drawing.
            COLORREF OldBKColor = GetBkColor(hdc);
            COLORREF OldTextColor = GetTextColor(hdc);

            // If this item is selected, set the background color
            // and the text color to appropriate values. Also, erase
            // rect by filling it with the background color.
            if( (lpdis->itemAction | ODA_SELECT) && (lpdis->itemState & ODS_SELECTED) )
            {
                SetTextColor(hdc, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
                SetBkColor(hdc, ::GetSysColor(COLOR_HIGHLIGHT));
                FillRect(hdc, &lpdis->rcItem, CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT)));
            }
            else
            {
                FillRect(hdc, &lpdis->rcItem, CreateSolidBrush(OldBKColor));
            }

            // If this item has focus draw a frame
            if( (lpdis->itemAction | ODA_FOCUS) && (lpdis->itemState & ODS_FOCUS) )
            {
                DrawFocusRect(hdc, &lpdis->rcItem);
            }

            // Deflate palette rect
            // We have to draw 256 lines but we don't have enough space
            // so we choose to draw just half, 128px
            int x, y, w, h;
            w = rcItem.right - rcItem.left - 4;
            if( w > 256 ) w = 256;
            h = rcItem.bottom - rcItem.top - 4;
            x = rcItem.right - w - 2;
            y = rcItem.top + 2;

            // Move text rect just a little bit and make it 32px wide
            rcItem.left += 2;
            rcItem.right = rcItem.left + 32;

            // Get the attached palette from itemData
            RGBQUAD * pPalette = (RGBQUAD*)lpdis->itemData;

            // If a palette was attached to the item
            if( pPalette )
            {
                if( lpdis->itemState & ODS_DISABLED )
                    SetTextColor(hdc, ::GetSysColor(COLOR_INACTIVECAPTIONTEXT));

                // Prepare pen
                LOGPEN logpen;
                logpen.lopnStyle = PS_SOLID;
                logpen.lopnWidth.x = 1;

                // Draw the palette
                for(int j=0;j<w;j++)
                {
                    // Change pen color
                    RGBQUAD c = pPalette[(int)(j * 256.f/w)];
                    logpen.lopnColor = RGB(c.rgbRed, c.rgbGreen, c.rgbBlue);
                    HPEN pen = CreatePenIndirect(&logpen);
                    SelectObject(hdc, pen);
                    // Draw lines
                    MoveToEx(hdc, x+j, y, NULL);
                    LineTo(hdc, x+j, y+h);
                    DeleteObject(pen);
                }

                // Draw a frame around the palette
                //FrameRect(hdc, &rp, CreateSolidBrush(BLACK_BRUSH));			
            }

            //char buff[16];
            //_itoa_s(lpdis->itemID, buff, 16);
            //SetBkMode(hdc, TRANSPARENT);
            //DrawText(hdc, buff , strlen(buff), &rcItem, DT_SINGLELINE|DT_VCENTER);
            SetTextColor(hdc, OldTextColor);
            SetBkColor(hdc, OldBKColor);
        }
        break;

    case WM_COMMAND:
        switch( LOWORD( wParam ) ) 
        {
        case IDC_SAVESCR:
            g_Config.Save();
            EndDialog( hDlg, TRUE );
            return TRUE; 

        case IDC_DISCARD:
            EndDialog( hDlg, FALSE );
            return TRUE;
        }
        break;

    case WM_CLOSE:
        EndDialog( hDlg, FALSE );
        return TRUE;
        break;

    case WM_DESTROY:
        DestroyPalettePickerData(GetDlgItem(hDlg, IDC_PALST));
        return TRUE;
        break;
    }

    return FALSE;
}

// needed for SCRNSAVE.LIB
BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
    return TRUE;
}


/////////////////////////////////////////////////
////   INFRASTRUCTURE ENDS, SPECIFICS BEGIN   ///
/////////////////////////////////////////////////


// Initialize OpenGL
static void InitGL(HWND hWnd, HDC & hDC, HGLRC & hRC)
{
    PIXELFORMATDESCRIPTOR pfd;
    ZeroMemory( &pfd, sizeof pfd );
    pfd.nSize = sizeof pfd;
    pfd.nVersion = 1;
    //pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
    pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;

    hDC = GetDC( hWnd );

    SetPixelFormat( hDC, ChoosePixelFormat( hDC, &pfd ), &pfd );

    hRC = wglCreateContext( hDC );
    wglMakeCurrent( hDC, hRC );
}

// Shut down OpenGL
static void CloseGL(HWND hWnd, HDC hDC, HGLRC hRC)
{
    wglMakeCurrent( NULL, NULL );
    wglDeleteContext( hRC );
    ReleaseDC( hWnd, hDC );
}

bool InitPalettePickerData(const char *fn, HWND hw)
{
    std::ifstream input(fn, std::ios::in | std::ios::binary);

    if( input.fail() )
        return false;

    int n = 0;
    input.read((char*)&n, sizeof(int));

    for(int i=0;i<n;i++)
    {
        RGBQUAD * pal = new RGBQUAD[256];

        for(int j = 0; j<256; j++)
            input.read((char*)(pal+j), sizeof(RGBQUAD));

        SendMessage(hw, LB_INSERTSTRING, -1, NULL);
        SendMessage(hw, LB_SETITEMDATA, i, (LPARAM)pal);
    }

    g_selected_palette = (RGBQUAD*)SendMessage(hw, LB_GETITEMDATA, 0, NULL);

    input.close();
    return true;
}

void DestroyPalettePickerData(HWND hw)
{
    g_selected_palette = NULL;
    int n = (int)SendMessage(hw, LB_GETCOUNT, 0, NULL);
    for(int i=0;i<n;i++)
    {
        RGBQUAD * pal = (RGBQUAD*)SendMessage(hw, LB_GETITEMDATA, i, NULL);
        delete [] pal;
    }
}

void OnTimer(HDC hDC)
{

}