//min OS windows95: http://msdn2.microsoft.com/en-us/library/aa383745.aspx
#define _WIN32_WINNT 0x0401	

#include <windows.h>
#include <scrnsave.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include "resource.h"
#include "commctrl.h"

#ifdef _DEBUG
#include "crtdbg.h"
#endif

typedef unsigned char byte;

#define PI 3.141592653589793
#define ITER 500
#define SQR(x) ((x)*(x))


extern HINSTANCE hMainInstance;   // screen saver instance handle  


//INI file
const char	szRedrawSpeed[] = "Redraw Speed";		// long(lSpeed)
const char	szMusicTempo[]  = "Master Tempo";		// long(lTempo)
const char	szInstrument[]	= "Instrument";			// int(instrIndex);
const char	szMandelbrotC[] = "Mandebrot Constant";	// double(PX) double(PY)
const char	szJuliaWin[]	= "Julia Window";		// double(LEFT) double(RIGHT) double(TOP) double(BOTTOM)
const char	szPaletteIndex[]= "Palette Index";		// int(palIndex)
const char	szCyclingDir[]  = "Cycling Direction";	// int(left?0:1)
const char	szColAlgIndex[]	= "Coloring Algorithm";	// int(colAlgIndex)
const char  szMuteMusic[]   = "Mute";				// int(mute?1:0)
const char  szCycleOffset[] = "Cycle Offset";		// int(cycleOffset)

//***Global Variables

BITMAP bmp;
HBITMAP hBMP1, hBMP2, hOldBMP;
HDC mdc1, mdc2;
HWND hPrgBar = NULL;

BITMAPINFOHEADER bi1 = {sizeof(BITMAPINFOHEADER), 0, 0, 1, 32, BI_RGB, 0, 0, 0, 0, 0};
BITMAPINFOHEADER bi2 = {sizeof(BITMAPINFOHEADER), 0, 0, 1, 32, BI_RGB, 0, 0, 0, 0, 0};

RECT brect = {0, 0, 0, 0};	//big window client rect
RECT srect = {0, 0, 0, 0};  //small window
int jWidth = 0, jHeight = 0;
int mWidth = 0, mHeight = 0;

char buff[256];					//temporary array of characters 

bool antialiasing = false;

RGBQUAD *palette=NULL;	//palette color data
int palIndex = 0;		//palette index
int colAlgIndex = 0;	//coloring algorithm index
bool cycleLeft;			//palette cycling direction
int cycleOffset = 0;	//palette cycling offset
bool bigmb = false;

//Mandelbrot world window
const double leftM = -2.0,   rightM = 0.5,   topM = -1.0, bottomM = 1.0;
const double leftJ = -1.575, rightJ = 1.575, topJ = -1.1, bottomJ = 1.1;

//Julia world window
double LEFT = -1.575, RIGHT = 1.575, TOP = -1.1, BOTTOM = 1.1; 

RGBQUAD *sbits, *bits;				//color bits for bitmaps
byte *jColorMap;					//indexes for julia color map
byte *jAliasMap;					//indexes for anti-aliased julia color map

double PX = 0, PY = 0;				// Mandelbrot Point (Constant C)

//*ScreenSaver Vars
#define MINVEL  1					// minimum redraw speed value
#define MAXVEL  20					// maximum redraw speed value
#define DEFVEL  10					// default redraw speed value
long    lSpeed = DEFVEL;			// redraw speed variable
 

//Fracta Music Tempo Vars
#define MINTMP 130
#define DEFTMP 250
#define MAXTMP 400

long lTempo = DEFTMP;				//master tempo
bool mute = false;					//music mute
int instrIndex = 0;					//instrument index

union
{
	DWORD	dwData;
	UCHAR	bData[4];
} u;								//union used for data sent to MIDI

//***Constants

const char *lpcszPalFile = "\\System32\\palettes.bin";
char *szPaletteFile;

const char* predefc[] = 
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

//Escape Time;Escape Angle;Distance Estimation;Curvature Estimation;Statistic;Gaussian Integers;Orbit Traps;Finite Attractors;
const char *colalg[] = {"Escape Time","Escape Angle",
"Distance Estimation","Curvature Estimation", "Statistic", 
"Gaussian Integers", "Finite Attractors"};

//Acoustic grand piano;Bright acoustic piano;Electric grand piano;Honky-tonk-piano;Rhodes piano;Chorused piano;Harpsichord;Clivinet;
//Celesta;Glockenspiel;Music box;Vibraphone;Marimba;Xylophone;Tubular bells;Dulcimer;Hammond organ;Percussive organ;Rock organ;
//Church organ;Reed organ;Accordion;Harmonica;Tango Accordion;Nylon guitar;Steel guitar;Jazz guitar;Clean guitar;Muted guitar;
//Overdriven guitar;Distortion guitar;Guitar harmonics;Acoustic base;Elec finger base;Elec pick base;Fretless base;Slap base 1;
//Slap bass 2;Synth bass 1;Synth bass 2;Violin;Viola;Cello;Contrabass;Tremolo strings;Pizzicato strings;Orchestral harp;Timpani;
//String ensemble 1;String ensemble 2;Synth strings 1;Synth strings 2;Choir Aahs;Voice Oohs;Synth voice;Orchestra hit;Trumpet;
//Trombone;Tuba;Muted trumpet;French horn;Brass section;Synth brass 1;Synth brass 2;Soprano sax;Alto sax;Tenor sax;Baritone sax;
//Oboe;English horn;Bassoon;Clarinet;Piccolo;Flute;Recorder;Pan flute;Bottle blow;Shakuhachi;Whistle;Ocarina;Lead 1 (square);
//Lead 2 (sawtooth);Lead 3 (calliope);Lead 4 (chiff);Lead 5 (charang);Lead 6 (voice);Lead 7 (fifths);Lead 8 (brass);Pad 1 (new age);
//Pad 2 (warm);Pad 3 (polysynth);Pad 4 (choir);Pad 5 (bowed);Pad 6 (metallic);Pad 7 (halo);Pad 8 (sweep);Guitar fret noise;
//Breath noise;Seashore;Bird tweet;Telephone ring;Helicopter;Applause;Gunshot;

const char *instruments[] = {
	    "Acoustic grand piano",
      "Bright acoustic piano",
      "Electric grand piano",
      "Honky-tonk-piano",
      "Rhodes piano",
      "Chorused piano",
      "Harpsichord",
      "Clivinet",
      "Celesta",
      "Glockenspiel",
      "Music box",
      "Vibraphone",
      "Marimba",
      "Xylophone",
      "Tubular bells",
      "Dulcimer",
      "Hammond organ",
      "Percussive organ",
      "Rock organ",
      "Church organ",
      "Reed organ",
      "Accordion",
      "Harmonica",
      "Tango Accordion",
      "Nylon guitar",
      "Steel guitar",
      "Jazz guitar",
      "Clean guitar",
      "Muted guitar",
      "Overdriven guitar",
      "Distortion guitar",
      "Guitar harmonics",
      "Acoustic base",
      "Elec finger base",
      "Elec pick base",
      "Fretless base",
      "Slap base 1",
      "Slap bass 2",
      "Synth bass 1",
      "Synth bass 2",
      "Violin",
      "Viola",
      "Cello",
      "Contrabass",
      "Tremolo strings",
      "Pizzicato strings",
      "Orchestral harp",
      "Timpani",
      "String ensemble 1",
      "String ensemble 2",
      "Synth strings 1",
      "Synth strings 2",
      "Choir Aahs",
      "Voice Oohs",
      "Synth voice",
      "Orchestra hit",
      "Trumpet",
      "Trombone",
      "Tuba",
      "Muted trumpet",
      "French horn",
      "Brass section",
      "Synth brass 1",
      "Synth brass 2",
      "Soprano sax",
      "Alto sax",
      "Tenor sax",
      "Baritone sax",
      "Oboe",
      "English horn",
      "Bassoon",
      "Clarinet",
      "Piccolo",
      "Flute",
      "Recorder",
      "Pan flute",
      "Bottle blow",
      "Shakuhachi",
      "Whistle",
      "Ocarina",
      "Lead 1 (square)",
      "Lead 2 (sawtooth)",
      "Lead 3 (calliope)",
      "Lead 4 (chiff)",
      "Lead 5 (charang)",
      "Lead 6 (voice)",
      "Lead 7 (fifths)",
      "Lead 8 (brass)",
      "Pad 1 (new age)",
      "Pad 2 (warm)",
      "Pad 3 (polysynth)",
      "Pad 4 (choir)",
      "Pad 5 (bowed)",
      "Pad 6 (metallic)",
      "Pad 7 (halo)",
      "Pad 8 (sweep)",
	  "Guitar fret noise",
      "Breath noise",
      "Seashore",
      "Bird tweet",
      "Telephone ring",
      "Helicopter",
      "Applause",
      "Gunshot"
};

//*** Functions

//Paint Small Mandelbrot
void PaintMandelbrot(int w, int h, double dx, double dy, double xx, double yy, RGBQUAD* c)
{
	int x, y, i; 
	double x2, y2, px, py, zx, zy;

	py = yy;
	for(y = 0; y < h; ++y)
	{
		px = xx;
		for(x = 0; x < w; ++x) 
		{
			zx = px, zy = py;
			for(i = 0; i < ITER; ++i) 
			{
				x2 = zx * zx;
				y2 = zy * zy;
				if(x2 + y2 > 4.0) //escape condition: |z|>2 => |z|^2 > 4
					break;
				// z[n+1] = z[n]^2 + c				
				zy = 2 * zx * zy   + py;
				zx = x2 - y2       + px;
			}
			c->rgbReserved=0;
			c->rgbRed   = 255-i*3;
			c->rgbGreen = 255-i*3;
			c->rgbBlue  = 255-i*3;
			c++;
			px += dx;					//incrementing with x scale factor
		}
		py += dy;						//incrementing with y scale factor		
	}
}

//Paint Small Julia
void PaintJulia(int w, int h, double dx, double dy, double xx, double yy, RGBQUAD* c)
{
	int x, y, i; 
	double x2, y2, zx2, zy2, zx, zy;	

	zy2 = yy;
	for(y = 0; y < h; ++y) 
	{
		zx2 = xx;
		for(x = 0; x < w; ++x) 
		{
			zx = zx2, zy = zy2;
			for(i = 0; i < ITER; ++i) 
			{
				x2 = zx * zx;
				y2 = zy * zy;
				if(x2 + y2 > 4.0)
					break;
				//z[n+1] = z[n]^2 + C
				zy = 2 * zx * zy  + PY;
				zx = x2 - y2      + PX;
			}
			c->rgbReserved=0;
			c->rgbRed   = 255-i*3;
			c->rgbGreen = 255-i*3;
			c->rgbBlue  = 255-i*3;
			c++;
			zx2 += dx;
		}		
		zy2 += dy;
	}
}

// Viewport to Windows Coordinates Trasnformation
inline void V2W(double& X, double& Y, int x, int y, int width, int height) 
{
	X = x * (RIGHT - LEFT) / double(width) + LEFT;
	Y = y * (TOP - BOTTOM) / double(height) + BOTTOM;
}

//Julia Orbit plot
void PlotJuliaOrbit(HDC hdc, int px, int py)
{
	double x2, y2, zx, zy;

	SelectObject(hdc, GetStockObject(WHITE_PEN));
	HBRUSH brush = CreateSolidBrush(RGB(0xFF, 0xA5, 0x00));
	HBRUSH oldbrush = (HBRUSH) SelectObject(hdc, brush);

	V2W(x2, y2, px-brect.left, py-brect.top, jWidth, jHeight);

	SelectClipRgn(hdc, CreateRectRgn(brect.left, brect.top, brect.right, brect.bottom));
	
	Ellipse(hdc, px-2, py-2, px+2, py+2);

	MoveToEx(hdc, px, py, NULL);
	
	zx = x2, zy = y2;
	for(int i = 0; i < ITER; ++i) 
	{
		x2 = zx * zx;
		y2 = zy * zy;
		if(x2 + y2 > 4.0)
			break;
		//z[n+1] = z[n]^2 + C
		zy = 2 * zx * zy  + PY;
		zx = x2 - y2      + PX;
				
		px = (zx - LEFT) * double(jWidth) / (RIGHT - LEFT) + brect.left; 
		py = (zy - BOTTOM) * double(jHeight) / (TOP - BOTTOM) + brect.top;

		LineTo(hdc, px, py);
	}

	// Destroying objects
	SelectObject(hdc, oldbrush);
	DeleteObject(brush);			
}

// Generate Julia Color Map for Screen Saver Palette Animation
void (*GenerateJuliaColorMap)(int w, int h, double dx, double dy, double xx, double yy, byte *c);

void GenerateJuliaColorMap_escape_time(int w, int h, double dx, double dy, double xx, double yy, byte *c)
{
	int x, y, i; 
	double x2, y2, zx2, zy2, zx, zy;	

	zy2 = yy;
	for(y = 0; y < h; ++y) 
	{
		zx2 = xx;
		for(x = 0; x < w; ++x) 
		{
			zx = zx2, zy = zy2;
			for(i = 0; i < ITER; ++i) 
			{
				x2 = zx * zx;
				y2 = zy * zy;
				if(x2 + y2 > 4.0)
					break;
				//z[n+1] = z[n]^2 + C
				zy = 2 * zx * zy  + PY;
				zx = x2 - y2      + PX;
			}
			*c = i*3;

			*c++ = (*c + cycleOffset)%256;

			zx2 += dx;
		}		
		zy2 += dy;
		SendMessage(hPrgBar, PBM_STEPIT, 0, 0);
	}
}

void GenerateJuliaColorMap_distance_estimators(int w, int h, double dx, double dy, double xx, double yy, byte *c)
{
	int x, y, i; 
	double x2, y2, zx2, zy2, zx, zy;

	zy2 = yy;
	for(y = 0; y < h; ++y) 
	{
		zx2 = xx;
		for(x = 0; x < w; ++x) 
		{
			zx = zx2, zy = zy2;
			for(i = 0; i < ITER; ++i) 
			{
				x2 = zx * zx;
				y2 = zy * zy;
				if(x2 + y2 > 4.0)
					break;
				//z[n+1] = z[n]^2 + C
				zy = 2 * zx * zy  + PY;
				zx = x2 - y2      + PX;
			}			

			*c = ((int)(fabs(log(sqrt(zx*zx+zy*zy)))*i)) % 256;

			*c++ = (*c + cycleOffset)%256;

			zx2 += dx;
		}		
		zy2 += dy;
		SendMessage(hPrgBar, PBM_STEPIT, 0, 0);
	}
}

void GenerateJuliaColorMap_escape_angle(int w, int h, double dx, double dy, double xx, double yy, byte *c)
{
	int x, y, i; 
	double x2, y2, zx2, zy2, zx, zy,alfa;
	const double beta = 0.024543692606170259675489401431871; //2*PI/256;

	zy2 = yy;
	for(y = 0; y < h; ++y) 
	{
		zx2 = xx;
		for(x = 0; x < w; ++x) 
		{
			zx = zx2, zy = zy2;
			for(i = 0; i < ITER; ++i) 
			{
				x2 = zx * zx;
				y2 = zy * zy;
				if(x2 + y2 > 4.0)
					break;
				//z[n+1] = z[n]^2 + C
				zy = 2 * zx * zy  + PY;
				zx = x2 - y2      + PX;
			}

			if (fabs(zx)<1e-6)
				if(zy<0) 
					alfa=0;
				else 
					alfa=PI;
			else 
				alfa=atan(zy/zx)+PI/2;
			
			*c = (byte)(alfa/beta);

			*c++ = (*c + cycleOffset)%256;

			zx2 += dx;
		}		
		zy2 += dy;
		SendMessage(hPrgBar, PBM_STEPIT, 0, 0);
	}
}

void GenerateJuliaColorMap_curvature_estimation(int w, int h, double dx, double dy, double xx, double yy, byte *c)
{
	int x, y, i; 
	double x2, y2, zx2, zy2, zx, zy,val;
	double j1,j2,k1,k2,n,im,re;

	j1=xx, j2=-xx;
	k1=yy, k2=-yy;

	zy2 = yy;
	for(y = 0; y < h; ++y) 
	{
		zx2 = xx;
		for(x = 0; x < w; ++x) 
		{
			zx = zx2, zy = zy2;
			for(i = 0; i < ITER; ++i) 
			{
				x2 = zx * zx;
				y2 = zy * zy;
				if(x2 + y2 > 4.0)
					break;
				//z[n+1] = z[n]^2 + C
				zy = 2 * zx * zy  + PY;
				zx = x2 - y2      + PX;
			}

			n  = (SQR(j1-j2)+SQR(k1-k2));
			re = ((zx-j1)*(j1-j2)+(zy-k1)*(k1-k2))/n;
			im = ((zy-k1)*(j1-j2)-(zx-j1)*(k1-k2))/n;

			if(fabs(re)>1e-6)
				val = fabs(atan(im/re))*2/PI*255;
			else
				val = 0;

			j2 = j1;
			k2 = k1;
			j1 = zx;
			k1 = zy;

			*c = (byte)val;

			*c++ = (*c + cycleOffset)%256;
			
			zx2 += dx;
		}		
		zy2 += dy;
		SendMessage(hPrgBar, PBM_STEPIT, 0, 0);
	}
}

void GenerateJuliaColorMap_statistic(int w, int h, double dx, double dy, double xx, double yy, byte *c)
{
	int x, y, i; 
	double x2, y2, zx2, zy2, zx, zy;
	double s, t, g;
	double d[ITER];

	zy2 = yy;
	for(y = 0; y < h; ++y) 
	{
		zx2 = xx;
		for(x = 0; x < w; ++x) 
		{
			s=0;
			zx = zx2, zy = zy2;
			for(i = 0; i < ITER; ++i) 
			{
				x2 = zx * zx;
				y2 = zy * zy;
				if(x2 + y2 > 4.0)
					break;

				t = sqrt(x2 + y2);
				s += t;
				d[i]= t;

				//z[n+1] = z[n]^2 + C
				zy = 2 * zx * zy  + PY;
				zx = x2 - y2      + PX;
			}
			
			//compute standard deviation
			if(i!=0)
				s/=i;

			g=0;
			for(int k=0;k<i;k++)
				g+=SQR(d[k]-s);

			if(i!=0)
				*c = byte(sqrt(g/i)*255);
			else
				*c = 0;

			*c++ = (*c + cycleOffset)%256;
			
			zx2 += dx;
		}		
		zy2 += dy;
		SendMessage(hPrgBar, PBM_STEPIT, 0, 0);
	}
}

void GenerateJuliaColorMap_gauss(int w, int h, double dx, double dy, double xx, double yy, byte *c)
{
	int x, y, i; 
	double x2, y2, zx2, zy2, zx, zy;
	int s, d;

	zy2 = yy;
	for(y = 0; y < h; ++y) 
	{
		zx2 = xx;
		for(x = 0; x < w; ++x) 
		{
			s = 255; d = 255;
			zx = zx2, zy = zy2;			
			for(i = 0; i < ITER; ++i) 
			{
				x2 = zx * zx;
				y2 = zy * zy;
				if(x2 + y2 > 4.0)
					break;

				d = int(sqrt(x2 + y2)*128);
				if(d<s) s=d;

				//z[n+1] = z[n]^2 + C
				zy = 2 * zx * zy  + PY;
				zx = x2 - y2      + PX;
			}

			*c++ = (s + cycleOffset)%256;
			
			zx2 += dx;
		}		
		zy2 += dy;
		SendMessage(hPrgBar, PBM_STEPIT, 0, 0);
	}
}

void GenerateJuliaColorMap_attractors(int w, int h, double dx, double dy, double xx, double yy, byte *c)
{
	int x, y, i, k; 
	double x2, y2, zx2, zy2, zx, zy;
	double mx[ITER],my[ITER], d, m;
	
	zy2 = yy;
	for(y = 0; y < h; ++y) 
	{
		zx2 = xx;
		for(x = 0; x < w; ++x) 
		{
			zx = zx2, zy = zy2;

			for(i = 0; i < ITER; ++i) 
			{
				x2 = zx * zx;
				y2 = zy * zy;

				if(x2 + y2 > 4)
					break;
				
				mx[i] = zx, my[i] = zy;

				//z[n+1] = z[n]^2 + C
				zy = 2 * zx * zy  + PY;
				zx = x2 - y2      + PX;				
			}

			m = 1000;
			for(k=0;k<i-1;k++)
			{
				d=sqrt(SQR(mx[k] - mx[i-1]) + SQR(my[k] - my[i-1]));
				if(d <  m) m = d;
				if(d < .1)
					break;
			}
			
			if(i>0 && k!=i-1)
				*c = byte(d*2560);	//attractor minimum distance
			else
				*c = k*3;			//escape time for divergent sets

			*c++ = (*c + cycleOffset)%256;
			
			zx2 += dx;
		}		
		zy2 += dy;
		SendMessage(hPrgBar, PBM_STEPIT, 0, 0);
	}
}

void GenerateMandelbrotColorMap_escape_time(int w, int h, double dx, double dy, double xx, double yy, byte *c)
{
	int x, y, i; 
	double x2, y2, px, py, zx, zy;

	py = yy;
	for(y = 0; y < h; ++y)
	{
		px = xx;
		for(x = 0; x < w; ++x) 
		{
			zx = px, zy = py;
			for(i = 0; i < ITER; ++i) 
			{
				x2 = zx * zx;
				y2 = zy * zy;
				if(x2 + y2 > 4.0) //escape condition: |z|>2 => |z|^2 > 4
					break;
				// z[n+1] = z[n]^2 + c				
				zy = 2 * zx * zy   + py;
				zx = x2 - y2       + px;
			}
			*c = i*3;

			*c++ = (*c + cycleOffset)%256;

			px += dx;					//incrementing with x scale factor
		}
		py += dy;						//incrementing with y scale factor
		SendMessage(hPrgBar, PBM_STEPIT, 0, 0);
	}
}

//end of coloring algorithms

void AllocateColorMap(bool saver)
{
	jColorMap = (byte*)malloc(jWidth*jHeight*sizeof(byte));
	if(!saver) jAliasMap = (byte*)malloc(4*jWidth*jHeight*sizeof(byte));
}

void ShiftLeft_PaintJulia()
{
	++cycleOffset;
	for(int i=0;i<jHeight; ++i)
		for(int j=0; j<jWidth; ++j)
			bits[i*jWidth+j] = palette[++jColorMap[i*jWidth+j]];

	SetDIBits(mdc1, hBMP1, 0, jHeight, bits, 
			(BITMAPINFO*)&bi1, DIB_RGB_COLORS);
}

void ShiftRight_PaintJulia()
{
	--cycleOffset;
	for(int i=0;i<jHeight; ++i)
		for(int j=0; j<jWidth; ++j)			
			bits[i*jWidth+j] = palette[--jColorMap[i*jWidth+j]];

	SetDIBits(mdc1, hBMP1, 0, jHeight, bits, 
			(BITMAPINFO*)&bi1, DIB_RGB_COLORS);
}

void FillBitmapFromColorMap(RGBQUAD *dest)
{
	byte *cm = jColorMap;
	//generate the image from the color map
	for (int i = 0; i < jHeight; ++i)
		for(int j = 0; j < jWidth; ++j)
			*dest++=palette[*cm++];

	//paint
	SetDIBits(mdc1, hBMP1, 0, jHeight, bits, 
		(BITMAPINFO*)&bi1, DIB_RGB_COLORS);
}

// Main repaint function that paints julia and mandelbrot
void RePaint_Small()
{
	double dx = 1 / (double)mWidth;
	double dy = 1 / (double)mHeight;

	//Small Mandel
	if(!bigmb)
	{
		dx *= rightM - leftM;
		dy *= bottomM - topM;
		
		PaintMandelbrot(mWidth, mHeight, dx, dy, leftM, topM, sbits);
	}
	else //Small Julia
	{
		dx *= rightJ - leftJ;
		dy *= bottomJ - topJ;
		
		PaintJulia(mWidth, mHeight, dx, dy, leftJ, topJ, sbits);
	}

	//paint small pic
	SetDIBits(mdc2, hBMP2, 0, mHeight, sbits, (BITMAPINFO*)&bi2, DIB_RGB_COLORS);
}


void RePaint_Big()
{
	double dx = (RIGHT - LEFT) / (double)jWidth;
	double dy = (BOTTOM - TOP) / (double)jHeight;	
	RGBQUAD *dest = bits;

	if(hPrgBar)
		SendMessage(hPrgBar, PBM_SETPOS, 0, 0);

	if(bigmb)
	{
		GenerateMandelbrotColorMap_escape_time(jWidth, jHeight, dx, dy, LEFT, TOP, jColorMap);
		FillBitmapFromColorMap(dest);
	}
	else
	{
		
		//Julia
		if(!antialiasing)
		{
			SendMessage(hPrgBar, PBM_SETRANGE, 0, MAKELPARAM(0, jHeight));

			GenerateJuliaColorMap(jWidth, jHeight, dx, dy, LEFT, TOP, jColorMap);

			FillBitmapFromColorMap(dest);

			return;
		}
		else
		{
			SendMessage(hPrgBar, PBM_SETRANGE, 0, MAKELPARAM(0, jHeight*3));

			//3 times the normal size
			GenerateJuliaColorMap(jWidth, jHeight, dx, dy, LEFT, TOP, jColorMap);
			GenerateJuliaColorMap(2*jWidth, 2*jHeight, dx/2, dy/2, LEFT, TOP, jAliasMap);

			//Apply 2x multisample and generate the image
			//NOTE! The colors aren't from the palette range

			for(int i=0; i<jHeight; ++i)
				for(int j=0; j<jWidth; ++j)
				{
					dest->rgbReserved = 0;

					dest->rgbRed = (palette[jAliasMap[i*2*jWidth*2 + j*2]].rgbRed + 
						palette[jAliasMap[i*2*jWidth*2 + j*2 + 1]].rgbRed +
						palette[jAliasMap[(i*2+1)*jWidth*2 + j*2]].rgbRed + 
						palette[jAliasMap[(i*2+1)*jWidth*2 + j*2 + 1]].rgbRed) >> 2;

					dest->rgbGreen = (palette[jAliasMap[i*2*jWidth*2 + j*2]].rgbGreen + 
						palette[jAliasMap[i*2*jWidth*2 + j*2 + 1]].rgbGreen +
						palette[jAliasMap[(i*2+1)*jWidth*2 + j*2]].rgbGreen+ 
						palette[jAliasMap[(i*2+1)*jWidth*2 + j*2 + 1]].rgbGreen) >> 2;

					dest->rgbBlue = (palette[jAliasMap[i*2*jWidth*2 + j*2]].rgbBlue+ 
						palette[jAliasMap[i*2*jWidth*2 + j*2 + 1]].rgbBlue +
						palette[jAliasMap[(i*2+1)*jWidth*2 + j*2]].rgbBlue + 
						palette[jAliasMap[(i*2+1)*jWidth*2 + j*2 + 1]].rgbBlue) >> 2;
					dest++;
				}
		}
	}

	//paint big pic
	SetDIBits(mdc1, hBMP1, 0, jHeight, bits, 
		(BITMAPINFO*)&bi1, DIB_RGB_COLORS);
}

// String to complex number
void inline atox(char *str, double &a, double &b) 
{
	char *p=str;
	while(*p==' ') p++; //skip leading spaces	
	a=atof(p);			//real part
	p++;
	while(*p!='+'&&*p!='-'&&*p!=' ') p++;
	b=atof(p);			//imaginary part
}

// Complex number to string
char inline *xtoa(char *str, double a, double b)
{
	char *p=str; 
	int i;
	sprintf(str,"%+.8f", a);
	str += strlen(str);
	sprintf(str,"%+.8f", b);
	i= strlen(str);
	str[i]='i';
	str[i+1]=0;
	return p;
}

// Inits the PalettePicker ListBox Data
bool InitPalettePickerData(const char *fn, HWND hw)
{
	RGBQUAD *pal;	
	int n;

	std::ifstream input(fn, std::ios::in | std::ios::binary);

	if(input.fail()) return false;

	input.read((char*)&n, sizeof(int));

	for(int i=0;i<n;i++)
	{
		pal = new RGBQUAD[256];

		for(int j = 0; j<256; j++)
			input.read((char*)(pal+j), sizeof(RGBQUAD));

		itoa(i, buff, 16);

		SendMessage(hw, LB_ADDSTRING, i, (LPARAM)buff);
		SendMessage(hw, LB_SETITEMDATA, i, (LPARAM)pal);
	}

	palette = (RGBQUAD*)SendMessage(hw, LB_GETITEMDATA, 0, NULL);

	input.close();
	return true;
}

// Loads a specified palette from 'fn' file with index 'pn'
bool LoadPalette(const char *fn, int pn)
{	
	int n;
	palette = new RGBQUAD[256];


	std::ifstream input(fn, std::ios::in | std::ios::binary);

	if(input.fail()) return false;

	input.read((char*)&n, sizeof(int));

	for(int i=0;i<n;i++)
	{
		if(i!=pn) input.ignore(256*sizeof(RGBQUAD));
		else
		{
			for(int j = 0; j<256; j++)
				input.read((char*)(palette+j), sizeof(RGBQUAD));

			break;
		}
	}

	input.close();
	return true;
}


// Error Function for MIDI System
void PrintMidiOutErrorMsg(unsigned long err)
{
	#define BUFFERSIZE 120
	char	buffer[BUFFERSIZE];
	
	if (!(err = midiOutGetErrorTextA(err, (LPSTR)&buffer[0], BUFFERSIZE)))
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

// Save Data to Registry.
void SaveScreenSaverConfiguration()
{
	int i;

	ltoa(lSpeed, buff, 10);
	WritePrivateProfileStringA((LPCSTR)szAppName, (LPCSTR)szRedrawSpeed, 
		(LPCSTR)buff, (LPCSTR)szIniFile);

	ltoa(lTempo, buff, 10);
	WritePrivateProfileStringA((LPCSTR)szAppName, (LPCSTR)szMusicTempo, 
		(LPCSTR)buff, (LPCSTR)szIniFile);

	itoa(instrIndex, buff, 10);
	WritePrivateProfileStringA((LPCSTR)szAppName, (LPCSTR)szInstrument, 
		(LPCSTR)buff, (LPCSTR)szIniFile);

	xtoa(buff, PX, PY);
	WritePrivateProfileStringA((LPCSTR)szAppName, (LPCSTR)szMandelbrotC, 
		(LPCSTR)buff, (LPCSTR)szIniFile);

	xtoa(buff, LEFT, TOP);
	i=strlen(buff);
	buff[i]=',';
	xtoa(buff+i+1, RIGHT, BOTTOM);

	WritePrivateProfileStringA((LPCSTR)szAppName, (LPCSTR)szJuliaWin,
		(LPCSTR)buff, (LPCSTR)szIniFile);

	itoa(colAlgIndex, buff, 10);

	WritePrivateProfileStringA((LPCSTR)szAppName, (LPCSTR)szColAlgIndex,
		(LPCSTR)buff, (LPCSTR)szIniFile);

	itoa(palIndex, buff, 10);

	WritePrivateProfileStringA((LPCSTR)szAppName, (LPCSTR)szPaletteIndex,
		(LPCSTR)buff, (LPCSTR)szIniFile);
	
	if(cycleLeft)
		WritePrivateProfileStringA((LPCSTR)szAppName, (LPCSTR)szCyclingDir,
			"0", (LPCSTR)szIniFile);
	else
		WritePrivateProfileStringA((LPCSTR)szAppName, (LPCSTR)szCyclingDir,
			"1", (LPCSTR)szIniFile);

	if(mute)
		WritePrivateProfileStringA((LPCSTR)szAppName, (LPCSTR)szMuteMusic,
			"1", (LPCSTR)szIniFile);
	else
		WritePrivateProfileStringA((LPCSTR)szAppName, (LPCSTR)szMuteMusic,
			"0", (LPCSTR)szIniFile);

	while(cycleOffset<0)
		cycleOffset+=256;

	itoa(cycleOffset%256, buff, 10);

	WritePrivateProfileStringA((LPCSTR)szAppName, (LPCSTR)szCycleOffset,
		(LPCSTR)buff, (LPCSTR)szIniFile);

}

// Retrieve data from the registry. 
void LoadScreenSaverConfiguration()
{	
	//initiate path to palette file
	GetWindowsDirectoryA(buff, 256);
	szPaletteFile = (char*)malloc(strlen(buff)+strlen(lpcszPalFile)+1);
	strcpy(szPaletteFile, buff);
	strcat(szPaletteFile, lpcszPalFile);

	lSpeed = GetPrivateProfileIntA((LPCSTR)szAppName,(LPCSTR)szRedrawSpeed, 
		DEFVEL, (LPCSTR)szIniFile);

	lTempo = GetPrivateProfileIntA((LPCSTR)szAppName,(LPCSTR)szMusicTempo, 
		DEFTMP, (LPCSTR)szIniFile);

	instrIndex = GetPrivateProfileIntA((LPCSTR)szAppName,(LPCSTR)szInstrument, 
		0, (LPCSTR)szIniFile);

	colAlgIndex = GetPrivateProfileIntA((LPCSTR)szAppName,(LPCSTR)szColAlgIndex, 
		0, (LPCSTR)szIniFile);

	palIndex = GetPrivateProfileIntA((LPCSTR)szAppName,(LPCSTR)szPaletteIndex, 
		0, (LPCSTR)szIniFile);

	GetPrivateProfileString((LPCSTR)szAppName, (LPCSTR)szMandelbrotC, "0.0+1.0i", 
		(LPSTR)buff, 256, (LPCSTR)szIniFile);

	atox(buff, PX, PY);

	GetPrivateProfileString((LPCSTR)szAppName, (LPCSTR)szJuliaWin, "-1.57500000-1.10000000i,+1.57500000+1.10000000i", 
		(LPSTR)buff, 256, (LPCSTR)szIniFile);

	atox(buff, LEFT, TOP);
	atox(strchr(buff, ',')+1, RIGHT, BOTTOM);

	cycleLeft = GetPrivateProfileIntA((LPCSTR)szAppName,(LPCSTR)szCyclingDir, 
		0, (LPCSTR)szIniFile)==0;

	mute = GetPrivateProfileIntA((LPCSTR)szAppName,(LPCSTR)szMuteMusic, 
		1, (LPCSTR)szIniFile)==1;

	cycleOffset = GetPrivateProfileIntA((LPCSTR)szAppName,(LPCSTR)szCycleOffset, 
		0, (LPCSTR)szIniFile);

	switch(colAlgIndex)
	{
		case 0:
			GenerateJuliaColorMap = GenerateJuliaColorMap_escape_time;
			break;
		case 1:
			GenerateJuliaColorMap = GenerateJuliaColorMap_escape_angle;
			break;
		case 2:
			GenerateJuliaColorMap = GenerateJuliaColorMap_distance_estimators;
			break;
		case 3:
			GenerateJuliaColorMap = GenerateJuliaColorMap_curvature_estimation;
			break;
		case 4:
			GenerateJuliaColorMap = GenerateJuliaColorMap_statistic;
			break;
		case 5:
			GenerateJuliaColorMap = GenerateJuliaColorMap_gauss;
			break;
		case 6:
			GenerateJuliaColorMap = GenerateJuliaColorMap_attractors;
			break;

		default:
			GenerateJuliaColorMap = GenerateJuliaColorMap_escape_time;
	}
}

//***Music Generation
HMIDIOUT	handle = 0;

void GenerateJuliaSong(int w, int h, double dx, double dy, double xx, double yy, byte *c)
{
	int x, y, i; 
	double x2, y2, zx2, zy2, zx, zy;	

	zy2 = yy;
	for(y = 0; y < h; ++y) 
	{
		zx2 = xx;
		for(x = 0; x < w; ++x) 
		{
			zx = zx2, zy = zy2;
			for(i = 0; i < ITER; ++i) 
			{
				x2 = zx * zx;
				y2 = zy * zy;
				if(x2 + y2 > 4.0)
					break;
				//z[n+1] = z[n]^2 + C
				zy = 2 * zx * zy  + PY;
				zx = x2 - y2      + PX;
			}	
			*c++ = (i*0.8);
			zx2 += dx;
		}		
		zy2 += dy;
	}
}

#define Ws 39
#define Hs 32


const int NoteMap[32]={108, 104, 103, 99, 98, 96, 92, 91, 87, 86, 84, 79, 75, 74, 72, 68, 67, 63, 62, 60, 56, 55, 51, 50, 48, 44, 43, 39, 38, 36, 80 };
const int chn[32] = {0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,10,10,11,11,12,12,13,13,14,14,15,15,15};

void PlayJuliaSmall()
{
	byte data[Ws*Hs], i, j, k, p, pp;

	double	dx = (RIGHT - LEFT) / (double)Ws,
			dy = (BOTTOM - TOP) / (double)Hs;

	unsigned long	err;

	GenerateJuliaSong(Ws, Hs, dx, dy, LEFT, TOP, data);

	for(i=0; i<Hs/32; i++)
		for(j=0; j<Ws; j++) 
		{
			u.bData[3] = 0;

			for(k=0;k<32;k++)
			{

				if(j>0)
					p = data[Ws*i + Ws*k + j - 1];
				else 
					p = 0;

				p=(p>>2)<<2;

				pp = data[Ws*i + Ws*k + j];
				pp = (pp>>2)<<2;

				if(p!=pp)
				{
					p=pp;

					u.bData[0] = 0x90 + chn[k];
					u.bData[1] = NoteMap[31-k];
					u.bData[2] = p;

					if((err = midiOutShortMsg((HMIDIOUT)handle, u.dwData)))
							PrintMidiOutErrorMsg(err);

				}
			}

			Sleep(lTempo);

			u.dwData = 0;

			for(k=0;k<32;k++)
			{	
				u.bData[0] = 0x80 + chn[k];
				u.bData[1] = NoteMap[31-k];

				if((err = midiOutShortMsg((HMIDIOUT)handle, u.dwData)))
						PrintMidiOutErrorMsg(err);
			}
		}
}

#define Wb 312
#define Hb 256

int ii=0, jj=0;
byte song[Wb*Hb];
bool sing = true;

void BeginJuliaSong()
{
	double	dx = (RIGHT - LEFT) / (double)Wb,
			dy = (BOTTOM - TOP) / (double)Hb;

	GenerateJuliaSong(Wb, Hb, dx, dy, LEFT, TOP, song);
}

void PlayJuliaPart()
{
	unsigned long	err;
	int k, p, pp;

	if(!sing)
	{
		u.dwData = 0;

		for(k=0;k<32;k++)
		{
			u.bData[0] = 0x80 + chn[k];
			u.bData[1] = NoteMap[31-k];

			if((err = midiOutShortMsg((HMIDIOUT)handle, u.dwData)))
				PrintMidiOutErrorMsg(err);
		}
	}

	u.bData[3] = 0;

	for(k=0;k<32;k++)	
	{
		if(jj>0)
			p = song[Wb*ii + Wb*k + jj - 1];
		else 
			p = 0;

		p=(p>>2)<<2;

		pp = song[Wb*ii + Wb*k + jj];
		pp = (pp>>2)<<2;

		if(p!=pp)
		{
			p=pp;

			u.bData[0] = 0x90 + chn[k];
			u.bData[1] = NoteMap[31-k];
			u.bData[2] = p;

			if((err = midiOutShortMsg((HMIDIOUT)handle, u.dwData)))
				PrintMidiOutErrorMsg(err);	
		}
	}
	
	//next music part
	if(jj<Wb) jj++;
	else
	{
		jj=0;
		if(ii<Hb/32) ii++;
		else ii=0;
	}

	sing = !sing;
}


//*************************************************

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT message, WPARAM wParam, LONG lParam) 
{
	double DX, DY, scale, a;
	HWND hw;
	HDC hdc;
	HBRUSH brush, oldbrush;
	HPEN pen;
	LPDRAWITEMSTRUCT lpdis;
	COLORREF OldBKColor, OldTextColor;
	RGBQUAD *pal;
	RECT r, rp;
	int x, y, w, h;
	PAINTSTRUCT ps;
	POINT pt;
	unsigned long err;

    switch(message) 
    { 
        case WM_INITDIALOG:

			// Resize screen saver dialog to 1024x600
			MoveWindow(hDlg, 0, 0, 1024, 612 + GetSystemMetrics(SM_CYCAPTION), TRUE);
 
            // Retrieve the application name from the .rc file.  
            LoadString(hMainInstance, idsAppName, szAppName, 
                       80 * sizeof(TCHAR)); 
 
            // Retrieve the .ini (or registry) file name. 
            LoadString(hMainInstance, idsIniFile, szIniFile, 
                       MAXFILELEN * sizeof(TCHAR)); 

			//*** INIT ***
			// Loads scr configuration
			LoadScreenSaverConfiguration();

			SendDlgItemMessage(hDlg, IDC_CYCLEFT, BM_SETCHECK, cycleLeft?BST_CHECKED:BST_UNCHECKED, 0);
			SendDlgItemMessage(hDlg, IDC_CYCRIGHT, BM_SETCHECK, cycleLeft?BST_UNCHECKED:BST_CHECKED, 0);

			// Init MIDI System
			if ((err = midiOutOpen(&handle, (UINT)-1, 0, 0, CALLBACK_NULL)))
			{
				mute = true;
				EnableWindow(GetDlgItem(hDlg, IDC_MUTE), FALSE);
				PrintMidiOutErrorMsg(err);
			}

			if(!mute)
			{
				if (instrIndex > 95) instrIndex += 24;				

				u.dwData  = 0;
				u.bData[1]= instrIndex;

				// Select instrument for all channels
				for(int i = 0; i<16; i++)
				{
					u.bData[0]=0xC0 + i;
					if((err = midiOutShortMsg((HMIDIOUT)handle, u.dwData)))
						PrintMidiOutErrorMsg(err);
				}
			}

			if(mute)
			{
				SendDlgItemMessage(hDlg, IDC_MUTE, BM_SETCHECK, BST_CHECKED, 0);

				EnableWindow(GetDlgItem(hDlg, IDC_PLAYMUS), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_INSTRUMENT), FALSE);
				EnableWindow(GetDlgItem(hDlg, IDC_TEMPO), FALSE);
			}


            // Initialize scrollbar controls
            hw = GetDlgItem(hDlg, ID_SPEED); 
            SetScrollRange(hw, SB_CTL, MINVEL, MAXVEL, FALSE); 
            SetScrollPos(hw, SB_CTL, lSpeed, TRUE);

			hw = GetDlgItem(hDlg, IDC_TEMPO);
			SetScrollRange(hw, SB_CTL, MINTMP, MAXTMP, FALSE); 
			SetScrollPos(hw, SB_CTL, lTempo, TRUE);
 
			// Init predefined constants combo box
			hw = GetDlgItem(hDlg, IDC_PRECONST);

			for(int i = 0; i < sizeof(predefc) / sizeof(char*); i++)
				SendMessage(hw, CB_ADDSTRING, 0, (LPARAM)predefc[i]);

			// Default constant
			xtoa(buff, PX, PY);
			SendMessage(hw, WM_SETTEXT, 0, (LPARAM)buff);			

			// Init coloring algorithm combo box
			hw = GetDlgItem(hDlg, IDC_COLALG);

			for(int i = 0; i < sizeof(colalg) / sizeof(char*); i++)
				SendMessage(hw, CB_ADDSTRING, 0, (LPARAM)colalg[i]);

			// Select algorithm 
			SendMessage(hw, CB_SETCURSEL, colAlgIndex, 0);

			// Instrument combo init
			hw = GetDlgItem(hDlg, IDC_INSTRUMENT);

			for(int i = 0; i < sizeof(instruments) / sizeof(char*); i++)
				SendMessage(hw, CB_ADDSTRING, 0, (LPARAM)instruments[i]);

			// Select default instrument
			SendMessage(hw, CB_SETCURSEL, instrIndex, 0); 

			// Get palette listbox item
			hw = GetDlgItem(hDlg, IDC_PALST);

			if(!InitPalettePickerData(szPaletteFile, hw))
			{
				MessageBox(hDlg, "Palette data not found!", "FractalArt", MB_OK | MB_ICONERROR);
				PostQuitMessage(1);
				return FALSE;
			}
			
			// Selects the palette
			SendDlgItemMessage(hDlg, IDC_PALST, LB_SETCURSEL, palIndex, 0);
			palette = (RGBQUAD*)SendDlgItemMessage(hDlg, IDC_PALST, LB_GETITEMDATA, palIndex, NULL);

			// Generate small mandelbrot
			hdc = GetDC(hDlg);

			srect.left = 9;
			srect.top = 32;
			srect.right = 266;
			srect.bottom = 245;

			r = srect;
			mWidth = r.right - r.left;
			mHeight = r.bottom - r.top;

			hBMP2 = CreateCompatibleBitmap(hdc, mWidth, mHeight );
			bi2.biWidth = mWidth;
			bi2.biHeight = mHeight;
			bi2.biSizeImage = mWidth * mHeight * 4;

			// Allocate memory for image
			sbits = (RGBQUAD*)malloc(bi2.biSizeImage + 16*3);
			mdc2  = CreateCompatibleDC(hdc);
			hOldBMP = (HBITMAP) SelectObject(mdc2, hBMP2);

			DX = (rightM - leftM) / mWidth;
			DY = (bottomM - topM) / mHeight;

			PaintMandelbrot(mWidth, mHeight, DX, DY, leftM, topM, sbits);

			SetDIBits(hdc, hBMP2, 0, mHeight, sbits, (BITMAPINFO*)&bi2, DIB_RGB_COLORS);

			// Generate julia
			GetClientRect(hDlg, &r);
			r.top += 7;
			r.bottom = r.top + 549;
			r.left += 270;
			r.right -= 2;
			
			brect = r;

			jWidth = brect.right-brect.left;
			jHeight = brect.bottom-brect.top;

			bi1.biWidth = jWidth, 
			bi1.biHeight = jHeight,
			bi1.biSizeImage = jWidth * jHeight * 4;

			hBMP1 = CreateCompatibleBitmap(hdc, jWidth , jHeight);
			
			bits = (RGBQUAD*)malloc(bi1.biSizeImage + 16 * 3);
			mdc1 = CreateCompatibleDC(hdc);
			hOldBMP = (HBITMAP) SelectObject(mdc1, hBMP1);

			//Sets the scrollbar
			hPrgBar = GetDlgItem(hDlg, IDC_PAINTPROGRESS);			
			SendMessage(hPrgBar, PBM_SETSTEP, (WPARAM) 1, 0);

			AllocateColorMap(false);
			RePaint_Small();
			RePaint_Big();			

			ReleaseDC(hDlg, hdc);
			break;

		case WM_PAINT:
			// Paint fractals
			hdc = BeginPaint(hDlg, &ps);

			//paint small picture
			BitBlt(hdc, srect.left, srect.top, mWidth, mHeight, mdc2, 0 , 0, SRCCOPY);
			FrameRect(hdc, &srect, CreateSolidBrush(0));

			// Paint big picture
			BitBlt(hdc, brect.left, brect.top, jWidth, jHeight, mdc1, 0, 0, SRCCOPY);
			FrameRect(hdc, &brect, CreateSolidBrush(0));


			SelectObject(hdc, GetStockObject(WHITE_PEN));
			brush = CreateSolidBrush(RGB(0xFF, 0xA5, 0x00));
			oldbrush = (HBRUSH) SelectObject(hdc, brush);


			// Drawing Orange point	
			if(!bigmb)
			{
				// From complex plane coordinates to screen coordinates
				x = (PX - leftM) * double(mWidth) / (rightM - leftM) + srect.left; 
				y = (PY - bottomM) * double(mHeight) / (topM - bottomM) + srect.top;
			}
			else
			{
				x = (PX - LEFT) * double(jWidth) / (RIGHT - LEFT) + brect.left; 
				y = (PY - BOTTOM) * double(jHeight) / (TOP - BOTTOM) + brect.top;
			}

			Ellipse(hdc, x - 2, y - 2, x + 2, y + 2); // Small circle

			// Destroying objects
			SelectObject(hdc, oldbrush);
			DeleteObject(brush);

			EndPaint(hDlg, &ps);
			break;

		case WM_SETCURSOR:
			GetCursorPos(&pt);
			ScreenToClient(hDlg, &pt);

			if(PtInRect(&brect, pt) || PtInRect(&srect, pt))
			{
				SetCursor(LoadCursor(NULL, IDC_CROSS));
				return TRUE;
			}

			break;

		case WM_MOUSEMOVE:
			GetCursorPos(&pt);
			ScreenToClient(hDlg, &pt);

			if(PtInRect(&brect, pt))
			{
				V2W(DX, DY, pt.x - brect.left, pt.y - brect.top, jWidth, jHeight);
			
				xtoa(buff, DX, DY);
				SetWindowText(GetDlgItem(hDlg, IDC_LOC), buff);
				break;
			}

			if(PtInRect(&srect, pt))
			{
				DX = double(pt.x - srect.left) * (rightM - leftM) / (double)mWidth + leftM;
				DY = double(pt.y - srect.top) * (topM - bottomM) / (double)mHeight + bottomM;

				xtoa(buff, DX, DY);

				SetWindowText(GetDlgItem(hDlg, IDC_LOC), buff);
				break;
			}

				SetWindowText(GetDlgItem(hDlg, IDC_LOC), "ShareITC © http://www.shareitc.eu/");
			break;

		case WM_MBUTTONUP:
			GetCursorPos(&pt);
			ScreenToClient(hDlg, &pt);

			if(!PtInRect(&brect, pt))
				break;

			//Select constant from enlarged Mandelbrot
			if(bigmb)
			{
				V2W(PX, PY, pt.x - brect.left, pt.y - brect.top, jWidth, jHeight);
				
				xtoa(buff, PX, PY);
				SendDlgItemMessage(hDlg, IDC_PRECONST, WM_SETTEXT, 0, (LPARAM)buff);

				//paint only small img
				RePaint_Small();

				InvalidateRect(hDlg, &brect, FALSE);
				InvalidateRect(hDlg, &srect, FALSE);
			}
			else //Plot Julia orbit
			{
				PlotJuliaOrbit(GetDC(hDlg), pt.x, pt.y);
			}

			SetFocus(GetDlgItem(hDlg, IDC_PAINTPROGRESS));
			break;

		case WM_MOUSEWHEEL:
			if(bigmb) break;

			GetCursorPos(&pt);
			ScreenToClient(hDlg, &pt);

			if(!PtInRect(&brect, pt))
				break;
			
			if(SendDlgItemMessage(hDlg, IDC_CYCLEFT, BM_GETCHECK, 0, 0)==BST_CHECKED)
			{
				if((int)wParam>0) 
					ShiftLeft_PaintJulia();
				else
					ShiftRight_PaintJulia();
			}
			else
			{				
				if((int)wParam>0) 
					ShiftRight_PaintJulia();
				else
					ShiftLeft_PaintJulia();
			}
			InvalidateRect(hDlg, &brect, FALSE);
			break;
			

		case WM_RBUTTONUP:
			GetCursorPos(&pt);
			ScreenToClient(hDlg, &pt);

			if(!PtInRect(&brect, pt))
				break;
			
			pt.x -= brect.left;
			pt.y -= brect.top;

			V2W(DX, DY, pt.x, pt.y, jWidth, jHeight);
			scale = 2;
			goto RESCALE;
			break;

		case WM_LBUTTONUP:			
			GetCursorPos(&pt);			
			ScreenToClient(hDlg, &pt);

			if(!PtInRect(&srect, pt))
				goto BIGRECT;

			// Clicking the small picture
			if(bigmb) break;

			PX = double(pt.x - srect.left) * (rightM - leftM) / (double)mWidth + leftM;
			PY = double(pt.y - srect.top) * (topM - bottomM) / (double)mHeight + bottomM;

			xtoa(buff, PX, PY);
			SendDlgItemMessage(hDlg, IDC_PRECONST, WM_SETTEXT, 0, (LPARAM)buff);

			// Redraw julia & refresh mandelbrot
			RePaint_Big();
			
			InvalidateRect(hDlg, &srect, FALSE);
			InvalidateRect(hDlg, &brect, FALSE);			
			break;
BIGRECT:
			if(!PtInRect(&brect, pt))
				break;
			
			pt.x -= brect.left;
			pt.y -= brect.top;
			
			V2W(DX, DY, pt.x, pt.y, jWidth, jHeight);
			scale = 0.5;

RESCALE:
			a = (BOTTOM - TOP) * scale;
			TOP = DY - (DY - TOP)* scale, BOTTOM = TOP + a;
			a = (RIGHT - LEFT) * scale;
			LEFT = DX - (DX - LEFT) * scale, RIGHT = LEFT + a;

			RePaint_Big();
			InvalidateRect(hDlg, &brect, FALSE);
			SetFocus(GetDlgItem(hDlg, IDC_PAINTPROGRESS));
			break;


		case WM_DRAWITEM:
			lpdis = (LPDRAWITEMSTRUCT)lParam; //the item

			if(lpdis->itemID<0) break;

			r = rp = lpdis->rcItem;

			// Gets hdc for the item
			hdc = lpdis->hDC;

			// Save these values to restore them when done drawing.
			OldBKColor = GetBkColor(hdc);
			OldTextColor = GetTextColor(hdc);

			// If this item is selected, set the background color
			// and the text color to appropriate values. Also, erase
			// rect by filling it with the background color.
			if((lpdis->itemAction|ODA_SELECT)&&
				(lpdis->itemState&ODS_SELECTED))
			{
				SetTextColor(hdc, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
				SetBkColor(hdc, ::GetSysColor(COLOR_HIGHLIGHT));
				FillRect(hdc, &lpdis->rcItem, CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT)));
			}
			else
				FillRect(hdc, &lpdis->rcItem, CreateSolidBrush(OldBKColor));


			// If this item has focus draw a frame
			if ((lpdis->itemAction | ODA_FOCUS) &&
				(lpdis->itemState & ODS_FOCUS))
				DrawFocusRect(hdc, &lpdis->rcItem);


			// Move text rect just a little bit
			r.left += 2;

			// Deflate palette rect
			rp.bottom -= 2;
			rp.left += 20;
			rp.right -= 2;		
			rp.top += 2;

			// Calculate width, height and position of palette box		
			x = rp.left;
			y = rp.top;
			w = rp.right - rp.left;
			h = rp.bottom - rp.top;

			// Get the attached palette from itemData
			pal=(RGBQUAD*)lpdis->itemData;

			// If a palette was attached to the item
			if(pal)
			{
				if(lpdis->itemState & ODS_DISABLED)						
					SetTextColor(hdc, ::GetSysColor(COLOR_INACTIVECAPTIONTEXT));

				// Prepare pen
				LOGPEN logpen;
				logpen.lopnStyle=PS_SOLID;
				logpen.lopnWidth.x=1;

				// Draw the palette
				for(int j=0;j<=w;j++)
				{
					// Change pen color
					RGBQUAD c = pal[j*(int)(256/w)];
					logpen.lopnColor = RGB(c.rgbRed, c.rgbGreen, c.rgbBlue);
					pen = CreatePenIndirect(&logpen);
					SelectObject(hdc, pen);
					// Draw lines
					MoveToEx(hdc, x+j, y, NULL);
					LineTo(hdc, x+j, y+h);
					DeleteObject(pen);
				}

				// Draw a frame around the palette
				//FrameRect(hdc, &rp, CreateSolidBrush(BLACK_BRUSH));			
			}

			itoa(lpdis->itemID, buff, 16);
			SetBkMode(hdc, TRANSPARENT);
			DrawText(hdc, buff , strlen(buff), &r, DT_SINGLELINE|DT_VCENTER);
			SetTextColor(hdc, OldTextColor);
			SetBkColor(hdc, OldBKColor);
			break;

        case WM_HSCROLL: 
            // Process scroll bar input, adjusting the lSpeed 
            // value as appropriate. 
			if ((HWND)lParam == GetDlgItem(hDlg, IDC_TEMPO)) goto SETEMPO;

SPEED:
            switch (LOWORD(wParam)) 
                { 
                    case SB_PAGEUP: 
                        --lSpeed; 
                    break; 
 
                    case SB_LINEUP: 
                        --lSpeed; 
                    break; 
 
                    case SB_PAGEDOWN: 
                        ++lSpeed; 
                    break; 
 
                    case SB_LINEDOWN: 
                        ++lSpeed; 
                    break; 
 
                    case SB_THUMBPOSITION: 
                        lSpeed = HIWORD(wParam); 
                    break; 
 
                    case SB_BOTTOM: 
                        lSpeed = MINVEL; 
                    break; 
 
                    case SB_TOP: 
                        lSpeed = MAXVEL; 
                    break; 
 
                    case SB_THUMBTRACK: 
                    case SB_ENDSCROLL: 
                        return TRUE; 
                    break; 
                } 

                if ((int) lSpeed <= MINVEL) 
                    lSpeed = MINVEL;
                if ((int) lSpeed >= MAXVEL) 
                    lSpeed = MAXVEL;
 
                SetScrollPos((HWND) lParam, SB_CTL, lSpeed, TRUE); 
            break;
SETEMPO:
			switch (LOWORD(wParam)) 
                { 
                    case SB_PAGEUP: 
                        --lTempo; 
                    break; 
 
                    case SB_LINEUP: 
                        --lTempo; 
                    break; 
 
                    case SB_PAGEDOWN: 
                        ++lTempo; 
                    break; 
 
                    case SB_LINEDOWN: 
                        ++lTempo;
                    break; 
 
                    case SB_THUMBPOSITION: 
                        lTempo = HIWORD(wParam); 
                    break; 
 
                    case SB_BOTTOM: 
                        lTempo = MINTMP; 
                    break; 
 
                    case SB_TOP: 
                        lTempo = MAXTMP; 
                    break; 
 
                    case SB_THUMBTRACK:
                    case SB_ENDSCROLL: 
                        return TRUE; 
                    break;
                } 

                if ((int) lTempo <= MINTMP) 
                    lTempo = MINTMP; 
				if ((int) lTempo >= MAXTMP) 
					lTempo = MAXTMP; 
 
                SetScrollPos((HWND) lParam, SB_CTL, lTempo, TRUE); 
			break;
 
        case WM_COMMAND: 
            switch(wParam)
            {
				case IDC_SWITCH:
					bigmb=!bigmb;
					RePaint_Small();
					InvalidateRect(hDlg, &srect, FALSE);
					EnableWindow(GetDlgItem(hDlg, IDC_ANTIALIASING), bigmb?FALSE:TRUE);

				case IDC_HOME:
					LEFT = bigmb?leftM:leftJ;
					RIGHT = bigmb?rightM:rightJ;
					TOP = bigmb?topM:topJ;
					BOTTOM = bigmb?bottomM:bottomJ;
					
					RePaint_Big();					
					InvalidateRect(hDlg, &brect, FALSE);
				break;

				case IDC_SAVESCR: 
                    // Write the current redraw speed variable to the .ini file
					cycleLeft = SendDlgItemMessage(hDlg, IDC_CYCLEFT, BM_GETCHECK, 0, 0)==BST_CHECKED;
					SaveScreenSaverConfiguration();

                case IDC_DISCARD:
                    EndDialog(hDlg, LOWORD(wParam) == IDOK);
					break;

				case IDC_PLAYMUS:
					PlayJuliaSmall();
					break;

				case IDC_ANTIALIASING:
					antialiasing=true;
					RePaint_Big();
					InvalidateRect(hDlg, &brect, FALSE);
					antialiasing=false;
					break;

				case IDC_MUTE:
					mute=!mute;

					EnableWindow(GetDlgItem(hDlg, IDC_PLAYMUS), mute?FALSE:TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_INSTRUMENT), mute?FALSE:TRUE);
					EnableWindow(GetDlgItem(hDlg, IDC_TEMPO), mute?FALSE:TRUE);					
					break;

				case MAKEWPARAM(IDC_PRECONST, CBN_SELENDOK):
					SendDlgItemMessage(hDlg, IDC_PRECONST, CB_GETLBTEXT, 
						SendDlgItemMessage(hDlg, IDC_PRECONST, CB_GETCURSEL, 0, 0), (LPARAM)buff);
					atox(buff, PX, PY);

					// Redraw julia & mandelbrot
					if(bigmb)	RePaint_Small();
					else		RePaint_Big();

					InvalidateRect(hDlg, &brect, FALSE);
					InvalidateRect(hDlg, &srect, FALSE);
				break;

				case MAKEWPARAM(IDC_PALST, LBN_SELCHANGE):					
					palIndex = SendDlgItemMessage(hDlg, IDC_PALST, LB_GETCURSEL, 0, 0);
					palette = (RGBQUAD*)SendDlgItemMessage(hDlg, IDC_PALST, LB_GETITEMDATA, palIndex, NULL);

					if(!bigmb)
					{
						FillBitmapFromColorMap(bits);
						InvalidateRect(hDlg, &brect, FALSE);
					}
					break;

				case MAKEWPARAM(IDC_COLALG, CBN_SELCHANGE):

					colAlgIndex = (int)SendDlgItemMessage(hDlg, IDC_COLALG, CB_GETCURSEL, 0, 0);

					switch(colAlgIndex)
					{
						case 0:						
							GenerateJuliaColorMap = GenerateJuliaColorMap_escape_time;
							break;
						case 1:
							GenerateJuliaColorMap = GenerateJuliaColorMap_escape_angle;
							break;
						case 2:
							GenerateJuliaColorMap = GenerateJuliaColorMap_distance_estimators;
							break;
						case 3:
							GenerateJuliaColorMap = GenerateJuliaColorMap_curvature_estimation;
							break;
						case 4:
							GenerateJuliaColorMap = GenerateJuliaColorMap_statistic;
							break;
						case 5:
							GenerateJuliaColorMap = GenerateJuliaColorMap_gauss;
							break;
						case 6:
							GenerateJuliaColorMap = GenerateJuliaColorMap_attractors;
						break;
						default:
							MessageBox(hDlg, "Algorithm not implemented yet!", "FractalArt", MB_OK | MB_ICONEXCLAMATION);
							return TRUE;
					}
					//repaint julia
					if(!bigmb)
					{
						RePaint_Big();
						InvalidateRect(hDlg, &brect, FALSE);
					}
					break;

				case MAKEWPARAM(IDC_INSTRUMENT, CBN_SELCHANGE):

					instrIndex = (int)SendDlgItemMessage(hDlg, IDC_INSTRUMENT, CB_GETCURSEL, 0, 0);

					if (instrIndex > 95) instrIndex += 24;				

					u.dwData  = 0;
					u.bData[1]= instrIndex;

					// Select instrument for all channels
					for(int i = 0; i<16; i++)
					{
						u.bData[0]=0xC0 + i;
						if((err = midiOutShortMsg((HMIDIOUT)handle, u.dwData)))
							PrintMidiOutErrorMsg(err);
					}

					break;
            } 
			break;

		case WM_CLOSE:
			EndDialog(hDlg, FALSE);
			return TRUE;

		case WM_DESTROY:
			free(sbits);
			free(bits);

			free(jAliasMap);
			free(jColorMap);

			free(szPaletteFile);

			hw = GetDlgItem(hDlg, IDC_PALST);
			x = SendMessage(hw, LB_GETCOUNT, 0, 0);

			for(int i=0; i<x; i++)
			{
				pal = (RGBQUAD*)SendMessage(hw, LB_GETITEMDATA, i, NULL);
				delete []pal;
			}

			DeleteObject(hBMP1);
			DeleteObject(hBMP2);

			// Close the MIDI device 
			if(!mute)				
				midiOutClose(handle);

			#ifdef _DEBUG
				_CrtDumpMemoryLeaks();
			#endif

			break;
    }
    return FALSE; 
}


LRESULT WINAPI ScreenSaverProc(HWND hwnd, UINT message, WPARAM wParam, LONG lParam) 
{ 
    static HDC          hdc;      // device-context handle  
    static RECT         rc;       // RECT structure  
    static UINT         uTimer;   // timer identifier  
	static UINT			fTimer;
	unsigned long err;

    switch(message) 
    { 
        case WM_CREATE: 
 
            // Retrieve the application name from the .rc file. 
			LoadStringA(hMainInstance, idsAppName, (LPSTR)szAppName, 40 * sizeof(TCHAR)); 
 
            // Retrieve the .ini (or registry) file name. 
			LoadStringA(hMainInstance, idsIniFile, (LPSTR)szIniFile, MAXFILELEN * sizeof(TCHAR)); 
             
            // Retrieve data from the registry.  
			LoadScreenSaverConfiguration();

			// Set used palette
			if(!LoadPalette(szPaletteFile, palIndex))
			{				
				for(int i=0;i<256;i++)
				{
					palette[i].rgbRed = i;
					palette[i].rgbGreen = i;
					palette[i].rgbBlue = i;
					palette[i].rgbReserved = 0;
				}
			}
 
			if(!mute)
			{
				// Init MIDI System
				if ((err = midiOutOpen(&handle, (UINT)-1, 0, 0, CALLBACK_NULL)))
				{
					mute=true;
					PrintMidiOutErrorMsg(err);
				}
				else
				{
					if (instrIndex > 95) instrIndex += 24;

					u.dwData  = 0;
					u.bData[1]= instrIndex;

					// Select instrument for all channels
					for(int i = 0; i<16; i++)
					{
						u.bData[0]=0xC0 + i;
						if((err = midiOutShortMsg((HMIDIOUT)handle, u.dwData)))
							PrintMidiOutErrorMsg(err);
					}

					BeginJuliaSong();

					// Set a timer for the screen saver to play the fractal music
					fTimer = SetTimer(hwnd, 2, lTempo, NULL);
				}				
			}

			// Set a timer for the screen saver window using the 
            // redraw rate stored in Regedit.ini.
            uTimer = SetTimer(hwnd, 1, (MAXVEL + MINVEL - lSpeed)*10, NULL); 			
            break; 
			
 
        case WM_ERASEBKGND:
 
            // The WM_ERASEBKGND message is issued before the 
            // WM_TIMER message, allowing the screen saver to 
            // paint the background as appropriate. 

            hdc = GetDC(hwnd); 
            GetClientRect (hwnd, &rc); 
			FillRect (hdc, &rc, CreateSolidBrush(BLACK_BRUSH)); 

			mdc1 = CreateCompatibleDC(hdc);

			hBMP2 = LoadBitmap(hMainInstance, MAKEINTRESOURCE(IDB_LOGO));

			hOldBMP = (HBITMAP) SelectObject(mdc1, hBMP2);

			GetObject(hBMP2, sizeof(BITMAP), &bmp);

			BitBlt(hdc, (rc.right-bmp.bmWidth)/2, (rc.bottom-bmp.bmHeight)/2, bmp.bmWidth, bmp.bmHeight, mdc1, 0, 0, SRCCOPY);
			
			brect = rc;
			
			jWidth = brect.right-brect.left;
			jHeight = brect.bottom-brect.top;	

			bi1.biWidth = rc.right-rc.left, bi1.biHeight = rc.bottom-rc.top,
			bi1.biSizeImage = bi1.biWidth * bi1.biHeight * 4;

			hBMP1 = CreateCompatibleBitmap(hdc, bi1.biWidth , bi1.biHeight);			
			
			bits = (RGBQUAD*)malloc(bi1.biSizeImage + 16 * 3);			
			
			hOldBMP = (HBITMAP) SelectObject(mdc1, hBMP1);

			AllocateColorMap(true);
			RePaint_Big();
			
            ReleaseDC(hwnd, hdc); 
            break; 
 
        case WM_TIMER:
			switch(wParam)
			{
			case 1:
				hdc = GetDC(hwnd); 
				GetClientRect(hwnd, &rc); 

				cycleLeft ? ShiftLeft_PaintJulia() : ShiftRight_PaintJulia();

				BitBlt(hdc, 0, 0, jWidth, jHeight, mdc1, 0, 0, SRCCOPY);

				ReleaseDC(hwnd,hdc); 
				break;

			case 2:
				PlayJuliaPart();

				break;
			}
            break; 
 
        case WM_DESTROY:  
            // When the WM_DESTROY message is issued, the screen saver 
            // must destroy any of the timers that were set at WM_CREATE 
            // time and release additional memory allocated at startup.

			// Close the MIDI device if we have open it
			if(!mute) midiOutClose(handle);

			// Delete created objects
			DeleteObject(hBMP1);
			DeleteObject(hBMP2);
			
			free(bits);

			delete []palette;
			free(jColorMap);
			
			free(szPaletteFile);

			// Kill Timers
            if (uTimer)
                KillTimer(hwnd, uTimer);

			if(fTimer) 
				KillTimer(hwnd, fTimer);
			
			#ifdef _DEBUG
				_CrtDumpMemoryLeaks();
			#endif

            break; 
    } 
 
    // DefScreenSaverProc processes any messages ignored by ScreenSaverProc. 
    return DefScreenSaverProc(hwnd, message, wParam, lParam); 
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst) 
{	
    return TRUE; 
}