#if (defined _WIN32 || defined WINDOWS)
#include "catacurse.h"
#include <fstream>
#include <string>
#include <map>

// requests W2K baseline Windows API ... unsure if this has link-time consequences w/posix_time.cpp which does not specify
#define _WIN32_WINNT 0x0500
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#if HAVE_MS_COM
#include <wincodec.h>
#include <locale>
#include <codecvt>

#include "Zaimoni.STL/MetaRAM.hpp"
#else
// lower-tech pathway that ectually exists on MingWin
#include "Zaimoni.STL/cstdio"
#endif
#ifndef NDEBUG
#include <iostream>
#endif

#if HAVE_MS_COM
#define HAVE_TILES 1
#endif
// #define HAVE_TILES 1

#if HAVE_TILES
// #define USING_RGBA32 1
#endif

class OS_Window
{
private:
	static HWND _last;
	static HDC _last_dc;

	BITMAPINFOHEADER _backbuffer_stats;
	void (OS_Window::*_fillRect)(int x, int y, int w, int h, unsigned char c);
	RGBQUAD* _color_table;
	size_t _color_table_size;
	HWND _window;
	RECT _dim;
	HDC _dc;
	HDC _backbuffer;	// backbuffer for the physical device context
	HDC _staging;	// the current tile to draw would go here, for instance
	HGDIOBJ _staging_0;	// where the original bitmap for _staging goes
//	unsigned char* _dcbits;	//the bits of the screen image, for direct access
public:
	static unsigned char* _dcbits;	// XXX should be private per-instance; trying to transition legacy code
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
		memset(&src, 0, sizeof(OS_Window));
	}
	~OS_Window() { destroy();  }

	OS_Window& operator=(const OS_Window& src) = delete;
	OS_Window& operator=(OS_Window&& src) {
		static_assert(std::is_standard_layout<OS_Window>::value, "OS_Window move assignment is invalid");
		destroy();
		memmove(this, &src, sizeof(OS_Window));
		memset(&src, 0, sizeof(OS_Window));
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
		if (!_staging_0) return false;
		// \todo bounds-checking
		return StretchBlt(_backbuffer, xDest, yDest, wDest, hDest, _staging, xSrc, ySrc, wSrc, hSrc,SRCCOPY);
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

	int X() const { return _dim.left; };
	int Y() const { return _dim.top; };
	int width() const { return _dim.right-_dim.left; }
	int height() const { return _dim.bottom - _dim.top; }
	void center(int w, int h) {
		_dim.left = (OS_Window::ScreenWidth - w) / 2;
		_dim.top = (OS_Window::ScreenHeight - h) / 2;
		_dim.right = _dim.left + w;
		_dim.bottom = _dim.top + h;
	}

	void clear() {
		if (_staging_0 && SelectObject(_staging, _staging_0)) _staging_0 = 0;
		if (_staging && DeleteDC(_staging)) _staging = 0;
		if (_backbuffer && DeleteDC(_backbuffer)) _backbuffer = 0;
		if (_last_dc == _dc) _last_dc = 0;
		if (_dc && ReleaseDC(_window, _dc)) _dc = 0;
		if (_last == _window) _last = 0;
		if (_window && DestroyWindow(_window)) _window = 0;
	}

	bool SetBackbuffer(const BITMAPINFO& buffer_spec)
	{
		if (!_window) return false;
		if (!_dc && !(_dc = GetDC(_window))) return false;
		_last_dc = _dc;
		if (!_backbuffer && !(_backbuffer = CreateCompatibleDC(_dc))) return false;
		if (!_staging && !(_staging = CreateCompatibleDC(_dc))) return false;
		_backbuffer_stats = buffer_spec.bmiHeader;
		// handle invariant parts here
		_backbuffer_stats.biSize = sizeof(BITMAPINFOHEADER);
		_backbuffer_stats.biWidth = width();
		_backbuffer_stats.biHeight = -height();
		_backbuffer_stats.biPlanes = 1;
		// we only handle 8 and 32 bits
		if (32 == _backbuffer_stats.biBitCount) {
			_backbuffer_stats.biClrUsed = 0;
			_backbuffer_stats.biClrImportant = 0;
			_backbuffer_stats.biSizeImage = width()*height()*4;
		} else if (8 == _backbuffer_stats.biBitCount) {
			// reality checks
			if (_backbuffer_stats.biClrUsed != _backbuffer_stats.biClrImportant) throw std::logic_error("inconsistent color table");
			if (1 > _backbuffer_stats.biClrUsed || 256 < _backbuffer_stats.biClrUsed) throw std::logic_error("color table has unreasonable number of entries");
			_backbuffer_stats.biSizeImage = width()*height();
		} else throw std::logic_error("unhandled color depth");

		_backbuffer_stats.biCompression = BI_RGB;   //store it in uncompressed bytes

		HBITMAP backbit = CreateDIBSection(0, (BITMAPINFO*)&_backbuffer_stats, DIB_RGB_COLORS, (void**)&_dcbits, NULL, 0);
		DeleteObject(SelectObject(_backbuffer, backbit));//load the buffer into DC
		SetBkMode(_backbuffer, TRANSPARENT);//Transparent font backgrounds
		if (8 >= _backbuffer_stats.biBitCount && _color_table && 256 >= _color_table_size) return SetDIBColorTable(_backbuffer, 0, _color_table_size, _color_table);	// actually need this
		return true;
	}

	static RGBQUAD* CreateNewColorTable(size_t n) { return zaimoni::_new_buffer_nonNULL_throws<RGBQUAD>(n); }
	bool SetColorTable(RGBQUAD*& colors, size_t n)	// probably should capture the color table instead and use it as a translation device
	{
		if (_color_table) free(_color_table);
		_color_table = colors;
		_color_table_size = n;
		colors = 0;
		if (8 >= _backbuffer_stats.biBitCount && 256>=n) return SetDIBColorTable(_backbuffer, 0, n, _color_table);	// actually need this
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

	bool Resize()
	{
		RECT tmp;
		if (!GetWindowRect(_window, &tmp)) return false;
		if (!MoveWindow(_window, tmp.left, tmp.top, width() + OS_Window::BorderWidth, height() + OS_Window::BorderHeight + OS_Window::TitleSize, true)) return false;	// this does not handle the backbuffer bitmap
		// problem...probably have to undo both the backbuffer and its device context
		if (_backbuffer && DeleteDC(_backbuffer)) _backbuffer = 0;
		if (_last_dc == _dc) _last_dc = 0;
		if (_dc && ReleaseDC(_window, _dc)) _dc = 0;
		_dcbits = 0;
		BITMAPINFO working;
		memset(&working, 0, sizeof(working));
		working.bmiHeader = _backbuffer_stats;
		return SetBackbuffer(working);
	}
private:
	void destroy() { 
		clear();
		free(_color_table);
		_color_table = 0;
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
};

#if _MSC_VER
// MSVC is empirically broken (following is correct for _MSC_VER 1913 ... 1914)
#define BORDERWIDTH_SCALE 5
#define BORDERHEIGHT_SCALE 5
#else
// assume we are on a working WinAPI (e.g., MingWin)
#define BORDERWIDTH_SCALE 2
#define BORDERHEIGHT_SCALE 2
#endif
unsigned char* OS_Window::_dcbits = 0;
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
	}
	// OS_Image(const wchar_t* src)
	OS_Image(const OS_Image& src) = delete;
	OS_Image(OS_Image&& src) {
		static_assert(std::is_standard_layout<OS_Window>::value, "OS_Window move constructor is invalid");
		memmove(this,&src,sizeof(OS_Image));
		memset(&src, 0, sizeof(OS_Image));
	}
	// clipping constructor
	OS_Image(const OS_Image& src, const size_t origin_x, const size_t origin_y, const size_t width, const size_t height) {
		if (0 >= width) throw std::logic_error("OS_Image::OS_Image: 0 >= width");
		if (0 >= height) throw std::logic_error("OS_Image::OS_Image: 0 >= height");
		if (((size_t)(-1)/sizeof(_pixels[0]))/width < height) throw std::logic_error("OS_Image::OS_Image: ((size_t)(-1)/sizeof(_pixels[0]))/width < height");
		if (src.width() <= origin_x) throw std::logic_error("OS_Image::OS_Image: src.width() <= origin_x");
		if (src.height() <= origin_y) throw std::logic_error("OS_Image::OS_Image: src.height() <= origin_y");
		if (src.width() - origin_x < width) throw std::logic_error("OS_Image::OS_Image: src.width() - origin_x < width");
		if (src.height() - origin_y < height) throw std::logic_error("OS_Image::OS_Image: src.height() - origin_y < height");
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
			memmove(_pixels + (scan_y*width), incoming + ((scan_y + origin_y)*src.width()), sizeof(RGBQUAD)*width);
		}
		_x = CreateCompatibleBitmap(OS_Window::last_dc(),width,height);
		if (!_x) {
			free(_pixels);
			throw std::bad_alloc();
		}
		if (!SetDIBits(OS_Window::last_dc(), (HBITMAP)_x, 0, height, _pixels, (LPBITMAPINFO)(&_data), DIB_RGB_COLORS)) {
			free(_pixels);
			throw std::bad_alloc();
		}
		_have_info = true;
	}

	~OS_Image() { clear(); }

	OS_Image& operator=(const OS_Image& src) = delete;
	OS_Image& operator=(OS_Image&& src)
	{
		static_assert(std::is_standard_layout<OS_Window>::value, "OS_Window move constructor is invalid");
		clear();
		memmove(this, &src, sizeof(OS_Image));
		memset(&src, 0, sizeof(OS_Image));
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
private:
	void init() {
		if (_x && !_have_info) {
			memset(&_data, 0, sizeof(_data));
			_data.biSize = sizeof(_data);
			_have_info = GetDIBits(OS_Window::last_dc(), (HBITMAP)_x, 0, 0, NULL, (LPBITMAPINFO)(&_data), DIB_RGB_COLORS);	// XXX needs testing, may need an HDC
			_data.biBitCount = 32;	// force alpha channel in (no padding bytes)
			_data.biCompression = BI_RGB;
			_data.biHeight = (0 < _data.biHeight) ? -_data.biHeight : _data.biHeight;	// force bottom-up
		}
	}

	RGBQUAD* pixels() const
	{
		if (_pixels) return _pixels;
		if (!_have_info) return 0;
		RGBQUAD* const tmp = zaimoni::_new_buffer<RGBQUAD>(width()*height());
		if (!tmp) return 0;
		if (!GetDIBits(OS_Window::last_dc(), (HBITMAP)_x, 0, 0, NULL, (LPBITMAPINFO)(&_data), DIB_RGB_COLORS))	// may need an HDC
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
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > widen;
		const auto w_src = widen.from_bytes(src);

		// plan B
		// Create a decoder
		// XXX research how to not leak these COM objects
		IWICImagingFactory* factory = NULL;
		HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
		if (!SUCCEEDED(hr)) return false;

		IWICBitmapDecoder* parser = 0;
		hr = factory->CreateDecoderFromFilename(w_src.c_str(), NULL, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &parser);
		if (!SUCCEEDED(hr)) {
			factory->Release();
			return false;
		}

		// Retrieve the first frame of the image from the decoder
		IWICBitmapFrameDecode *pFrame = NULL;
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

		IWICFormatConverter* conv = NULL;
		hr = factory->CreateFormatConverter(&conv);
		if (!SUCCEEDED(hr)) {
			pFrame->Release();
			parser->Release();
			factory->Release();
			return 0;
		}
		hr = conv->Initialize(pFrame, GUID_WICPixelFormat32bppBGRA, WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom);
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

		hr = conv->CopyPixels(NULL, 4 * width, buf_len, reinterpret_cast<unsigned char*>(buffer));
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
		pixels = NULL;
		got_stats - false;
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

		HBITMAP tmp = CreateCompatibleBitmap(OS_Window::last_dc(), native_width, native_height);
		if (!tmp) {
			free(pixels); pixels = 0;
			return 0;
		}
		got_stats = SetDIBits(OS_Window::last_dc(), tmp, 0, native_height, pixels, (BITMAPINFO*)(&working), DIB_RGB_COLORS);
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
OS_Window _win;				// proxy for actual window
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
	_win.center(80 * fontwidth, 25 * fontheight);
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

bool load_tile(const char* src)
{
	static unsigned short _next = 0;

	if (!src || !src[0]) return false;	// nothing to load
	if ((unsigned short)(-1) == _next) return false;	// at implementation limit
	if (_translate.count(src)) return true;	// already loaded
	const char* const has_rotation_specification = strchr(src, ':');
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
			if (SetFontSize(16, 16) && _win) _win.Resize();
		} else {
			OS_Image image(base_tile.c_str());
			if (!image.handle()) return false;	// failed to load
			_translate[base_tile] = ++_next;
			_cache[_next] = std::move(image);
			if (SetFontSize(16, 16) && _win) _win.Resize();
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
void WinDestroy();
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
int lastchar;          //the last character that was pressed, resets in getch
int inputdelay;         //How long getch will wait for a character to be typed
//WINDOW *_windows;  //Probably need to change this to dynamic at some point
//int WindowCount;        //The number of curses windows currently in use
HFONT font = 0;             //Handle to the font created by CreateFont
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
    WindowClassType.hInstance = OS_Window::program;// hInstance
    WindowClassType.hIcon = LoadIcon(NULL, IDI_APPLICATION);//Default Icon
    WindowClassType.hIconSm = LoadIcon(NULL, IDI_APPLICATION);//Default Icon
    WindowClassType.hCursor = LoadCursor(NULL, IDC_ARROW);//Default Pointer
    WindowClassType.lpszMenuName = NULL;
    WindowClassType.hbrBackground = 0;//Thanks jday! Remove background brush
    WindowClassType.lpszClassName = szWindowClass;
    if (!RegisterClassExW(&WindowClassType)) return false;
	const unsigned int WindowStyle = WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME) & ~(WS_MAXIMIZEBOX);
	return (_win = CreateWindowExW(WS_EX_APPWINDOW || WS_EX_TOPMOST * 0,
                                   szWindowClass , szTitle,WindowStyle, _win.X(),
                                   _win.Y(), _win.width() + OS_Window::BorderWidth,
                                   _win.height() + OS_Window::BorderHeight + OS_Window::TitleSize,
		                           0, 0, OS_Window::program, NULL));
};

//Unregisters, releases the DC if needed, and destroys the window.
void WinDestroy()
{
	_win.clear();
	UnregisterClassW(szWindowClass, OS_Window::program);	// would happen on program termination anyway
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
            BitBlt(_win.dc(), 0, 0, _win.width(), _win.height(), _win.backbuffer(), 0, 0,SRCCOPY);
            ValidateRect(_win,NULL);
            break;
        case WM_DESTROY:
            exit(0);//A messy exit, but easy way to escape game loop
        default://If we didnt process a message, return the default value for it
            return DefWindowProcW(hWnd, Msg, wParam, lParam);
    };
    return 0;
};

#if USING_RGBA32
#else
//The following 3 methods use mem functions for fast drawing
inline void VertLineDIB(int x, int y, int y2,int thickness, unsigned char color)
{
    for (int j=y; j<y2; j++) memset(&OS_Window::_dcbits[x+j*_win.width()],color,thickness);
};
inline void HorzLineDIB(int x, int y, int x2,int thickness, unsigned char color)
{
    for (int j=y; j<y+thickness; j++) memset(&OS_Window::_dcbits[x+j* _win.width()],color,x2-x);
};
inline void FillRectDIB(int x, int y, int width, int height, unsigned char color)
{
    for (int j=y; j<y+height; j++) memset(&OS_Window::_dcbits[x+j* _win.width()],color,width);
};
#endif

void DrawWindow(WINDOW *win)
{
    int i,j;
    char tmp;	// following assumes char is signed
    for (j=0; j<win->height; j++){
		if (!win->line[j].touched) continue;
		win->line[j].touched = false;
		const int drawy = ((win->y + j)*fontheight);//-j;
		if ((drawy + fontheight) > _win.height()) continue;	// reject out of bounds
		for (i=0; i<win->width; i++){
			const int drawx=((win->x+i)*fontwidth);
			if ((drawx + fontwidth) > _win.width()) continue;	// reject out of bounds
                {
                tmp = win->line[j].chars[i];	// \todo alternate data source here for tiles
                int FG = win->line[j].FG[i];
                int BG = win->line[j].BG[i];
				// \todo interpose background tile drawing here
#if USING_RGBA32
#error need to implement DrawWindow
#else
				FillRectDIB(drawx,drawy,fontwidth,fontheight,BG);
#endif

				// \todo interpose foreground tile drawing here
#if 0
				if (unsigned short t_index = win->line[j].tiles[i]) {
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
                    ExtTextOut(_win.backbuffer(),drawx,drawy,0,NULL,&tmp,1,NULL);
                //    }     //and this line too.
                } else if (  tmp < 0 ) {
                    switch (tmp) {
#if USING_RGBA32
#error need to implement DrawWindow
#else
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
#endif
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
     MessageBox(_win, "Failed to open FONTDATA, loading defaults.", NULL, 0);
	 if (0 >= fontwidth) SetFontSize(8, 16);
 } else {
     getline(fin, typeface);
	 int tmp_width;
	 int tmp_height;

     fin >> tmp_width;
     fin >> tmp_height;
	 if (   (0 >= fontwidth && 4 >= tmp_width)
		 || (0 >= fontheight && 4 >= tmp_height)) {
         MessageBox(_win, "Invalid font size specified!", NULL, 0);
		 tmp_width  = 8;
		 tmp_height = 16;
     }
	 if (0 >= fontwidth) SetFontSize(tmp_width, tmp_height);
 }
 haveCustomFont = (!typeface.empty() ? AddFontResourceExA("data\\termfont", FR_PRIVATE, NULL) : 0);
 if (!haveCustomFont) MessageBox(_win, "Failed to load default font, using FixedSys.", NULL, 0);
 {
	 const char* const t_face = haveCustomFont ? typeface.c_str() : "FixedSys";	//FixedSys will be user-changable at some point in time??
	 font = CreateFont(fontheight, fontwidth, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
		 ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		 PROOF_QUALITY, FF_MODERN, t_face);
 }

    WinCreate();    //Create the actual window, register it, etc
    ZeroMemory(&bmi, sizeof(BITMAPINFO));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = _win.width();
    bmi.bmiHeader.biHeight = -_win.height();
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount=8;
    bmi.bmiHeader.biCompression = BI_RGB;   //store it in uncompressed bytes
    bmi.bmiHeader.biSizeImage = _win.width() * _win.height() * 1;
    bmi.bmiHeader.biClrUsed=16;         //the number of colors in our palette
    bmi.bmiHeader.biClrImportant=16;    //the number of colors in our palette

	_win.SetBackbuffer(bmi);
    
    SelectObject(_win.backbuffer(), font);//Load our font into the DC
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
 InvalidateRect(_win,NULL,true);
 lastchar=ERR;//ERR=-1
    if (inputdelay < 0)
    {
        do
        {
			OS_Window::CheckMessages();
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
 return _win.SetColorTable(windowsPalette, 16);
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
