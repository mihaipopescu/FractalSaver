#pragma once
#include <string>

#define SI_SUPPORT_IOSTREAMS
#include "../simpleini/SimpleIni.h"


#define TEMPO_MIN 130
#define TEMPO_DEF 250
#define TEMPO_MAX 400

#define SPEED_MIN  1					// minimum redraw speed value
#define SPEED_MAX  20					// maximum redraw speed value
#define SPEED_DEF  10					// default redraw speed value


/*!
 * \class CScreenSaverConfig is used to save and load screen saver configuration.
 * The class uses SimpleIni \see http://code.jellycan.com/simpleini/
 */
class CScreenSaverConfig
{
public:
    CScreenSaverConfig(void);
    ~CScreenSaverConfig(void);

    bool Load(); //! Loads the configuration from the config ini file
    bool Save(); //! Saves the configuration to the config ini file

    std::string get_formated_constant() const;

public:
    unsigned myMusicTempo;
    unsigned myRefreshTime;
    unsigned char myInstrument;
    unsigned myColoringAlgorithm;
    unsigned myPaletteIndex;

    float myPX, myPY;
    float myLeft, myRight;
    float myTop, myBottom;
    
    bool myCycleRight;
    bool myMute;

    unsigned char myCycleOffset;

    std::string myPaletteFile;

    CSimpleIni myConfigFile;
};

