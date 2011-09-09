#include "ScreenSaverConfig.h"
#include <windows.h>
#include <scrnsave.h>
#include <fstream>
#include <sstream>
#include <iostream>


//INI file
const char	szRedrawSpeed[] = "Redraw Speed";		// long(lSpeed)
const char	szMusicTempo[]  = "Master Tempo";		// long(lTempo)
const char	szInstrument[]	= "Instrument";			// int(instrIndex);
const char	szMandelbrotC[] = "Mandelbrot Constant";// double(PX) double(PY)
const char	szJuliaWin[]	= "Julia Window";		// double(LEFT) double(RIGHT) double(TOP) double(BOTTOM)
const char	szPaletteIndex[]= "Palette Index";		// int(palIndex)
const char	szCyclingDir[]  = "Cycling Direction";	// int(left?0:1)
const char	szColAlgIndex[]	= "Coloring Algorithm";	// int(colAlgIndex)
const char  szMuteMusic[]   = "Mute";				// int(mute?1:0)
const char  szCycleOffset[] = "Cycle Offset";		// int(cycleOffset)

void inline atox(char *str, float &a, float &b);
char inline *xtoa(char *str, float a, float b);

CScreenSaverConfig::CScreenSaverConfig( void )
    : myMusicTempo(TEMPO_DEF)
    , myRefreshTime(SPEED_DEF)
    , myInstrument(0)
    , myColoringAlgorithm(0)
    , myPaletteIndex(0)
    , myPX(0.f)
    , myPY(1.f)
    , myLeft(-1.575f)
    , myTop(-1.1f)
    , myRight(1.575f)
    , myBottom(1.1f)
    , myCycleRight(true)
    , myCycleOffset(0)
    , myMute(true)
{

}

CScreenSaverConfig::~CScreenSaverConfig(void)
{
}

bool CScreenSaverConfig::Load()
{
    // Retrieve the application name from the .rc file.
    LoadString(hMainInstance, idsAppName, ::szAppName, APPNAMEBUFFERLEN * sizeof(TCHAR));

    // Retrieve the .ini (or registry) file name.
    LoadString(hMainInstance, idsIniFile, ::szIniFile, MAXFILELEN * sizeof(TCHAR));

    myConfigFile.SetUnicode(false);
    myConfigFile.SetMultiKey(false);
    myConfigFile.SetMultiLine(false);

    // load the file
    try 
    {
        std::ifstream instream;
        instream.open(szIniFile, std::ifstream::in);
        if (myConfigFile.LoadData(instream) < 0) throw false;
        instream.close();
    }
    catch (...)
    {
        return false;
    }

    char buff[MAX_PATH];
    GetWindowsDirectory(buff, MAX_PATH);
    myPaletteFile = buff + std::string("\\System32\\palettes.bin");

    myRefreshTime = myConfigFile.GetLongValue(szAppName, szRedrawSpeed, SPEED_DEF, NULL);
    myMusicTempo = myConfigFile.GetLongValue(szAppName, szMusicTempo, TEMPO_DEF, NULL);
    myInstrument = (unsigned char)myConfigFile.GetLongValue(szAppName, szInstrument, 0, NULL);
    myColoringAlgorithm = myConfigFile.GetLongValue(szAppName, szColAlgIndex, 0, NULL);
    myPaletteIndex = myConfigFile.GetLongValue(szAppName, szPaletteIndex, 0, NULL);

    const char * pszValue = myConfigFile.GetValue(szAppName, szMandelbrotC, "0.0 1.0", NULL);
    std::istringstream iss(pszValue);
    iss.precision(10);
    iss.setf(std::ios::fixed, std::ios::floatfield);
    iss >> myPX >> myPY;

    pszValue = myConfigFile.GetValue(szAppName, szJuliaWin, "-1.57500000 -1.10000000 +1.57500000 +1.10000000", NULL);
    iss.str(pszValue);
    iss >> myLeft >> myTop >> myRight >> myBottom;

    myCycleOffset = (unsigned char)myConfigFile.GetLongValue(szAppName, szCycleOffset, NULL);
    myCycleRight = myConfigFile.GetBoolValue(szAppName, szCyclingDir, true);
    myMute = myConfigFile.GetBoolValue(szAppName, szMuteMusic, true);
    
    return true;
}

bool CScreenSaverConfig::Save()
{
    myConfigFile.SetLongValue(szAppName, szRedrawSpeed, myRefreshTime, NULL);
    myConfigFile.SetLongValue(szAppName, szMusicTempo, myMusicTempo, NULL);
    myConfigFile.SetLongValue(szAppName, szInstrument, myInstrument, NULL);
    myConfigFile.SetLongValue(szAppName, szColAlgIndex, myColoringAlgorithm, NULL);
    myConfigFile.SetLongValue(szAppName, szPaletteIndex, myPaletteIndex, NULL);

    std::ostringstream ostr;
    ostr.precision(10);
    ostr.setf(std::ios::fixed, std::ios::floatfield);

    ostr << myPX << " " << myPY;
    myConfigFile.SetValue(szAppName, szMandelbrotC, ostr.str().c_str(), NULL);
   
    ostr.seekp(std::ios_base::beg);
    ostr << myLeft << " " << myTop << " " << myRight << " " << myBottom;

    myConfigFile.SetValue(szAppName, szJuliaWin, ostr.str().c_str(), NULL);

    myConfigFile.SetLongValue(szAppName, szCycleOffset, myCycleOffset, NULL);
    myConfigFile.SetBoolValue(szAppName, szCyclingDir, myCycleRight);
    myConfigFile.SetBoolValue(szAppName, szMuteMusic, myMute);

    // save the file
    try
    {
        std::ofstream outfile;
        outfile.open(szIniFile, std::ofstream::out);
        if (myConfigFile.Save(outfile, true) < 0)
            throw false;
        outfile.close();
    }
    catch (...) {
        return false;
    }
    return true;
}

std::string CScreenSaverConfig::get_formated_constant() const
{
    std::ostringstream ostr;
    ostr << myPX << myPY;
    return ostr.str();
}

