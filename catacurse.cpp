#if (defined _WIN32 || defined WINDOWS)
#include "catacurse.h"
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string>
#include <map>

// requests W2K baseline Windows API ... unsure if this has link-time consequences w/posix_time.cpp which does not specify
#define _WIN32_WINNT 0x0500
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

// #define TILES 1

#define FULL_IMAGE_INFO 1

// we allow move construction/assignment but not copy construction/assignment
class OS_Image
{
private:
	HANDLE _x;
#ifdef FULL_IMAGE_INFO
	BITMAPINFOHEADER _data;
	mutable RGBQUAD* _pixels;
#else
	BITMAPCOREHEADER _data;
#endif
	bool _have_info;
public:
	OS_Image() : _x(0), _pixels(0), _have_info(false) { memset(&_data, 0, sizeof(_data)); }
	OS_Image(const char* src, int width = 0, int height = 0) : _x(LoadImageA(0, src, IMAGE_BITMAP, width, height, LR_LOADFROMFILE)),_pixels(0),_have_info(false) { init(); }
	OS_Image(const wchar_t* src, int width = 0, int height = 0) : _x(LoadImageW(0, src, IMAGE_BITMAP, width, height, LR_LOADFROMFILE)), _pixels(0),_have_info(false) { init(); }
	OS_Image(const OS_Image& src) = delete;
	OS_Image(OS_Image&& src) : _x(src._x),_data(src._data),_have_info(src._have_info) {
		src._x = 0;
#ifdef FULL_IMAGE_INFO
		_pixels = src._pixels;
		src._pixels = 0;
#endif
	}
#ifdef FULL_IMAGE_INFO
	// clipping constructor
	OS_Image(const OS_Image& src, const size_t origin_x, const size_t origin_y, const size_t width, const size_t height)
	: _x(0), _data(src._data), _pixels(0), _have_info(false)
	{
		if (0 >= width) throw std::logic_error("OS_Image::OS_Image: 0 >= width");
		if (0 >= height) throw std::logic_error("OS_Image::OS_Image: 0 >= height");
		if (((size_t)(-1)/sizeof(_pixels[0]))/width < height) throw std::logic_error("OS_Image::OS_Image: ((size_t)(-1)/sizeof(_pixels[0]))/width < height");
		if (src.width() <= origin_x) throw std::logic_error("OS_Image::OS_Image: src.width() <= origin_x");
		if (src.height() <= origin_y) throw std::logic_error("OS_Image::OS_Image: src.height() <= origin_y");
		if (src.width() - origin_x < width) throw std::logic_error("OS_Image::OS_Image: src.width() - origin_x < width");
		if (src.height() - origin_y < height) throw std::logic_error("OS_Image::OS_Image: src.height() - origin_y < height");
		RGBQUAD* const incoming = src.pixels();

		_data.biWidth = width;
		_data.biHeight = -((signed long)height);
		RGBQUAD* working = (RGBQUAD*)calloc(width*height, sizeof(RGBQUAD));
		if (!working) throw std::bad_alloc();

		// would like static assertion here but not needed
		for (size_t scan_y = 0; scan_y < height; scan_y++) {
			memmove(working + (scan_y*width), incoming + ((scan_y + origin_y)*src.width()), sizeof(RGBQUAD)*width);
		}
		_x = CreateCompatibleBitmap(0,width,height);
		if (!_x) throw std::bad_alloc();
		if (!SetDIBits(NULL, (HBITMAP)_x, 0, height, working, (LPBITMAPINFO)(&_data), DIB_RGB_COLORS)) {
			free(working);
			throw std::bad_alloc();
		}
		free(working);
	}
#endif

	~OS_Image() { clear(); }

	OS_Image& operator=(const OS_Image& src) = delete;
	OS_Image& operator=(OS_Image&& src)
	{
		_x = src._x;
		src._x = 0;
		_data = src._data;
		_have_info = src._have_info;
#ifdef FULL_IMAGE_INFO
		_pixels = src._pixels;
		src._pixels = 0;
#endif
		return *this;
	}

	HANDLE handle() const { return _x; }
#ifdef FULL_IMAGE_INFO
	size_t width() const { return _data.biWidth; }
	size_t height() const { return -_data.biHeight; }
#else
	size_t width() const { return _data.bcWidth; }
	size_t height() const { return _data.bcHeight; }
#endif

	void clear()
	{
		if (_x) {
			DeleteObject(_x);
			_x = 0;
		}
		if (_pixels) {
			free(_pixels);
			_pixels = 0;
		}
	}
private:
	void init() {
		if (_x && !_have_info) {
			memset(&_data, 0, sizeof(_data));
#ifdef FULL_IMAGE_INFO
			_data.biSize = sizeof(_data);
#else
			_data.bcSize = sizeof(_data);
#endif
			_have_info = GetDIBits(0, (HBITMAP)_x, 0, 0, NULL, (LPBITMAPINFO)(&_data), DIB_RGB_COLORS);	// XXX needs testing
#ifdef FULL_IMAGE_INFO
			_data.biBitCount = 32;	// force alpha channel in (no padding bytes)
			_data.biCompression = BI_RGB;
			_data.biHeight = (0 < _data.biHeight) ? -_data.biHeight : _data.biHeight;	// force top-down
#endif
		}
	}

#ifdef FULL_IMAGE_INFO
	RGBQUAD* pixels() const
	{
		if (!_have_info) return 0;
		if (_pixels) return _pixels;
		RGBQUAD* const tmp = (RGBQUAD*)calloc(width()*height(), sizeof(RGBQUAD));
		if (!tmp) return 0;
		if (!GetDIBits(0, (HBITMAP)_x, 0, 0, NULL, (LPBITMAPINFO)(&_data), DIB_RGB_COLORS))
			{
			free(tmp);
			return 0;
			}
		return _pixels = tmp;
	}
#endif
};

// tiles support
std::map<unsigned short, OS_Image> _cache;
std::map<std::string, unsigned short> _translate;	// const char* won't work -- may need to store base names seen first time with a rotation specifier

// tilesheet support
std::map<std::string, OS_Image> _tilesheets;
std::map<std::string, int > _tilesheet_tile;

// globals used by the main curses simulation
int fontwidth = 0;          //the width of the font, background is always this size
int fontheight = 0;         //the height of the font, background is always this size
int halfwidth = 0;          //half of the font width, used for centering lines
int halfheight = 0;          //half of the font height, used for centering lines
int WindowWidth = 0;        //Width of the actual window, not the curses window
int WindowHeight = 0;       //Height of the actual window, not the curses window
int WindowX = 0;            //X pos of the actual window, not the curses window
int WindowY = 0;            //Y pos of the actual window, not the curses window
HWND WindowHandle = 0;      //the handle of the window
#if _MSC_VER
							// MSVC is empirically broken (following is correct for _MSC_VER 1913 ... 1914)
#define BORDERWIDTH_SCALE 5
#define BORDERHEIGHT_SCALE 5
#else
							// assume we are on a working WinAPI (e.g., MingWin)
#define BORDERWIDTH_SCALE 2
#define BORDERHEIGHT_SCALE 2
#endif
const int WinTitleSize = GetSystemMetrics(SM_CYCAPTION);      //These lines ensure
const int WinBorderWidth = GetSystemMetrics(SM_CXDLGFRAME) * BORDERWIDTH_SCALE;  //that our window will
const int WinBorderHeight = GetSystemMetrics(SM_CYDLGFRAME) * BORDERHEIGHT_SCALE; // be a perfect size
const int ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
const int ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
#undef BORDERWIDTH_SCALE
#undef BORDERHEIGHT_SCALE

bool SetFontSize(const int x, const int y)
{
	if (4 >= x) return false;	// illegible
	if (4 >= y) return false;	// illegible
	if (x == fontwidth && y == fontheight) return false;	// no-op
	fontwidth = x;
	fontheight = y;
	halfwidth = fontwidth / 2;
	halfheight = fontheight / 2;
	WindowWidth = 80 * fontwidth;
	WindowHeight = 25 * fontheight;
	WindowX = (ScreenWidth / 2) - WindowWidth / 2;    //center this
	WindowY = (ScreenHeight / 2) - WindowHeight / 2;   //sucker
	return true;
}

bool WinResize()
{
	RECT tmp;
	if (!GetWindowRect(WindowHandle, &tmp)) return false;
	return MoveWindow(WindowHandle, tmp.left, tmp.top, WindowWidth + WinBorderWidth, WindowHeight + WinBorderHeight + WinTitleSize,true);	// \todo test whether this handles the backbuffer and WindowDC
}

std::string extract_file_infix(std::string src)
{
	{
	const char* x = src.c_str();
	const char* test = strrchr(x, '.');
	if (!test) return std::string();
	src = std::string(x, 0, test - x);
	}
	const char* test = strrchr(src.c_str(), '.');
	if (!test) return std::string();
	return std::string(test);
}

bool load_tile(const char* src)
{
	static unsigned short _next = 0;

	if (!src || !src[0]) return false;	// nothing to load
	if ((unsigned short)(-1) == _next) return false;	// at implementation limit
	if (_translate.count(src)) return true;	// already loaded
	const char* const has_rotation_specification = strchr(src, ':');
	std::string base_tile(has_rotation_specification ? std::string(src, 0, has_rotation_specification-src) : src);
	if (!_translate.count(base_tile.c_str())) {
		const char* const is_from_tilesheet = strchr(base_tile.c_str(), '#');
		if (is_from_tilesheet) {
			std::string tilesheet(src, 0, is_from_tilesheet - src);
			std::string offset(is_from_tilesheet +1);
			int index = 0;
			try {
				index = std::stoi(offset);
				if (0 > index) return false;
			} catch (std::exception& e) {
				return false;
			}
			if (!_tilesheets.count(tilesheet)) {
				OS_Image relay(tilesheet.c_str());
				if (!relay.handle()) return false;	// failed to load
				// read off the tilesheet size from the filename ...#.png
				std::string infix = extract_file_infix(tilesheet);
				if (infix.empty()) return false;
				try {
					int dim = std::stoi(infix);
					if (0 >= dim) return false;
					_tilesheet_tile[tilesheet] = dim;
				} catch (std::exception& e) {
					return false;
				}
				// we have to handle the per-sheet configuration elsewhere
				_tilesheets[tilesheet] = std::move(relay);
			}
			const int dim = _tilesheet_tile[tilesheet];
			const OS_Image& src = _tilesheets[tilesheet];
			const size_t scaled_x = src.width() / dim;
			const size_t scaled_y = src.height() / dim;
			const int index_y = index / scaled_x;
			if (index_y <= scaled_y) return false;
			const int index_x = index - index_y * scaled_x;
			OS_Image image(src, dim*index_x, dim*index_y, dim, dim);	// clipping constructor
			if (!image.handle()) return false;	// failed to load
			_translate[base_tile] = ++_next;
			_cache[_next] = std::move(image);
			if (SetFontSize(16, 16) && WindowHandle) WinResize();
		} else {
			OS_Image image(base_tile.c_str(), fontwidth, fontheight);
			if (!image.handle()) return false;	// failed to load
			_translate[base_tile] = ++_next;
			_cache[_next] = std::move(image);
			if (SetFontSize(16, 16) && WindowHandle) WinResize();
		}
	}
	if (!has_rotation_specification) return true;
	// \todo use the rotation specifier
	return true;
}

void flush_tilesheets()
{
	 _tilesheets.clear();
	 _tilesheet_tile.clear();
}

// struct definitions
//a pair of colors[] indexes, foreground and background
struct pairs
{
	int FG;//foreground index in colors[]
	int BG;//foreground index in colors[]
};

//The curse character struct, just a char, attribute, and color pair
//typedef struct
//{
// char character;//the ascii actual character
// int attrib;//attributes, mostly for A_BLINK and A_BOLD
// pairs color;//pair of foreground/background, indexed into colors[]
//} cursechar;

//Individual lines, so that we can track changed lines
struct curseline {
	char *chars;
	char *FG;
	char *BG;
#ifdef TILES
	unsigned short* background_tiles;
	unsigned short* tiles;
#endif
	bool touched;

	// would not want to use destructor in pure C mode
	~curseline();

	void init(int ncols);
};

void curseline::init(const int ncols)
{
	chars = (char*)calloc(ncols, sizeof(char));
	FG = (char*)calloc(ncols, sizeof(char));
	BG = (char*)calloc(ncols, sizeof(char));
#ifdef TILES
	background_tiles = (unsigned short*)calloc(ncols, sizeof(unsigned short));
	tiles = (unsigned short*)calloc(ncols, sizeof(unsigned short));
#endif
	touched = true;
}

curseline::~curseline()
{	// this should not be on a critical path; be nice to archaic, not-remotely-ISO compilers that splat on free(NULL)
	if (chars) { free(chars); chars = 0; }
	if (FG) { free(FG); FG = 0; }
	if (BG) { free(BG); BG = 0; }
#if TILES
	if (background_tiles) { free(background_tiles); background_tiles = 0; }
	if (tiles) { free(tiles); tiles = 0; }
#endif
}

//The curses window struct
struct WINDOW {
	int x;//left side of window
	int y;//top side of window
	int width;//width of the curses window
	int height;//height of the curses window
	int FG;//current foreground color from attron
	int BG;//current background color from attron
	bool inuse;// Does this window actually exist?
	bool draw;//Tracks if the window text has been changed
	int cursorx;//x location of the cursor
	int cursory;//y location of the cursor
	curseline *line;
};

//Window Functions, Do not call these outside of catacurse.cpp
void WinDestroy();
void CheckMessages();
/// int FindWin(WINDOW *wnd);	// may want this at some point
LRESULT CALLBACK ProcessMessages(HWND__ *hWnd, unsigned int Msg, WPARAM wParam, LPARAM lParam);

/*
 Optimal tileset modification target is DrawWindow, with a mapping from codepoint,(foreground) color pairs to tiles with a transparent background.
*/

//***********************************
//Globals                           *
//***********************************

WINDOW *mainwin;
const WCHAR *szWindowClass = (L"CataCurseWindow");    //Class name :D
HINSTANCE WindowINST = 0;   //the instance of the window...normally obtained from WinMain but we don't have WinMain
HDC WindowDC;           //Device Context of the window, used for backbuffer
int lastchar;          //the last character that was pressed, resets in getch
int inputdelay;         //How long getch will wait for a character to be typed
//WINDOW *_windows;  //Probably need to change this to dynamic at some point
//int WindowCount;        //The number of curses windows currently in use
HDC backbuffer;         //an off-screen DC to prevent flickering, lower cpu
HFONT font;             //Handle to the font created by CreateFont
RGBQUAD *windowsPalette;  //The coor palette, 16 colors emulates a terminal
pairs *colorpairs;   //storage for pair'ed colored, should be dynamic, meh
unsigned char *dcbits;  //the bits of the screen image, for direct access
char szDirectory[MAX_PATH] = "";
int haveCustomFont = 0;	// custom font was there and loaded

// 

//***********************************
//Non-curses, Window functions      *
//***********************************

//Registers, creates, and shows the Window!!
bool WinCreate()
{
    WNDCLASSEXW WindowClassType;
    const WCHAR *szTitle=  (L"Cataclysm");
    WindowClassType.cbSize = sizeof(WNDCLASSEXW);
    WindowClassType.style = 0;//No point in having a custom style, no mouse, etc
    WindowClassType.lpfnWndProc = ProcessMessages;//the procedure that gets msgs
    WindowClassType.cbClsExtra = 0;
    WindowClassType.cbWndExtra = 0;
    WindowClassType.hInstance = WindowINST;// hInstance
    WindowClassType.hIcon = LoadIcon(WindowINST, IDI_APPLICATION);//Default Icon
    WindowClassType.hIconSm = LoadIcon(WindowINST, IDI_APPLICATION);//Default Icon
    WindowClassType.hCursor = LoadCursor(NULL, IDC_ARROW);//Default Pointer
    WindowClassType.lpszMenuName = NULL;
    WindowClassType.hbrBackground = 0;//Thanks jday! Remove background brush
    WindowClassType.lpszClassName = szWindowClass;
    if (!RegisterClassExW(&WindowClassType)) return false;
	const unsigned int WindowStyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME) & ~(WS_MAXIMIZEBOX);
    WindowHandle = CreateWindowExW(WS_EX_APPWINDOW || WS_EX_TOPMOST * 0,
                                   szWindowClass , szTitle,WindowStyle, WindowX,
                                   WindowY, WindowWidth + WinBorderWidth,
                                   WindowHeight + WinBorderHeight + WinTitleSize,
		                           0, 0, WindowINST, NULL);
    if (WindowHandle == 0) return false;
    ShowWindow(WindowHandle,5);
    return true;
};

//Unregisters, releases the DC if needed, and destroys the window.
void WinDestroy()
{
	if (backbuffer && DeleteDC(backbuffer)) backbuffer = 0;
	if (WindowDC && ReleaseDC(WindowHandle, WindowDC)) WindowDC = 0;
	if (WindowHandle && DestroyWindow(WindowHandle)) WindowHandle = 0;
	UnregisterClassW(szWindowClass, WindowINST);	// would happen on program termination anyway
};

//This function processes any Windows messages we get. Keyboard, OnClose, etc
LRESULT CALLBACK ProcessMessages(HWND__ *hWnd,unsigned int Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg){
        case WM_CHAR:               //This handles most key presses
            lastchar=(int)wParam;
            switch (lastchar){
                case 13:            //Reroute ENTER key for compatilbity purposes
                    lastchar=10;
                    break;
                case 8:             //Reroute BACKSPACE key for compatilbity purposes
                    lastchar=127;
                    break;
            };
            break;
        case WM_KEYDOWN:                //Here we handle non-character input
            switch (wParam){
                case VK_LEFT:
                    lastchar = KEY_LEFT;
                    break;
                case VK_RIGHT:
                    lastchar = KEY_RIGHT;
                    break;
                case VK_UP:
                    lastchar = KEY_UP;
                    break;
                case VK_DOWN:
                    lastchar = KEY_DOWN;
                    break;
                default:
                    break;
            };
        case WM_ERASEBKGND:
            return 1;               //We don't want to erase our background
        case WM_PAINT:              //Pull from our backbuffer, onto the screen
            BitBlt(WindowDC, 0, 0, WindowWidth, WindowHeight, backbuffer, 0, 0,SRCCOPY);
            ValidateRect(WindowHandle,NULL);
            break;
        case WM_DESTROY:
            exit(0);//A messy exit, but easy way to escape game loop
        default://If we didnt process a message, return the default value for it
            return DefWindowProcW(hWnd, Msg, wParam, lParam);
    };
    return 0;
};

//The following 3 methods use mem functions for fast drawing
inline void VertLineDIB(int x, int y, int y2,int thickness, unsigned char color)
{
    for (int j=y; j<y2; j++) memset(&dcbits[x+j*WindowWidth],color,thickness);
};
inline void HorzLineDIB(int x, int y, int x2,int thickness, unsigned char color)
{
    for (int j=y; j<y+thickness; j++) memset(&dcbits[x+j*WindowWidth],color,x2-x);
};
inline void FillRectDIB(int x, int y, int width, int height, unsigned char color)
{
    for (int j=y; j<y+height; j++) memset(&dcbits[x+j*WindowWidth],color,width);
};

void DrawWindow(WINDOW *win)
{
    int i,j,drawx,drawy;
    char tmp;	// following assumes char is signed
    for (j=0; j<win->height; j++){
        if (win->line[j].touched)
            for (i=0; i<win->width; i++){
                win->line[j].touched=false;
                drawx=((win->x+i)*fontwidth);
                drawy=((win->y+j)*fontheight);//-j;
                if (((drawx+fontwidth)<=WindowWidth) && ((drawy+fontheight)<=WindowHeight)){
                tmp = win->line[j].chars[i];	// \todo alternate data source here for tiles
                int FG = win->line[j].FG[i];
                int BG = win->line[j].BG[i];
                FillRectDIB(drawx,drawy,fontwidth,fontheight,BG);

				// \todo interpose tile drawing here
#if 0
				if (unsigned short t_index = win->line[j].tiles[i]) {

				}
#endif

                if ( tmp > 0){
                //if (tmp==95){//If your font doesnt draw underscores..uncomment
                //        HorzLineDIB(drawx,drawy+fontheight-2,drawx+fontwidth,1,FG);
                //    } else { // all the wa to here
                    int color = RGB(windowsPalette[FG].rgbRed,windowsPalette[FG].rgbGreen,windowsPalette[FG].rgbBlue);
                    SetTextColor(backbuffer,color);
                    ExtTextOut(backbuffer,drawx,drawy,0,NULL,&tmp,1,NULL);
                //    }     //and this line too.
                } else if (  tmp < 0 ) {
                    switch (tmp) {
                    case -60://box bottom/top side (horizontal line)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        break;
                    case -77://box left/right side (vertical line)
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        break;
                    case -38://box top left
                        HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                        break;
                    case -65://box top right
                        HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                        break;
                    case -39://box bottom right
                        HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight+1,2,FG);
                        break;
                    case -64://box bottom left
                        HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight+1,2,FG);
                        break;
                    case -63://box bottom north T (left, right, up)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+halfheight,2,FG);
                        break;
                    case -61://box bottom east T (up, right, down)
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        HorzLineDIB(drawx+halfwidth,drawy+halfheight,drawx+fontwidth,1,FG);
                        break;
                    case -62://box bottom south T (left, right, down)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy+halfheight,drawy+fontheight,2,FG);
                        break;
                    case -59://box X (left down up right)
                        HorzLineDIB(drawx,drawy+halfheight,drawx+fontwidth,1,FG);
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        break;
                    case -76://box bottom east T (left, down, up)
                        VertLineDIB(drawx+halfwidth,drawy,drawy+fontheight,2,FG);
                        HorzLineDIB(drawx,drawy+halfheight,drawx+halfwidth,1,FG);
                        break;
                    default:
                        // SetTextColor(DC,_windows[w].line[j].chars[i].color.FG);
                        // TextOut(DC,drawx,drawy,&tmp,1);
                        break;
                    }
                    };//switch (tmp)
                }//(tmp < 0)
            };//for (i=0;i<_windows[w].width;i++)
    };// for (j=0;j<_windows[w].height;j++)
    win->draw=false;                //We drew the window, mark it as so
};

//Check for any window messages (keypress, paint, mousemove, etc)
void CheckMessages()
{
    MSG msg;
    while (PeekMessage(&msg, 0 , 0, 0, PM_REMOVE)){
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
};

//***********************************
//Psuedo-Curses Functions           *
//***********************************

//Basic Init, create the font, backbuffer, etc
WINDOW *initscr(void)
{
   // _windows = new WINDOW[20];         //initialize all of our variables
    BITMAPINFO bmi;
    lastchar=-1;
    inputdelay=-1;
    std::string typeface;
	std::ifstream fin;
	fin.open("data\\FONTDATA");
 if (!fin.is_open()){
     MessageBox(WindowHandle, "Failed to open FONTDATA, loading defaults.", NULL, 0);
	 if (0 >= fontwidth) SetFontSize(8, 16);
 } else {
     getline(fin, typeface);
	 int tmp_width;
	 int tmp_height;

     fin >> tmp_width;
     fin >> tmp_height;
	 if (   (0 >= fontwidth && 4 >= tmp_width)
		 || (0 >= fontheight && 4 >= tmp_height)) {
         MessageBox(WindowHandle, "Invalid font size specified!", NULL, 0);
		 tmp_width  = 8;
		 tmp_height = 16;
     }
	 if (0 >= fontwidth) SetFontSize(tmp_width, tmp_height);
 }
    WinCreate();    //Create the actual window, register it, etc
    CheckMessages();    //Let the message queue handle setting up the window
    WindowDC = GetDC(WindowHandle);
    backbuffer = CreateCompatibleDC(WindowDC);
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = WindowWidth;
    bmi.bmiHeader.biHeight = -WindowHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount=8;
    bmi.bmiHeader.biCompression = BI_RGB;   //store it in uncompressed bytes
    bmi.bmiHeader.biSizeImage = WindowWidth * WindowHeight * 1;
    bmi.bmiHeader.biClrUsed=16;         //the number of colors in our palette
    bmi.bmiHeader.biClrImportant=16;    //the number of colors in our palette
	HBITMAP backbit = CreateDIBSection(0, &bmi, DIB_RGB_COLORS, (void**)&dcbits, NULL, 0);
    DeleteObject(SelectObject(backbuffer, backbit));//load the buffer into DC
	SetBkMode(backbuffer, TRANSPARENT);//Transparent font backgrounds

	haveCustomFont = (!typeface.empty() ? AddFontResourceExA("data\\termfont",FR_PRIVATE,NULL) : 0);
   if (0 < haveCustomFont){
    font = CreateFont(fontheight, fontwidth, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      ANSI_CHARSET, OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                      PROOF_QUALITY, FF_MODERN, typeface.c_str());   //Create our font

  } else {
      MessageBox(WindowHandle, "Failed to load default font, using FixedSys.", NULL, 0);
       font = CreateFont(fontheight, fontwidth, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                      ANSI_CHARSET, OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,
                      PROOF_QUALITY, FF_MODERN, "FixedSys");   //Create our font
   }
    //FixedSys will be user-changable at some point in time??
    SelectObject(backbuffer, font);//Load our font into the DC
//    WindowCount=0;

	// cf mapdata.h: typical value of SEEX/SEEY is 12 so the console is 25x25 display, 55x25 readout
    mainwin = newwin(25,80,0,0);
    return mainwin;   //create the 'stdscr' window and return its ref
};

WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x)
{
    int i,j;
    WINDOW *newwindow = new WINDOW;
    //newwindow=&_windows[WindowCount];
    newwindow->x=begin_x;
    newwindow->y=begin_y;
    newwindow->width=ncols;
    newwindow->height=nlines;
    newwindow->inuse=true;
    newwindow->draw=false;
    newwindow->BG=0;
    newwindow->FG=8;
    newwindow->cursorx=0;
    newwindow->cursory=0;
    newwindow->line = new curseline[nlines];

    for (j=0; j<nlines; j++) newwindow->line[j].init(ncols);
    //WindowCount++;
    return newwindow;
};


//Deletes the window and marks it as free. Clears it just in case.
int delwin(WINDOW *win)
{
    int j;
    win->inuse=false;
    win->draw=false;
    delete [] win->line;
    delete win;
    return 1;
};

inline int newline(WINDOW *win){
    if (win->cursory < win->height - 1){
        win->cursory++;
        win->cursorx=0;
        return 1;
    }
	return 0;
};

inline void addedchar(WINDOW *win){
    win->cursorx++;
    win->line[win->cursory].touched=true;
    if (win->cursorx > win->width)
        newline(win);
};


//Borders the window with fancy lines!
int wborder(WINDOW *win, chtype ls, chtype rs, chtype ts, chtype bs, chtype tl, chtype tr, chtype bl, chtype br)
{

    int i, j;
    int oldx=win->cursorx;//methods below move the cursor, save the value!
    int oldy=win->cursory;//methods below move the cursor, save the value!
    if (ls>0)
        for (j=1; j<win->height-1; j++)
            mvwaddch(win, j, 0, 179);
    if (rs>0)
        for (j=1; j<win->height-1; j++)
            mvwaddch(win, j, win->width-1, 179);
    if (ts>0)
        for (i=1; i<win->width-1; i++)
            mvwaddch(win, 0, i, 196);
    if (bs>0)
        for (i=1; i<win->width-1; i++)
            mvwaddch(win, win->height-1, i, 196);
    if (tl>0)
        mvwaddch(win,0, 0, 218);
    if (tr>0)
        mvwaddch(win,0, win->width-1, 191);
    if (bl>0)
        mvwaddch(win,win->height-1, 0, 192);
    if (br>0)
        mvwaddch(win,win->height-1, win->width-1, 217);
    //_windows[w].cursorx=oldx;//methods above move the cursor, put it back
    //_windows[w].cursory=oldy;//methods above move the cursor, put it back
    wmove(win,oldy,oldx);
    return 1;
};

//Refreshes a window, causing it to redraw on top.
int wrefresh(WINDOW *win)
{
    if (win==0) win=mainwin;
    if (win->draw)
        DrawWindow(win);
    return 1;
};

//Refreshes window 0 (stdscr), causing it to redraw on top.
int refresh(void)
{
    return wrefresh(mainwin);
};

//Not terribly sure how this function is suppose to work,
//but jday helped to figure most of it out
int getch(void)
{
 refresh();
 InvalidateRect(WindowHandle,NULL,true);
 lastchar=ERR;//ERR=-1
    if (inputdelay < 0)
    {
        do
        {
            CheckMessages();
            if (lastchar!=ERR) break;
            MsgWaitForMultipleObjects(0, NULL, FALSE, 50, QS_ALLEVENTS);//low cpu wait!
        }
        while (lastchar==ERR);
    }
    else if (inputdelay > 0)
    {
        unsigned long starttime=GetTickCount();
        unsigned long endtime;
        do
        {
            CheckMessages();        //MsgWaitForMultipleObjects won't work very good here
            endtime=GetTickCount(); //it responds to mouse movement, and WM_PAINT, not good
            if (lastchar!=ERR) break;
            Sleep(2);
        }
        while (endtime<(starttime+inputdelay));
    }
    else
    {
        CheckMessages();
    };
    Sleep(25);
    return lastchar;
};

//The core printing function, prints characters to the array, and sets colors
inline int printstring(WINDOW *win, char *fmt)
{
 int size = strlen(fmt);
 int j;
 for (j=0; j<size; j++){
  if (!(fmt[j]==10)){//check that this isnt a newline char
   if (win->cursorx <= win->width - 1 && win->cursory <= win->height - 1) {
    win->line[win->cursory].chars[win->cursorx]=fmt[j];
    win->line[win->cursory].FG[win->cursorx]=win->FG;
    win->line[win->cursory].BG[win->cursorx]=win->BG;
    win->line[win->cursory].touched=true;
    addedchar(win);
   } else
   return 0; //if we try and write anything outside the window, abort completely
} else // if the character is a newline, make sure to move down a line
  if (newline(win)==0){
      return 0;
      }
 }
 win->draw=true;
 return 1;
}

//Prints a formatted string to a window at the current cursor, base function
int wprintw(WINDOW *win, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char printbuf[2048];
    vsnprintf(printbuf, 2047, fmt, args);
    va_end(args);
    return printstring(win,printbuf);
};

//Prints a formatted string to a window, moves the cursor
int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char printbuf[2048];
    vsnprintf(printbuf, 2047, fmt, args);
    va_end(args);
    if (wmove(win,y,x)==0) return 0;
    return printstring(win,printbuf);
};

//Prints a formatted string to window 0 (stdscr), moves the cursor
int mvprintw(int y, int x, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char printbuf[2048];
    vsnprintf(printbuf, 2047, fmt, args);
    va_end(args);
    if (move(y,x)==0) return 0;
    return printstring(mainwin,printbuf);
};

//Prints a formatted string to window 0 (stdscr) at the current cursor
int printw(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    char printbuf[2078];
    vsnprintf(printbuf, 2047, fmt, args);
    va_end(args);
    return printstring(mainwin,printbuf);
};

//erases a window of all text and attributes
int werase(WINDOW *win)
{
    int j,i;
    for (j=0; j<win->height; j++)
    {
     for (i=0; i<win->width; i++)   {
     win->line[j].chars[i]=0;
     win->line[j].FG[i]=0;
     win->line[j].BG[i]=0;
     }
        win->line[j].touched=true;
    }
    win->draw=true;
    wmove(win,0,0);
    wrefresh(win);
    return 1;
};

//erases window 0 (stdscr) of all text and attributes
int erase(void)
{
    return werase(mainwin);
};

//pairs up a foreground and background color and puts it into the array of pairs
int init_pair(short pair, short f, short b)
{
    colorpairs[pair].FG=f;
    colorpairs[pair].BG=b;
    return 1;
};

//moves the cursor in a window
int wmove(WINDOW *win, int y, int x)
{
    if (x>=win->width)
     {return 0;}//FIXES MAP CRASH -> >= vs > only
    if (y>=win->height)
     {return 0;}// > crashes?
    if (y<0)
     {return 0;}
    if (x<0)
     {return 0;}
    win->cursorx=x;
    win->cursory=y;
    return 1;
};

//Clears windows 0 (stdscr)     I'm not sure if its suppose to do this?
int clear(void)
{
    return wclear(mainwin);
};

//Ends the terminal, destroy everything
int endwin(void)
{
    DeleteObject(font);
    WinDestroy();
    RemoveFontResourceExA("data\\termfont",FR_PRIVATE,NULL);//Unload it
    return 1;
};

//adds a character to the window
int mvwaddch(WINDOW *win, int y, int x, const chtype ch)
{
   if (wmove(win,y,x)==0) return 0;
   return waddch(win, ch);
};

//clears a window
int wclear(WINDOW *win)
{
    werase(win);
    wrefresh(win);
    return 1;
};

//gets the max x of a window (the width)
int getmaxx(WINDOW *win)
{
    if (win==0) return mainwin->width;     //StdScr
    return win->width;
};

//gets the max y of a window (the height)
int getmaxy(WINDOW *win)
{
    if (win==0) return mainwin->height;     //StdScr
    return win->height;
};

inline RGBQUAD BGR(int b, int g, int r)
{
    RGBQUAD result;
    result.rgbBlue=b;    //Blue
    result.rgbGreen=g;    //Green
    result.rgbRed=r;    //Red
    result.rgbReserved=0;//The Alpha, isnt used, so just set it to 0
    return result;
};

int start_color(void)
{
 colorpairs=new pairs[50];
 windowsPalette=new RGBQUAD[16];     //Colors in the struct are BGR!! not RGB!!
 windowsPalette[0]= BGR(0,0,0);
 windowsPalette[1]= BGR(0, 0, 196);
 windowsPalette[2]= BGR(0,196,0);
 windowsPalette[3]= BGR(30,180,196);
 windowsPalette[4]= BGR(196, 0, 0);
 windowsPalette[5]= BGR(180, 0, 196);
 windowsPalette[6]= BGR(200, 170, 0);
 windowsPalette[7]= BGR(196, 196, 196);
 windowsPalette[8]= BGR(96, 96, 96);
 windowsPalette[9]= BGR(64, 64, 255);
 windowsPalette[10]= BGR(0, 240, 0);
 windowsPalette[11]= BGR(0, 255, 255);
 windowsPalette[12]= BGR(255, 20, 20);
 windowsPalette[13]= BGR(240, 0, 255);
 windowsPalette[14]= BGR(255, 240, 0);
 windowsPalette[15]= BGR(255, 255, 255);
 return SetDIBColorTable(backbuffer, 0, 16, windowsPalette);
};

int keypad(WINDOW *faux, bool bf)
{
return 1;
};

int noecho(void)
{
    return 1;
};
int cbreak(void)
{
    return 1;
};
int keypad(int faux, bool bf)
{
    return 1;
};
int curs_set(int visibility)
{
    return 1;
};

int mvaddch(int y, int x, const chtype ch)
{
    return mvwaddch(mainwin,y,x,ch);
};

int wattron(WINDOW *win, int attrs)
{
    bool isBold = !!(attrs & A_BOLD);
    bool isBlink = !!(attrs & A_BLINK);
    int pairNumber = (attrs & A_COLOR) >> 17;
    win->FG=colorpairs[pairNumber].FG;
    win->BG=colorpairs[pairNumber].BG;
    if (isBold) win->FG += 8;
    if (isBlink) win->BG += 8;
    return 1;
};
int wattroff(WINDOW *win, int attrs)
{
     win->FG=8;                                  //reset to white
     win->BG=0;                                  //reset to black
    return 1;
};
int attron(int attrs)
{
    return wattron(mainwin, attrs);
};
int attroff(int attrs)
{
    return wattroff(mainwin,attrs);
};
int waddch(WINDOW *win, const chtype ch)
{
    char charcode;
    charcode=ch;

    switch (ch){        //LINE_NESW  - X for on, O for off
        case 4194424:   //#define LINE_XOXO 4194424
            charcode=179;
            break;
        case 4194417:   //#define LINE_OXOX 4194417
            charcode=196;
            break;
        case 4194413:   //#define LINE_XXOO 4194413
            charcode=192;
            break;
        case 4194412:   //#define LINE_OXXO 4194412
            charcode=218;
            break;
        case 4194411:   //#define LINE_OOXX 4194411
            charcode=191;
            break;
        case 4194410:   //#define LINE_XOOX 4194410
            charcode=217;
            break;
        case 4194422:   //#define LINE_XXOX 4194422
            charcode=193;
            break;
        case 4194420:   //#define LINE_XXXO 4194420
            charcode=195;
            break;
        case 4194421:   //#define LINE_XOXX 4194421
            charcode=180;
            break;
        case 4194423:   //#define LINE_OXXX 4194423
            charcode=194;
            break;
        case 4194414:   //#define LINE_XXXX 4194414
            charcode=197;
            break;
        default:
            charcode = (char)ch;
            break;
        }


int curx=win->cursorx;
int cury=win->cursory;

//if (win2 > -1){
   win->line[cury].chars[curx]=charcode;
   win->line[cury].FG[curx]=win->FG;
   win->line[cury].BG[curx]=win->BG;


    win->draw=true;
    addedchar(win);
    return 1;
  //  else{
  //  win2=win2+1;

};




//Move the cursor of windows 0 (stdscr)
int move(int y, int x)
{
    return wmove(mainwin,y,x);
};

//Set the amount of time getch waits for input
void timeout(int delay)
{
    inputdelay=delay;
};

#endif
