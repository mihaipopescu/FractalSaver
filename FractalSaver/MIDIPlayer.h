#pragma once

#include "ScreenSaverConfig.h"
#include <windows.h>

/*!
 * \class CMIDIPlayer is responsible of initializing and closing the midi system
 * and playing the midi sounds.
 */
class CMIDIPlayer
{
public:
    CMIDIPlayer(void);
    ~CMIDIPlayer(void);

    bool Init(const CScreenSaverConfig & _config);

    static const char * get_instrument_name(int index);
    static size_t get_instrument_count();


protected:
    void DisplayMidiOutErrorMsg(unsigned long err);

    HMIDIOUT myHandle;

    union midi_data
    {
        DWORD	dwData;
        UCHAR	bData[4];
    };

};

