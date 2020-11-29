#if (defined _WIN32 || defined WINDOWS)

// requests W2K baseline Windows API ... unsure if this has link-time consequences w/posix_time.cpp which does not specify
#define _WIN32_WINNT 0x0500
#define WINVER 0x0500
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "catacurse.h"
#include "json.h"
#include "ui.h"
#include "options.h"
#include <fstream>
#include <string>
#include <map>

#if HAVE_MS_COM
#include <wincodec.h>
#include <locale>
#include <codecvt>
#include "Zaimoni.STL/Logging.h"
#include "Zaimoni.STL/MetaRAM.hpp"
#else
// lower-tech pathway that ectually exists on MingWin
#include <wingdi.h>
#include "Zaimoni.STL/cstdio"
#endif

#ifndef NDEBUG
#include <iostream>
#endif

#if HAVE_MS_COM
#define HAVE_TILES 1
#endif
// #define HAVE_TILES 1

#ifdef _MSC_VER
#pragma comment(lib,"msimg32")
#endif

// linking against output.cpp...duplicate this part of output.h
bool reject_not_whitelisted_printf(const std::string& src);

class OS_Window
{
private:
	static HWND _last;
	static HDC _last_dc;

	BITMAPINFOHEADER _backbuffer_stats;
	void (OS_Window::* _fillRect)(int x, int y, int w, int h, unsigned char c);
	RGBQUAD* _color_table;
	size_t _color_table_size;
	HWND _window;
	RECT _dim;
	HDC _dc;
	HDC _backbuffer;	// backbuffer for the physical device context
	HDC _staging;	// the current tile to draw would go here, for instance
	HGDIOBJ _staging_0;	// where the original bitmap for _staging goes
	unsigned char* _dcbits;	//the bits of the screen image, for direct access
public:
	static const HINSTANCE program;

	static const int TitleSize;
	static const int BorderWidth;
	static const int BorderHeight;
	static const int ScreenWidth;
	static const int ScreenHeight;

	OS_Window() {
		static_assert(std::is_standard_layout<OS_Window>::value, "OS_Window constructor is invalid");
		memset(this, 0, sizeof(OS_Window));
	};
	OS_Window(const OS_Window& src) = delete;
	OS_Window(OS_Window&& src) {
		static_assert(std::is_standard_layout<OS_Window>::value, "OS_Window move constructor is invalid");
		memmove(this, &src, sizeof(OS_Window));
		HBITMAP backbit = CreateDIBSection(0, (BITMAPINFO*)&_backbuffer_stats, DIB_RGB_COLORS, (void**)&_dcbits, nullptr, 0);	// _dcbits doesn't play nice w/move constructor; would have to rebuild this
		DeleteObject(SelectObject(_backbuffer, backbit));//load the buffer into DC
		SetBkMode(_backbuffer, TRANSPARENT);//Transparent font backgrounds
		memset((void*)&src, 0, sizeof(OS_Window));
	}
	~OS_Window() { destroy(); }

	OS_Window& operator=(const OS_Window& src) = delete;
	OS_Window& operator=(OS_Window&& src) {
		static_assert(std::is_standard_layout<OS_Window>::value, "OS_Window move assignment is invalid");
		destroy();
		memmove(this, &src, sizeof(OS_Window));
		HBITMAP backbit = CreateDIBSection(0, (BITMAPINFO*)&_backbuffer_stats, DIB_RGB_COLORS, (void**)&_dcbits, nullptr, 0);	// _dcbits doesn't play nice w/move constructor; would have to rebuild this
		DeleteObject(SelectObject(_backbuffer, backbit));//load the buffer into DC
		SetBkMode(_backbuffer, TRANSPARENT);//Transparent font backgrounds
		memset((void*)&src, 0, sizeof(OS_Window));
		return *this;
	};
	// lock defensible here
	OS_Window& operator=(HWND src)
	{
		if (_last == _window) _last = 0;
		clear();
		_window = src;
		if (!_window) return *this;
		ShowWindow(_window, SW_SHOW);
		CheckMessages();    //Let the message queue handle setting up the window
		_last = _window;
		if (!_backbuffer_stats.biSize) Bootstrap();
		if (!_backbuffer_stats.biBitCount) SetColorDepth(8, 16);
		SetBackbuffer();
		return *this;
	}

	operator HWND() const { return _window; }
	static HWND last() { return _last; }
	static HDC last_dc() { return _last_dc; }
	HDC dc() const { return _dc; }
	HDC backbuffer() const { return _backbuffer; }
	RGBQUAD color(size_t n) {
#ifndef NDEBUG
		if (_color_table_size <= n || !_color_table) throw std::logic_error("invalid color table access");
#endif
		return _color_table[n];
	}
	size_t color_table_size() const { return _color_table_size; }

	// these three are to draw tiles in a fairly high-level way
	void PrepareToDraw(HGDIOBJ src) {	// \todo should be taking an OS_Image object
		if (!_staging_0) _staging_0 = SelectObject(_staging, src);
		else SelectObject(_staging, src);
	}
	void UnpreparedToDraw() {
		if (_staging_0) SelectObject(_staging, _staging_0);
	}
	bool Draw(int xDest, int yDest, int wDest, int hDest, int xSrc, int ySrc, int wSrc, int hSrc)
	{
		static const BLENDFUNCTION alpha = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

		if (!_staging_0) return false;
		// \todo bounds-checking
		return AlphaBlend(_backbuffer, xDest, yDest, wDest, hDest, _staging, xSrc, ySrc, wSrc, hSrc, alpha);	// AlphaBlend equivalent, but requires unusual linking to msimg32.dll
	}

	void FillRect(int x, int y, int w, int h, unsigned char c)
	{
		if (_fillRect) {
			(this->*_fillRect)(x, y, w, h, c);
			return;
		}
		if (!_dcbits) throw std::logic_error("invalid call");
		switch (_backbuffer_stats.biBitCount)
		{
		case 8: _fillRect = &OS_Window::FillRect_8bit;
			break;
		case 32: _fillRect = &OS_Window::FillRect_32bit;
			break;
		default: throw std::logic_error("invalid call");
		}
	}

	void FillRect(int x, int y, int w, int h, unsigned char c, unsigned char alpha)
	{
		if (UCHAR_MAX == alpha) return FillRect(x, y, w, h, c);
		if (0 == alpha) return;
		if (!_dcbits) throw std::logic_error("invalid call");
		if (32 != _backbuffer_stats.biBitCount) throw std::logic_error("invalid call");
		FillRect_transparent(x, y, w, h, c, alpha);
	}

	int X() const { return _dim.left; };
	int Y() const { return _dim.top; };
	int width() const { return _dim.right - _dim.left; }
	int height() const { return _dim.bottom - _dim.top; }
	void center(int w, int h) {
		_dim.left = (OS_Window::ScreenWidth - w - OS_Window::BorderWidth) / 2;
		if (0 > _dim.left) _dim.left = 0;
		_dim.top = (OS_Window::ScreenHeight - h - OS_Window::BorderHeight - OS_Window::TitleSize) / 2;
		if (0 > _dim.top) _dim.top = 0;
		_dim.right = _dim.left + w;
		_dim.bottom = _dim.top + h;
		// arguably this should trigger update even if backbuffer is present
		if (!_backbuffer) {
			Bootstrap(_backbuffer_stats);
			if (_backbuffer_stats.biBitCount) SetImageSize(_backbuffer_stats); // if we're zero-initialized, we don't know our color depth yet
		}
	}

	void clear() {
		if (_staging_0 && SelectObject(_staging, _staging_0)) _staging_0 = 0;
		if (_staging && DeleteDC(_staging)) _staging = 0;
		if (_backbuffer && DeleteDC(_backbuffer)) _backbuffer = 0;
		_dcbits = 0;
		if (_last_dc == _dc) _last_dc = 0;
		if (_dc && ReleaseDC(_window, _dc)) _dc = 0;
		if (_last == _window) _last = 0;
		if (_window && DestroyWindow(_window)) _window = 0;
	}

	void Bootstrap(BITMAPINFOHEADER& dest)
	{
		dest.biSize = sizeof(BITMAPINFOHEADER);
		dest.biWidth = width();
		dest.biHeight = -height();
		dest.biPlanes = 1;
		dest.biCompression = BI_RGB;   //store it in uncompressed bytes
	}

	void Bootstrap() { Bootstrap(_backbuffer_stats); }

	static void SetColorDepth(BITMAPINFOHEADER& dest, unsigned char bits, unsigned long important_colors = 0)
	{
		dest.biBitCount = bits;
		SetImageSize(dest);
		switch (bits)
		{
		case 32:
			dest.biClrUsed = 0;
			dest.biClrImportant = 0;
			break;
		case 8:
			if (1 > important_colors || (UCHAR_MAX + 1) < important_colors) throw std::logic_error("color table has unreasonable number of entries");
			dest.biClrUsed = important_colors;
			dest.biClrImportant = important_colors;
			break;
		default: throw std::logic_error("unsupported color depth");
		}
	}

	void SetColorDepth(unsigned char bits, unsigned long important_colors = 0) { SetColorDepth(_backbuffer_stats, bits, important_colors); }

	bool SetBackbuffer(const BITMAPINFO& buffer_spec)
	{
		if (!_window) return false;
		if (!_dc && !(_dc = GetDC(_window))) return false;
		_last_dc = _dc;
		if (!_backbuffer && !(_backbuffer = CreateCompatibleDC(_dc))) return false;
		if (!_staging && !(_staging = CreateCompatibleDC(_dc))) return false;
		_backbuffer_stats = buffer_spec.bmiHeader;
		// handle invariant parts here
		Bootstrap(_backbuffer_stats);
		SetImageSize(_backbuffer_stats);
		// we only handle 8 and 32 bits
		if (32 == _backbuffer_stats.biBitCount) {
			_backbuffer_stats.biClrUsed = 0;
			_backbuffer_stats.biClrImportant = 0;
		}
		else if (8 == _backbuffer_stats.biBitCount) {
			// reality checks
			SUCCEED_OR_DIE(_backbuffer_stats.biClrUsed == _backbuffer_stats.biClrImportant);
			SUCCEED_OR_DIE(1 <= _backbuffer_stats.biClrUsed && (UCHAR_MAX + 1) >= _backbuffer_stats.biClrUsed);
		}
		else SUCCEED_OR_DIE(!"unhandled color depth");

		HBITMAP backbit = CreateDIBSection(0, (BITMAPINFO*)&_backbuffer_stats, DIB_RGB_COLORS, (void**)&_dcbits, nullptr, 0);	// _dcbits doesn't play nice w/move constructor; would have to rebuild this
		DeleteObject(SelectObject(_backbuffer, backbit));//load the buffer into DC
		SetBkMode(_backbuffer, TRANSPARENT);//Transparent font backgrounds
		if (8 >= _backbuffer_stats.biBitCount && _color_table && (1ULL << _backbuffer_stats.biBitCount) >= _color_table_size) return SetDIBColorTable(_backbuffer, 0, _color_table_size, _color_table);	// actually need this
		return true;
	}

	bool SetBackbuffer()
	{
		BITMAPINFO working;
		memset(&working, 0, sizeof(working));
		working.bmiHeader = _backbuffer_stats;
		return SetBackbuffer(working);
	}

	static RGBQUAD* CreateNewColorTable(size_t n) { return zaimoni::_new_buffer_nonNULL_throws<RGBQUAD>(n); }
	bool SetColorTable(RGBQUAD*& colors, size_t n)	// probably should capture the color table instead and use it as a translation device
	{
		if (_color_table) free(_color_table);
		_color_table = colors;
		_color_table_size = n;
		colors = 0;
		if (8 >= _backbuffer_stats.biBitCount && (1ULL << _backbuffer_stats.biBitCount) >=n) return SetDIBColorTable(_backbuffer, 0, n, _color_table);	// actually need this
		return true;
	}

	static void CheckMessages()	//Check for any window messages (keypress, paint, mousemove, etc)
	{
		MSG msg;
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	};

	bool Resize(int new_color_depth = 0)
	{
		RECT tmp;
		if (!GetWindowRect(_window, &tmp)) return false;
		if (!MoveWindow(_window, _dim.left, _dim.top, width() + OS_Window::BorderWidth, height() + OS_Window::BorderHeight + OS_Window::TitleSize, true)) return false;	// this does not handle the backbuffer bitmap
		// problem...probably have to undo both the backbuffer and its device context
		if (_backbuffer && DeleteDC(_backbuffer)) _backbuffer = 0;
		if (_last_dc == _dc) _last_dc = 0;
		if (_dc && ReleaseDC(_window, _dc)) _dc = 0;
		_dcbits = 0;

		if (new_color_depth && new_color_depth != _backbuffer_stats.biBitCount) SetColorDepth(_backbuffer_stats, new_color_depth);
		return SetBackbuffer();
	}
private:
	void destroy() { 
		clear();
		free(_color_table);
		_color_table = 0;
	}

	static void SetImageSize(BITMAPINFOHEADER& dest)
	{
		const unsigned long w = (0 < dest.biWidth ? dest.biWidth : -dest.biWidth);
		const unsigned long h = (0 < dest.biHeight ? dest.biHeight : -dest.biHeight);
		switch (dest.biBitCount)
		{
		case 32:
			dest.biSizeImage = 4 * w * h;
			break;
		case 8:
			dest.biSizeImage = w * h;
			break;
		default: throw std::logic_error("unsupported color depth");
		}
	}

	// note non-use of HBRUSH
	void FillRect_8bit(int x, int y, int w, int h, unsigned char c)
	{
		for (int j = y; j<y + h; j++) memset(_dcbits+(x + j *width()), c, w);
	};
	void FillRect_32bit(int x, int y, int w, int h, unsigned char c)
	{
		const RGBQUAD rgba = color(c);
		for (int j = y; j < y + h; j++) {
			RGBQUAD* const dest = reinterpret_cast<RGBQUAD*>(_dcbits + sizeof(RGBQUAD)*(x + j * width()));
			for (int i = 0; i < w; i++) dest[i] = rgba;
		}
	};
	void FillRect_transparent(int x, int y, int w, int h, unsigned char c,unsigned char alpha)
	{
		const RGBQUAD rgba = color(c);
		const unsigned short red_in = (unsigned short)rgba.rgbRed*(unsigned short)alpha;
		const unsigned short blue_in = (unsigned short)rgba.rgbBlue*(unsigned short)alpha;
		const unsigned short green_in = (unsigned short)rgba.rgbGreen*(unsigned short)alpha;
		const unsigned short inv_alpha = UCHAR_MAX - alpha;
		for (int j = y; j < y + h; j++) {
			RGBQUAD* const dest = reinterpret_cast<RGBQUAD*>(_dcbits + sizeof(RGBQUAD)*(x + j * width()));
			for (int i = 0; i < w; i++)
				{
				dest[i].rgbBlue = (unsigned char)((blue_in + inv_alpha * (unsigned short)(dest[i].rgbBlue)) / UCHAR_MAX);
				dest[i].rgbGreen = (unsigned char)((green_in + inv_alpha * (unsigned short)(dest[i].rgbGreen)) / UCHAR_MAX);
				dest[i].rgbRed = (unsigned char)((red_in + inv_alpha * (unsigned short)(dest[i].rgbRed)) / UCHAR_MAX);
				}
		}
	};
};

static const auto& OPTIONS = option_table::get();

#if _MSC_VER
// MSVC is empirically broken (following is correct for _MSC_VER 1913 ... 1914)
#define BORDERWIDTH_SCALE (5+3*OPTIONS[OPT_EXTRA_MARGIN])
#define BORDERHEIGHT_SCALE (5+3*OPTIONS[OPT_EXTRA_MARGIN])
#else
// assume we are on a working WinAPI (e.g., MingWin)
#define BORDERWIDTH_SCALE (2+3*OPTIONS[OPT_EXTRA_MARGIN])
#define BORDERHEIGHT_SCALE (2+3*OPTIONS[OPT_EXTRA_MARGIN])
#endif
HWND OS_Window::_last = 0;	// the most recently created, valid window
HDC OS_Window::_last_dc = 0;	// the most recently created, valid physical device context
const HINSTANCE OS_Window::program = GetModuleHandle(0);	// available from WinMain as well

const int OS_Window::TitleSize = GetSystemMetrics(SM_CYCAPTION);      //These lines ensure
const int OS_Window::BorderWidth = GetSystemMetrics(SM_CXDLGFRAME) * BORDERWIDTH_SCALE;  //that our window will
const int OS_Window::BorderHeight = GetSystemMetrics(SM_CYDLGFRAME) * BORDERHEIGHT_SCALE; // be a perfect size
const int OS_Window::ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
const int OS_Window::ScreenHeight = GetSystemMetrics(SM_CYSCREEN);
#undef BORDERWIDTH_SCALE
#undef BORDERHEIGHT_SCALE

// we allow move construction/assignment but not copy construction/assignment
class OS_Image
{
private:
	HANDLE _x;
	BITMAPINFOHEADER _data;
	mutable RGBQUAD* _pixels;
	bool _have_info;
public:
	// XXX LoadImage only works for *BMP.  SetDIBits required for JPEG and PNG, but that requires an HDC
	OS_Image() {
		static_assert(std::is_standard_layout<OS_Window>::value, "OS_Window constructor is invalid");
		memset(this, 0, sizeof(OS_Image));
	}
	OS_Image(const char* src) {
		static_assert(std::is_standard_layout<OS_Window>::value, "OS_Window filename constructor is invalid");
		memset(this, 0, sizeof(OS_Image));
		_x = loadFromFile(src, _pixels, _data, _have_info); init();
		assert(!_x || 0 < width());
		assert(!_x || 0 < height());
	}
	// OS_Image(const wchar_t* src)
	OS_Image(const OS_Image& src) = delete;
	OS_Image(OS_Image&& src) {
		static_assert(std::is_standard_layout<OS_Window>::value, "OS_Window move constructor is invalid");
		memmove(this,&src,sizeof(OS_Image));
		memset((void*)&src, 0, sizeof(OS_Image));
	}
	// clipping constructor
	OS_Image(const OS_Image& src, const size_t origin_x, const size_t origin_y, const size_t width, const size_t height) {
		SUCCEED_OR_DIE(0 < width);
		SUCCEED_OR_DIE(0 < height);
		SUCCEED_OR_DIE(((size_t)(-1)/sizeof(_pixels[0]))/width >= height);
		SUCCEED_OR_DIE(src.width() > origin_x);
		SUCCEED_OR_DIE(src.height() > origin_y);
		SUCCEED_OR_DIE(src.width() - origin_x >= width);
		SUCCEED_OR_DIE(src.height() - origin_y >= height);
		static_assert(std::is_standard_layout<OS_Window>::value, "OS_Window filename constructor is invalid");
		memset(this, 0, sizeof(OS_Image));
		_data = src._data;
		RGBQUAD* const incoming = src.pixels();

		_data.biWidth = width;
//		_data.biHeight = -((signed long)height);
		_data.biHeight = height;
		_pixels = zaimoni::_new_buffer_nonNULL_throws<RGBQUAD>(width*height);

		// would like static assertion here but not needed
		for (size_t scan_y = 0; scan_y < height; scan_y++) {
			memmove(_pixels + (scan_y*width), incoming + ((scan_y + origin_y)*src.width()+origin_x), sizeof(RGBQUAD)*width);
		}
		decltype(auto) local_dc = GetDC(nullptr); // \todo? std::unique_ptr with custom deleter?
		_x = CreateCompatibleBitmap(local_dc, width, height);
		if (!_x) {
			ReleaseDC(nullptr, local_dc);
			free(_pixels);
			throw std::bad_alloc();
		}
		if (!SetDIBits(local_dc, (HBITMAP)_x, 0, height, _pixels, (LPBITMAPINFO)(&_data), DIB_RGB_COLORS)) {
			ReleaseDC(nullptr, local_dc);
			free(_pixels);
			throw std::bad_alloc();
		}
		ReleaseDC(nullptr, local_dc);
		_have_info = true;
	}

	~OS_Image() { clear(); }

	OS_Image& operator=(const OS_Image& src) = delete;
	OS_Image& operator=(OS_Image&& src)
	{
		static_assert(std::is_standard_layout<OS_Window>::value, "OS_Window move constructor is invalid");
		clear();
		memmove(this, &src, sizeof(OS_Image));
		memset((void*)&src, 0, sizeof(OS_Image));
		return *this;
	}

	HANDLE handle() const { return _x; }
	size_t width() const { return _data.biWidth; }
	size_t height() const { return 0 < _data.biHeight ? _data.biHeight : -_data.biHeight; }

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

	bool tint(const RGBQUAD rgba, unsigned char alpha)
	{
		if (0 == alpha) return true;
		SUCCEED_OR_DIE(32 == _data.biBitCount);	// invariant failure
		RGBQUAD* const dest = pixels();
		if (!dest) throw std::bad_alloc();
		SUCCEED_OR_DIE(UCHAR_MAX != alpha);	// not what we're supposed to do here
		const unsigned short red_in = (unsigned short)rgba.rgbRed*(unsigned short)alpha;
		const unsigned short blue_in = (unsigned short)rgba.rgbBlue*(unsigned short)alpha;
		const unsigned short green_in = (unsigned short)rgba.rgbGreen*(unsigned short)alpha;
		const unsigned short inv_alpha = UCHAR_MAX - alpha;
		size_t i = width()*height();
		do	{
			--i;
			if (0 == dest[i].rgbReserved) continue;	// ignore transparent pixels
			dest[i].rgbBlue = (unsigned char)((blue_in + inv_alpha * (unsigned short)(dest[i].rgbBlue)) / UCHAR_MAX);
			dest[i].rgbGreen = (unsigned char)((green_in + inv_alpha * (unsigned short)(dest[i].rgbGreen)) / UCHAR_MAX);
			dest[i].rgbRed = (unsigned char)((red_in + inv_alpha * (unsigned short)(dest[i].rgbRed)) / UCHAR_MAX);
			}
		while(0 < i);
		return SetDIBits(OS_Window::last_dc(), (HBITMAP)_x, 0, height(), dest, (BITMAPINFO*)(&_data), DIB_RGB_COLORS);
	}

	bool overlay(const OS_Image& src)
	{
	SUCCEED_OR_DIE(32 == _data.biBitCount);	// invariant failure
	SUCCEED_OR_DIE(width() == src.width());	// invariant failure
	SUCCEED_OR_DIE(height() == src.height());	// invariant failure
	RGBQUAD* const dest = pixels();
	const RGBQUAD* const src_px = src.pixels();
	size_t i = width()*height();
	do {
		--i;
		if (0 == src_px[i].rgbReserved) continue;	// ignore transparent pixels
		if (UCHAR_MAX == src_px[i].rgbReserved) {	// opaque: copy
			dest[i] = src_px[i];
			continue;
		}
		const unsigned short inv_alpha = 255 - src_px[i].rgbReserved;
		dest[i].rgbBlue = (unsigned char)((src_px[i].rgbReserved*(unsigned short)(src_px[i].rgbBlue) + inv_alpha * (unsigned short)(dest[i].rgbBlue)) / UCHAR_MAX);
		dest[i].rgbGreen = (unsigned char)((src_px[i].rgbReserved*(unsigned short)(src_px[i].rgbGreen) + inv_alpha * (unsigned short)(dest[i].rgbGreen)) / UCHAR_MAX);
		dest[i].rgbRed = (unsigned char)((src_px[i].rgbReserved*(unsigned short)(src_px[i].rgbRed) + inv_alpha * (unsigned short)(dest[i].rgbRed)) / UCHAR_MAX);
		dest[i].rgbReserved = UCHAR_MAX - (inv_alpha*(unsigned short)(UCHAR_MAX- dest[i].rgbReserved)) / UCHAR_MAX;
	} while (0 < i);
	return SetDIBits(OS_Window::last_dc(), (HBITMAP)_x, 0, height(), dest, (BITMAPINFO*)(&_data), DIB_RGB_COLORS);
	}
private:
	void init() {
		if (_x && !_have_info) {
			memset(&_data, 0, sizeof(_data));
			_data.biSize = sizeof(_data);
			_have_info = GetDIBits(OS_Window::last_dc(), (HBITMAP)_x, 0, 0, nullptr, (LPBITMAPINFO)(&_data), DIB_RGB_COLORS);	// XXX needs testing, may need an HDC
			_data.biCompression = BI_RGB;
			_data.biHeight = (0 < _data.biHeight) ? -_data.biHeight : _data.biHeight;	// force bottom-up
			OS_Window::SetColorDepth(_data,32);	// force alpha channel in (no padding bytes)
		}
	}

	RGBQUAD* pixels() const
	{
		if (_pixels) return _pixels;
		if (!_have_info) return 0;
		RGBQUAD* const tmp = zaimoni::_new_buffer<RGBQUAD>(width()*height());
		if (!tmp) return 0;
		if (!GetDIBits(OS_Window::last_dc(), (HBITMAP)_x, 0, 0, nullptr, (LPBITMAPINFO)(&_data), DIB_RGB_COLORS))	// may need an HDC
			{
			free(tmp);
			return 0;
			}
		return _pixels = tmp;
	}

	// header: 8 bytes; each chunk is strictly bounded below by 12 bytes and three are required.
	static bool LooksLikePNGHeader(const unsigned char* src, size_t len, long& incoming_width, long& incoming_height)
	{
		if (!src || 24 > len) return false;	// not correct for a PNG-class file parser.
		// Cf https://en.wikipedia.org/wiki/Portable_Network_Graphics
		// the network-order/big-endian 4-byte width and height are at offsets 16 and 20
		static_assert(0x50 == 'P', "source code encoding not close to ASCII");
		static_assert(0x4E == 'N', "source code encoding not close to ASCII");
		static_assert(0x47 == 'G', "source code encoding not close to ASCII");
#if REFERENCE
		if (!(0x89 == src[0]	// high-bit set: binary file, 
			&& 'P' == src[1]	// ASCII: P
			&& 'N' == src[2]	// N
			&& 'G' == src[3]	// G
			&& 0x0D == src[4]	// DOS CR-LF (can be trashed by text-mode FTP)
			&& 0x0A == src[5]
			&& 0x1A == src[6]	// DOS EOF character, stops TYPE
			&& 0x0A == src[7]))	// UNIX LF (can be trashed by text-mode FTP)
			return false;
#else
		if (strncmp(reinterpret_cast<const char*>(src), "\x89PNG\x0D\x0A\x1A\x0A", 8)) return false;
#endif
		// should be an IHDR chunk immediately afterwards
		if (strncmp(reinterpret_cast<const char*>(src+12), "IHDR", 4)) return false;
		static_assert(256 == (UCHAR_MAX + 1), "hard to read PNG on not-8-bit hardware");
		incoming_width = (((src[16] * 256) + src[17]) * 256 + src[18]) * 256 + src[19];
		incoming_height = (((src[20] * 256) + src[21]) * 256 + src[22]) * 256 + src[23];
		return true;
	}

	static bool LooksLikeBMPFileHeader(const unsigned char* src, size_t len)
	{
		if (!src || 24 > len) return false;
		// Cf https://en.wikipedia.org/wiki/BMP_file_format
		static_assert(0x42 == 'B', "source code encoding not close to ASCII");
		static_assert(0x4D == 'M', "source code encoding not close to ASCII");
		return (   'B' == src[0]	// ASCII: B
			    && 'M' == src[1]);	// M
	}

	static bool LooksLikeJPEGHeader(const unsigned char* src, size_t len)
	{
		if (!src || 14 > len) return false;
		// Cf https://en.wikipedia.org/wiki/JPEG
		return (   0xFF == src[0]	// Start of Image (SOI) block
			    && 0xD8 == src[1]);
	}

	// buffer is owned by the caller
	static bool Get32BRGAbuffer(const std::string src,  RGBQUAD*& buffer, unsigned int& width, unsigned int& height)
	{	// this implementation requires COM classes
#if HAVE_MS_COM
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > widen;	// 2019-11-01: MSVC++ indicates std::wstring_convert was deprecated in C++17
		const auto w_src = widen.from_bytes(src);

		// plan B
		// Create a decoder
		// XXX research how to not leak these COM objects
		IWICImagingFactory* factory = nullptr;
		HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
		if (!SUCCEEDED(hr)) return false;

		IWICBitmapDecoder* parser = 0;
		hr = factory->CreateDecoderFromFilename(w_src.c_str(), nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &parser);
		if (!SUCCEEDED(hr)) {
			factory->Release();
			return false;
		}

		// Retrieve the first frame of the image from the decoder
		IWICBitmapFrameDecode *pFrame = nullptr;
		hr = parser->GetFrame(0, &pFrame);
		if (!SUCCEEDED(hr)) {
			parser->Release();
			factory->Release();
			return false;
		}

		hr = pFrame->GetSize(&width, &height);
		if (!SUCCEEDED(hr)) {
			pFrame->Release();
			parser->Release();
			factory->Release();
			return false;
		}
		if ((size_t)(-1) / 4 <= width) {
			pFrame->Release();
			parser->Release();
			factory->Release();
			return false;
		}
		if (((size_t)(-1) / 4) / width < height) {
			pFrame->Release();
			parser->Release();
			factory->Release();
			return false;
		}

		IWICFormatConverter* conv = nullptr;
		hr = factory->CreateFormatConverter(&conv);
		if (!SUCCEEDED(hr)) {
			pFrame->Release();
			parser->Release();
			factory->Release();
			return 0;
		}
		hr = conv->Initialize(pFrame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
		if (!SUCCEEDED(hr)) {
			conv->Release();
			pFrame->Release();
			parser->Release();
			factory->Release();
			return false;
		}
		// want target format to be GUID_WICPixelFormat32bppBGRA
		static_assert(4 == sizeof(RGBQUAD), "nonstandard definition of RGBQUAD");
		const size_t buf_len = sizeof(RGBQUAD) * width * height;
		buffer = zaimoni::_new_buffer<RGBQUAD>(width * height);
		if (!buffer) {
			conv->Release();
			pFrame->Release();
			parser->Release();
			factory->Release();
			return false;
		}

		hr = conv->CopyPixels(nullptr, 4 * width, buf_len, reinterpret_cast<unsigned char*>(buffer));
		if (!SUCCEEDED(hr)) {
			free(buffer);
			buffer = 0;
			conv->Release();
			pFrame->Release();
			parser->Release();
			factory->Release();
			return false;
		}

		conv->Release();
		pFrame->Release();
		parser->Release();
		factory->Release();
		return true;
#else
		return false;
#endif
	}

	static HANDLE loadFromFile(const std::string src, RGBQUAD*& pixels, BITMAPINFOHEADER& working,bool& got_stats)
	{
		pixels = nullptr;
		got_stats = false;
		memset(&working, 0, sizeof(working));
		working.biSize = sizeof(working);
		working.biPlanes = 1;

#if 1
		unsigned int native_width = 0;
		unsigned int native_height = 0;
		if (!Get32BRGAbuffer(src, pixels, native_width, native_height)) return 0;

		working.biCompression = BI_RGB;
		working.biWidth = native_width;
		working.biHeight = native_height;	// both JPEG and PNG are bottom-up
		working.biBitCount = 32;
		working.biSizeImage = sizeof(RGBQUAD)*native_width*native_height;	// both PNG and JPEG use image buffer size

		decltype(auto) local_dc = GetDC(nullptr);
		HBITMAP tmp = CreateCompatibleBitmap(local_dc, native_width, native_height);
		if (!tmp) {
			free(pixels); pixels = 0;
			return 0;
		}
		got_stats = SetDIBits(local_dc, tmp, 0, native_height, pixels, (BITMAPINFO*)(&working), DIB_RGB_COLORS);
		ReleaseDC(nullptr, local_dc);
		if (got_stats) return tmp;
		DeleteObject(tmp);
		return 0;
#endif

#if PROTOTYPE
		HANDLE ret = LoadImageA(OS_Window::program, src.c_str(), IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
		if (ret) return ret;	// was a fully legal BMP for the current version of Windows (untested)

		// Plan C
		// doesn't look like a bitmap...try SetDIBits instead
		unsigned char* raw_image = 0;	// \todo ideally would be wrapped in an object
		size_t raw_image_size = 0;
		if (!zaimoni::GetBinaryFileImage(src.c_str(), raw_image, raw_image_size)) {
			free(raw_image);
			return 0;
		}
		if (LooksLikeBMPFileHeader(raw_image, raw_image_size))
			{
			free(raw_image); raw_image = 0;
			return 0;	// in theory we could try to recover but we know format is "off"
			}
		long incoming_width = 0;	// XXX need image dimensions from somewhere or else \todo fix
		long incoming_height = 0;

		if (LooksLikePNGHeader(raw_image, raw_image_size, incoming_width, incoming_height)) working.biCompression = BI_PNG;
		else if (LooksLikeJPEGHeader(raw_image, raw_image_size)) working.biCompression = BI_JPEG;
		else {
			free(raw_image); raw_image = 0;
			return 0;
		}
		working.biWidth = incoming_width;
		working.biHeight = incoming_height;	// both JPEG and PNG are bottom-up
		working.biBitCount = 0;	// JPEG and PNG manage this themselves
		working.biSizeImage = raw_image_size;	// both PNG and JPEG use image buffer size
		// image dimensions must be accurate
		HBITMAP tmp = CreateCompatibleBitmap(OS_Window::last_dc(), incoming_width, incoming_height);
#ifndef NDEBUG
		std::cerr << src << "; " << incoming_width << " x " << incoming_height << "; " << tmp << '\n';
#endif
		if (!tmp) {
			free(raw_image); raw_image = 0;
			return 0;
		}
		bool test = SetDIBits(OS_Window::last_dc(),tmp,0, incoming_height,raw_image,(BITMAPINFO*)(&working),DIB_RGB_COLORS);	// DeonApocalypse fails here
		free(raw_image); raw_image = 0;
#ifndef NDEBUG
		std::cerr << test << '\n';
#endif
		if (test) return tmp;
		else {
			DeleteObject(tmp);
			return 0;
		}
#endif
	}
};

// tiles support
std::map<unsigned short, OS_Image> _cache;
std::map<std::string, unsigned short> _translate;	// const char* won't work -- may need to store base names seen first time with a rotation specifier

// tilesheet support
std::map<std::string, OS_Image> _tilesheets;
std::map<std::string, int > _tilesheet_tile;

// globals used by the main curses simulation
// accessors intended to allow computing accurate dimensions before physically creating the window
static OS_Window& offscreen()
{
	static OS_Window ooao;
	return ooao;
}

bool WinCreate(OS_Window& _win);
static OS_Window& win()
{
	static bool initialized = false;
	if (!initialized) { // C++20 [[unlikely]]
		initialized = true;
		WinCreate(offscreen());
	}
	return offscreen();
}

int fontwidth = 0;          //the width of the font, background is always this size
int fontheight = 0;         //the height of the font, background is always this size
int halfwidth = 0;          //half of the font width, used for centering lines
int halfheight = 0;          //half of the font height, used for centering lines

bool SetFontSize(const int x, const int y)
{
	if (4 >= x) return false;	// illegible
	if (4 >= y) return false;	// illegible
	if (x == fontwidth && y == fontheight) return false;	// no-op
	fontwidth = x;
	fontheight = y;
	halfwidth = fontwidth / 2;
	halfheight = fontheight / 2;

	option_table::get().set_screen_options(getmaxx(nullptr), getmaxy(nullptr));

	offscreen().center(SCREEN_WIDTH * fontwidth, VIEW * fontheight);
	return true;
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
	return std::string(test+1);
}

static const char* JSON_transcode_color[] = {
	"BLACK",
	"RED",
	"GREEN",
	"YELLOW",
	"BLUE",
	"MAGENTA",
	"CYAN",
	"WHITE",
	"BOLD BLACK",
	"BOLD RED",
	"BOLD GREEN",
	"BOLD YELLOW",
	"BOLD BLUE",
	"BOLD MAGENTA",
	"BOLD CYAN",
	"BOLD WHITE",
};

bool parse_JSON_color(const char* src, int& color)
{
	size_t i = sizeof(JSON_transcode_color) / sizeof(*JSON_transcode_color);
	do {
		--i;
		if (!strcmp(src, JSON_transcode_color[i])) {
			color = i;
			return true;
		}
	}
	while(0<i);
	return false;
}

bool load_tile(const char* src)
{
	static unsigned short _next = 0;

	if (!src || !src[0] || ':' == src[0]) return false;	// nothing to load
	if ((unsigned short)(-1) == _next) return false;	// at implementation limit
	if (_translate.count(src)) return true;	// already loaded
	const char* const has_rotation_specification = strchr(src, ':');
	if (has_rotation_specification)
		{	// reject pathological syntax now
		std::string test(src);
		if (':' == test.back() || std::string::npos != test.find("::")) return false;
		}

	std::string base_tile(has_rotation_specification ? std::string(src, 0, has_rotation_specification-src) : src);
	if (!_translate.count(base_tile)) {
		const char* const is_from_tilesheet = strchr(base_tile.c_str(), '#');
		if (is_from_tilesheet) {
			std::string tilesheet(base_tile.c_str(), 0, is_from_tilesheet - base_tile.c_str());
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
			if (scaled_y <= index_y) return false;
			const int index_x = index - index_y * scaled_x;
			OS_Image image(src, dim*index_x, dim*index_y, dim, dim);	// clipping constructor
			if (!image.handle()) return false;	// failed to load
			_translate[base_tile] = ++_next;
			_cache[_next] = std::move(image);
		} else {
			OS_Image image(base_tile.c_str());
			if (!image.handle()) return false;	// failed to load
			_translate[base_tile] = ++_next;
			_cache[_next] = std::move(image);
		}
		if (SetFontSize(16, 16)) {
			OS_Window& _win = offscreen();
			if (_win) _win.Resize(32);
			else _win.SetColorDepth(32);
		}
	}
//	if (!has_rotation_specification) return true;
	if (!has_rotation_specification) return _translate.count(src);
	// use the rotation specifier.  For now, assume only one specifier total (one of tint, rotation, or background)
	{
	int color_code = -1;
	if (parse_JSON_color(has_rotation_specification+1, color_code))
		{	// this triggers an alpha-transparent tint
		const OS_Image& tmp = _cache[_translate[base_tile]];
		OS_Image working(tmp, 0, 0, tmp.width(), tmp.height());
//		need to apply color as alpha-transparent tint only to pixels that already exist
		working.tint(offscreen().color(color_code),UCHAR_MAX/2);
		_translate[src] = ++_next;
		_cache[_next] = std::move(working);
		return true;
	}
	}
	// \todo rotation case
	// background tile case
	if (cataclysm::JSON::cache.count("tiles") && cataclysm::JSON::cache["tiles"].has_key(has_rotation_specification + 1)) {
		// tile requested as background has been configured
		const std::string& bg = cataclysm::JSON::cache["tiles"][has_rotation_specification + 1].scalar();
		if (!load_tile(bg.c_str())) return false;
		SUCCEED_OR_DIE(_translate.count(bg.c_str()));
		const OS_Image& background = _cache[_translate[bg.c_str()]];
		OS_Image working(background, 0, 0, background.width(), background.height());
		working.overlay(_cache[_translate[base_tile]]);
		_translate[src] = ++_next;
		_cache[_next] = std::move(working);
		return true;
	}
	return false;
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
#if HAVE_TILES
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
	chars = zaimoni::_new_buffer_nonNULL_throws<char>(ncols);
	FG = zaimoni::_new_buffer_nonNULL_throws<char>(ncols);
	BG = zaimoni::_new_buffer_nonNULL_throws<char>(ncols);
#if HAVE_TILES
	background_tiles = zaimoni::_new_buffer_nonNULL_throws<unsigned short>(ncols);
	tiles = zaimoni::_new_buffer_nonNULL_throws<unsigned short>(ncols);
#endif
	touched = true;
}

curseline::~curseline()
{	// this should not be on a critical path; be nice to archaic, not-remotely-ISO compilers that splat on free(NULL)
	if (chars) { free(chars); chars = 0; }
	if (FG) { free(FG); FG = 0; }
	if (BG) { free(BG); BG = 0; }
#if HAVE_TILES
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
/// int FindWin(WINDOW *wnd);	// may want this at some point

/*
 Optimal tileset modification target is DrawWindow, with a mapping from codepoint,(foreground) color pairs to tiles with a transparent background.
*/

//***********************************
//Globals                           *
//***********************************

WINDOW *mainwin;
const WCHAR *szWindowClass = (L"CataCurseWindow");    //Class name :D
int lastchar;          //the last character that was pressed, resets in getch
int inputdelay;         //How long getch will wait for a character to be typed
//WINDOW *_windows;  //Probably need to change this to dynamic at some point
//int WindowCount;        //The number of curses windows currently in use
HFONT font = 0;             //Handle to the font created by CreateFont
int haveCustomFont = 0;	// custom font was there and loaded

//***********************************
//Non-curses, Window functions      *
//***********************************

//This function processes any Windows messages we get. Keyboard, OnClose, etc
static LRESULT CALLBACK ProcessMessages(HWND__* hWnd, unsigned int Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg) {
	case WM_CHAR:               //This handles most key presses
		lastchar = (int)wParam;
		switch (lastchar) {
		case 13:            //Reroute ENTER key for compatilbity purposes
			lastchar = 10;
			break;
		case 8:             //Reroute BACKSPACE key for compatilbity purposes
			lastchar = 127;
			break;
		}
		break;
	case WM_KEYDOWN:                //Here we handle non-character input
		switch (wParam) {
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
		}
		break;
		// \todo we usually don't want to erase background but we have to when resizing or else.
		// Control variable in OS_Window; starts true, set by resizing, cleared by redrawing background.
		//      case WM_ERASEBKGND: return 1;
	case WM_PAINT:              //Pull from our backbuffer, onto the screen
		{
		OS_Window& _win = win();
		BitBlt(_win.dc(), 0, 0, _win.width(), _win.height(), _win.backbuffer(), 0, 0, SRCCOPY);
		ValidateRect(_win, nullptr);
		}
		break;
	case WM_DESTROY:
		exit(0);//A messy exit, but easy way to escape game loop
	default://If we didnt process a message, return the default value for it
		return DefWindowProcW(hWnd, Msg, wParam, lParam);
	}
	return 0;
}

//Registers, creates, and shows the Window!!
static bool WinCreate(OS_Window& _win)
{
    WNDCLASSEXW WindowClassType;
    const WCHAR *szTitle=  (L"Cataclysm" " (" __DATE__ ")");
    WindowClassType.cbSize = sizeof(WNDCLASSEXW);
    WindowClassType.style = 0;//No point in having a custom style, no mouse, etc
    WindowClassType.lpfnWndProc = ProcessMessages;//the procedure that gets msgs
    WindowClassType.cbClsExtra = 0;
    WindowClassType.cbWndExtra = 0;
    WindowClassType.hInstance = OS_Window::program;// hInstance
    WindowClassType.hIcon = LoadIcon(nullptr, IDI_APPLICATION);//Default Icon
    WindowClassType.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);//Default Icon
    WindowClassType.hCursor = LoadCursor(nullptr, IDC_ARROW);//Default Pointer
    WindowClassType.lpszMenuName = nullptr;
    WindowClassType.hbrBackground = CreateSolidBrush(RGB(0, 0, 0)); // will be auto-deleted on close
    WindowClassType.lpszClassName = szWindowClass;
    if (!RegisterClassExW(&WindowClassType)) return false;
    const unsigned int WindowStyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME) & ~(WS_MAXIMIZEBOX);
    _win = CreateWindowExW(WS_EX_APPWINDOW,
                           szWindowClass , szTitle,WindowStyle, _win.X(),
                           _win.Y(), _win.width() + OS_Window::BorderWidth,
                           _win.height() + OS_Window::BorderHeight + OS_Window::TitleSize,
                           0, 0, OS_Window::program, nullptr);

	SelectObject(_win.backbuffer(), font);//Load our font into the DC
    return _win;
}

//Unregisters, releases the DC if needed, and destroys the window.
static void WinDestroy()
{
	win().clear();
	UnregisterClassW(szWindowClass, OS_Window::program);	// would happen on program termination anyway
};

static void DrawWindow(WINDOW *w)
{
	OS_Window& _win = win();
    int i,j;
    char tmp;	// following assumes char is signed
    for (j=0; j<w->height; j++){
		auto& line = w->line[j];
		if (!line.touched) continue;
		line.touched = false;
		const int drawy = ((w->y + j)*fontheight);//-j;
		if ((drawy + fontheight) > _win.height()) continue;	// reject out of bounds
		for (i=0; i<w->width; i++){
			const int drawx=((w->x+i)*fontwidth);
			if ((drawx + fontwidth) > _win.width()) continue;	// reject out of bounds
                {
                tmp = line.chars[i];	// \todo alternate data source here for tiles
                int FG = line.FG[i];
                int BG = line.BG[i];
#if HAVE_TILES
				if (const unsigned short bg_tile = line.background_tiles[i]) {
					const OS_Image& tile = _cache[bg_tile];
					_win.PrepareToDraw(tile.handle());
					_win.Draw(drawx,drawy,fontwidth,fontheight,0,0, tile.width(),tile.height());
					// if background color is not black, draw it as translucent (this handles nightvision and boomered statuses, at least)
					if (0 != BG) _win.FillRect(drawx, drawy, fontwidth, fontheight, BG, UCHAR_MAX / 2);
				} else
#endif
					_win.FillRect(drawx, drawy, fontwidth, fontheight, BG);

#if HAVE_TILES
				if (const unsigned short fg_tile = line.tiles[i]) {
					const OS_Image& tile = _cache[fg_tile];
					_win.PrepareToDraw(tile.handle());
					_win.Draw(drawx, drawy, fontwidth, fontheight, 0, 0, tile.width(), tile.height());
					continue;
				}
#endif
				if (' ' == tmp) continue; // do not waste CPU on ASCII space, any sane font will be blank
				if (FG == BG) continue;		// likewise do not waste CPU on foreground=background color (affected by tiles support)

                if ( tmp > 0){
                //if (tmp==95){//If your font doesnt draw underscores..uncomment
                //        HorzLineDIB(drawx,drawy+fontheight-2,drawx+fontwidth,1,FG);
                //    } else { // all the wa to here
					const auto fg = _win.color(FG);
					int color = RGB(fg.rgbRed, fg.rgbGreen, fg.rgbBlue);
                    SetTextColor(_win.backbuffer(),color);
                    ExtTextOut(_win.backbuffer(),drawx,drawy,0,nullptr,&tmp,1,nullptr);
                //    }     //and this line too.
                } else if (  tmp < 0 ) {
                    switch (tmp) {
#define VertLineDIB(X,Y,Y2,THICKNESS,COLOR) _win.FillRect(X,Y,THICKNESS,(Y2)-(Y),COLOR)
#define HorzLineDIB(X,Y,X2,THICKNESS,COLOR) _win.FillRect(X,Y,(X2)-(X),THICKNESS,COLOR)
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
#undef VertLineDIB
#undef HorzLineDIB
					default:
                        // SetTextColor(DC,_windows[w].line[j].chars[i].color.FG);
                        // TextOut(DC,drawx,drawy,&tmp,1);
                        break;
                    }
                    };//switch (tmp)
                }//(tmp < 0)
            };//for (i=0;i<_windows[w].width;i++)
    };// for (j=0;j<_windows[w].height;j++)
    w->draw=false;                //We drew the window, mark it as so
};

//***********************************
//Psuedo-Curses Functions           *
//***********************************

//Basic Init, create the font, backbuffer, etc
WINDOW *initscr(void)
{
   // _windows = new WINDOW[20];         //initialize all of our variables
	OS_Window& _win = offscreen();
    lastchar=-1;
    inputdelay=-1;

	SetFontSize(OPTIONS[OPT_FONT_HEIGHT]/2, OPTIONS[OPT_FONT_HEIGHT]);

    std::string typeface;
	std::ifstream fin;
	fin.open("data\\FONTDATA");
 if (!fin.is_open()){
     MessageBox(_win, "Failed to open FONTDATA, loading defaults.", nullptr, 0);
	 if (0 >= fontwidth) SetFontSize(8, 16);	// predicted to be dead code
 } else {
     getline(fin, typeface);
	 if (0 >= fontwidth) {	// predicted to be dead code
		 int tmp_width;
		 int tmp_height;

		 fin >> tmp_width;
		 fin >> tmp_height;
		 if ((0 >= fontwidth && 4 >= tmp_width)
			 || (0 >= fontheight && 4 >= tmp_height)) {
			 MessageBox(_win, "Invalid font size specified!", nullptr, 0);
			 tmp_width = 8;
			 tmp_height = 16;
		 }
		 if (0 >= fontwidth) SetFontSize(tmp_width, tmp_height);
	 }
 }
 haveCustomFont = (!typeface.empty() ? AddFontResourceExA("data\\termfont", FR_PRIVATE, nullptr) : 0);
 if (!haveCustomFont) MessageBox(_win, "Failed to load default font, using FixedSys.", nullptr, 0);
 {
	 const char* const t_face = haveCustomFont ? typeface.c_str() : "FixedSys";	//FixedSys will be user-changable at some point in time??
	 font = CreateFont(fontheight, fontwidth, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		 ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		 PROOF_QUALITY, FF_MODERN, t_face);
 }

//    WinCreate(_win);    //Create the actual window, register it, etc

	// cf mapdata.h: typical value of SEEX/SEEY is 12 so the console is 25x25 display, 55x25 readout
	// \todo set default option values for windows; once mainwin is constructed it's too late

    mainwin = newwin(VIEW, SCREEN_WIDTH,0,0);
    return mainwin;   //create the 'stdscr' window and return its ref
};

WINDOW *newwin(int nlines, int ncols, int begin_y, int begin_x)
{
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

    for (int j=0; j<nlines; j++) newwindow->line[j].init(ncols);
    //WindowCount++;
    return newwindow;
};


//Deletes the window and marks it as free. Clears it just in case.
int delwin(WINDOW *win)
{
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

bool mvwaddbgtile(WINDOW *win, int y, int x, const char* bgtile)
{
#if HAVE_TILES
	if (!bgtile || !bgtile[0]) return false;
	std::string tmp(bgtile);
	if (!_translate.count(tmp)) return false;
	if (!wmove(win, y, x)) return false;
	// win->cursorx,cursory now set to x,y
	const unsigned short t_index = _translate[tmp];	// invariant: tile index at least 1

	win->line[y].background_tiles[x] = t_index;
	win->draw = true;
	addedchar(win);
	return true;
#else
	return false;
#endif
}

bool mvwaddfgtile(WINDOW *win, int y, int x, const char* fgtile)
{
#if HAVE_TILES
	if (!fgtile || !fgtile[0]) return false;
	std::string tmp(fgtile);
	if (!_translate.count(tmp)) return false;
	if (!wmove(win, y, x)) return false;
	// win->cursorx,cursory now set to x,y
	const unsigned short t_index = _translate[tmp];	// invariant: tile index at least 1

	win->line[y].tiles[x] = t_index;
	win->draw = true;
	addedchar(win);
	return true;
#else
	return false;
#endif
}

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
 InvalidateRect(win(),nullptr,true);
 lastchar=ERR;//ERR=-1
    if (inputdelay < 0)
    {
        do
        {
			OS_Window::CheckMessages();
            if (lastchar!=ERR) break;
            MsgWaitForMultipleObjects(0, nullptr, FALSE, 50, QS_ALLEVENTS);//low cpu wait!
        }
        while (lastchar==ERR);
    }
    else if (inputdelay > 0)
    {
        unsigned long starttime=GetTickCount();
        unsigned long endtime;
        do
        {
			OS_Window::CheckMessages();        //MsgWaitForMultipleObjects won't work very good here
            endtime=GetTickCount(); //it responds to mouse movement, and WM_PAINT, not good
            if (lastchar!=ERR) break;
            Sleep(2);
        }
        while (endtime<(starttime+inputdelay));
    }
    else
    {
		OS_Window::CheckMessages();
    };
    Sleep(25);
    return lastchar;
};

//The core printing function, prints characters to the array, and sets colors
inline int printstring(WINDOW *win, char *fmt)
{
 size_t size = strlen(fmt);
 size_t j;
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
  } else if (newline(win)==0){ // if the character is a newline, make sure to move down a line
      return 0;
  }
 }
 win->draw=true;
 return 1;
}

//Prints a formatted string to a window at the current cursor, base function
int wprintw(WINDOW *win, const char *fmt, ...)
{
	if (reject_not_whitelisted_printf(fmt)) return ERR;
    va_list args;
    va_start(args, fmt);
	char printbuf[2048];
	vsprintf_s<sizeof(printbuf)>(printbuf, fmt, args);
	va_end(args);
    return printstring(win,printbuf);
};

//Prints a formatted string to a window, moves the cursor
int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...)
{
	if (reject_not_whitelisted_printf(fmt)) return ERR;
	va_list args;
    va_start(args, fmt);
	char printbuf[2048];
	vsprintf_s<sizeof(printbuf)>(printbuf, fmt, args);
	va_end(args);
    if (wmove(win,y,x)==0) return 0;
    return printstring(win,printbuf);
};

//Prints a formatted string to window 0 (stdscr), moves the cursor
int mvprintw(int y, int x, const char *fmt, ...)
{
	if (reject_not_whitelisted_printf(fmt)) return ERR;
	va_list args;
    va_start(args, fmt);
    char printbuf[2048];
    vsprintf_s<sizeof(printbuf)>(printbuf, fmt, args);
    va_end(args);
    if (move(y,x)==0) return 0;
    return printstring(mainwin,printbuf);
};

//Prints a formatted string to window 0 (stdscr) at the current cursor
int printw(const char *fmt, ...)
{
	if (reject_not_whitelisted_printf(fmt)) return ERR;
	va_list args;
    va_start(args, fmt);
	char printbuf[2048];
	vsprintf_s<sizeof(printbuf)>(printbuf, fmt, args);
	va_end(args);
    return printstring(mainwin,printbuf);
};

//erases a window of all text and attributes
int werase(WINDOW *win)
{
    for (int j=0; j<win->height; j++) {
		auto& line = win->line[j];
		memset(line.chars, 0, win->width*sizeof(*line.chars));
		memset(line.FG, 0, win->width * sizeof(*line.FG));
		memset(line.BG, 0, win->width * sizeof(*line.BG));
#if HAVE_TILES
		memset(line.background_tiles, 0, win->width * sizeof(*line.background_tiles));
		memset(line.tiles, 0, win->width * sizeof(*line.tiles));
#endif
		line.touched=true;
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

pairs colorpairs[((A_COLOR) >> 17)+1];   //storage for pair'ed colored, should be dynamic, meh

//pairs up a foreground and background color and puts it into the array of pairs
int init_pair(short pair, short f, short b)
{
	if (sizeof(colorpairs) / sizeof(*colorpairs) <= pair || 0 > pair) return 0;	// out of range
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
    RemoveFontResourceExA("data\\termfont",FR_PRIVATE,nullptr);//Unload it
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
int getmaxx(WINDOW* win)
{
	if (!win) return (OS_Window::ScreenWidth - OS_Window::BorderWidth) / fontwidth; // StdScr
	return win->width;
};

//gets the max y of a window (the height)
int getmaxy(WINDOW* win)
{
	if (!win) return (OS_Window::ScreenHeight - OS_Window::BorderHeight - OS_Window::TitleSize) / fontheight; // StdScr
	return win->height;
};

#define BGR(B,G,R) {B, G, R, 0}

int start_color(void)
{
 RGBQUAD* windowsPalette = OS_Window::CreateNewColorTable(16);     //Colors in the struct are BGR!! not RGB!!
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
 return offscreen().SetColorTable(windowsPalette, 16);
};
#undef BGR

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
    const int pairNumber = (attrs & A_COLOR) >> 17;
    win->FG=colorpairs[pairNumber].FG;
    win->BG=colorpairs[pairNumber].BG;
    if (attrs & A_BOLD) win->FG += 8;
    if (attrs & A_BLINK) win->BG += 8;
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
#if HAVE_TILES
   if (!win->BG && (!win->FG || ' ' == ch)) {	// true display blank: erase the tile assignments
	   win->line[cury].background_tiles[curx] = 0;
	   win->line[cury].tiles[curx] = 0;
   }
#endif


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
