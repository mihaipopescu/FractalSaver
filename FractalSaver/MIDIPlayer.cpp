#include "MIDIPlayer.h"
#include <windows.h>

const char *g_instruments[] =
{
    "Acoustic grand piano","Bright acoustic piano","Electric grand piano","Honky-tonk-piano","Rhodes piano","Chorused piano","Harpsichord","Clavi",
    "Celesta","Glockenspiel","Music box","Vibraphone","Marimba","Xylophone","Tubular bells","Dulcimer",
    "Hammond organ","Percussive organ","Rock organ","Church organ","Reed organ","Accordion","Harmonica","Tango Accordion",
    "Nylon guitar","Steel guitar","Jazz guitar","Clean guitar","Muted guitar","Overdriven guitar","Distortion guitar","Guitar harmonics",
    "Acoustic bass","Electric finger bass","Electric pick bass","Fretless bass","Slap bass 1","Slap bass 2","Synth bass 1","Synth bass 2",
    "Violin","Viola","Cello","Contrabass","Tremolo strings","Pizzicato strings","Orchestral harp","Timpani",
    "String ensemble 1","String ensemble 2","Synth strings 1","Synth strings 2","Choir Aahs","Voice Oohs","Synth voice","Orchestra hit",
    "Trumpet","Trombone","Tuba","Muted trumpet","French horn","Brass section","Synth brass 1","Synth brass 2",
    "Soprano sax","Alto sax","Tenor sax","Baritone sax","Oboe","English horn","Bassoon","Clarinet",
    "Piccolo","Flute","Recorder","Pan flute","Bottle blow","Shakuhachi","Whistle","Ocarina",
    "Lead 1 (square)","Lead 2 (sawtooth)","Lead 3 (calliope)","Lead 4 (chiff)","Lead 5 (charang)","Lead 6 (voice)","Lead 7 (fifths)","Lead 8 (brass)",
    "Pad 1 (new age)","Pad 2 (warm)","Pad 3 (polysynth)","Pad 4 (choir)","Pad 5 (bowed)","Pad 6 (metallic)","Pad 7 (halo)","Pad 8 (sweep)",
    "Guitar fret noise","Breath noise","Seashore","Bird tweet","Telephone ring","Helicopter","Applause","Gunshot"
};

CMIDIPlayer::CMIDIPlayer(void)
    : myHandle(0)
{
}


CMIDIPlayer::~CMIDIPlayer(void)
{
}

bool CMIDIPlayer::Init(const CScreenSaverConfig & _config)
{
    unsigned long err = 0;

    // Init MIDI System
    if ((err = midiOutOpen(&myHandle, (UINT)-1, 0, 0, CALLBACK_NULL)))
    {
        DisplayMidiOutErrorMsg(err);
        return false;
    }

    midi_data u;
    u.dwData  = 0;
    u.bData[1] = _config.myInstrument;

    // Select instrument for all channels
    for(int i = 0; i<16; i++)
    {
        u.bData[0]=0xC0 + i;
        if((err = midiOutShortMsg((HMIDIOUT)myHandle, u.dwData)))
        {
            DisplayMidiOutErrorMsg(err);
            return false;
        }
    }

    return true;
}

// Error Function for MIDI System
void CMIDIPlayer::DisplayMidiOutErrorMsg(unsigned long err)
{
    char buffer[120];

    if (!(err = midiOutGetErrorText(err, (LPSTR)&buffer[0], 120)))
    {
        MessageBox(NULL, buffer, "MIDI ERROR", MB_OK + MB_ICONERROR);
    }
    else if (err == MMSYSERR_BADERRNUM)
    {
        MessageBox(NULL, "Strange error number returned!", "MIDI ERROR", MB_OK + MB_ICONERROR);
    }
    else
    {
        MessageBox(NULL, "Specified pointer is invalid!", "MIDI ERROR", MB_OK + MB_ICONERROR);
    }
}

const char * CMIDIPlayer::get_instrument_name( int index )
{
    return g_instruments[index];
}

size_t CMIDIPlayer::get_instrument_count()
{
    return sizeof(g_instruments)/sizeof(char*);
}