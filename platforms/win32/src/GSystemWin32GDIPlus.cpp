/*
	GUIDO Library
	Copyright (C) 2006  Grame

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include <iostream>
using namespace std;

#include "GUIDOEngine.h"
#include "GSystemWin32GDIPlus.h"
#include "GFontWin32GDIPlus.h"
#include "GDeviceWin32GDIPlus.h"

static ULONG_PTR gGdiplusToken = 0;

GUIDOAPI(GuidoErrCode) GuidoInitGDIPlusWithHDC(void* dispHDC, VGSystem** vgsystem, VGDevice** vgdevice)
{
	GuidoInitDesc desc;

	if(vgsystem == NULL || vgdevice == NULL)
		return guidoErrBadParameter;

	GdiplusStartupInput input;
	GdiplusStartup(&gGdiplusToken, &input, NULL);

	PrivateFontCollection* fc = new PrivateFontCollection();
	fc->AddFontFile(L"Guido2.ttf");

	*vgsystem = new GSystemWin32GDIPlus(static_cast<HDC>(dispHDC), nullptr, fc);
	*vgdevice = desc.graphicDevice = (*vgsystem)->CreateDisplayDevice();
	desc.musicFont = "Guido2";
	desc.textFont  = "Times";
	GuidoErrCode errcode = GuidoInit (&desc);
	return errcode;
}

#pragma warning (disable : 4996)	// for _CRT_SECURE_NO_DEPRECATE

ULONG_PTR GSystemWin32GDIPlus::mGdiplusToken = 0;
// --------------------------------------------------------------
GSystemWin32GDIPlus::GSystemWin32GDIPlus( HDC dispDC, HDC printDC ) 
					: mDispDC(dispDC), mPrintDC(printDC)													
{
}

// --------------------------------------------------------------
GSystemWin32GDIPlus::GSystemWin32GDIPlus( HDC dispDC, HDC printDC, FontCollection* fc )
					: mDispDC(dispDC), mPrintDC(printDC), mFontCollection(fc)
{
}

// --------------------------------------------------------------
GSystemWin32GDIPlus::~GSystemWin32GDIPlus()
{
	delete mFontCollection;

	if (gGdiplusToken) GdiplusShutdown(gGdiplusToken);
	gGdiplusToken = 0;
}

// --------------------------------------------------------------
void GSystemWin32GDIPlus::Start()
{
	if (!mGdiplusToken) {
		GdiplusStartupInput gdiplusStartupInput;
		GdiplusStartup(&mGdiplusToken, &gdiplusStartupInput, NULL);
	}
}

// --------------------------------------------------------------
void GSystemWin32GDIPlus::ShutDown()
{
	if (mGdiplusToken) GdiplusShutdown(mGdiplusToken);
	mGdiplusToken = 0;
}

// --------------------------------------------------------------
// - VGDevice services ------------------------------------------
// --------------------------------------------------------------
// Creates and returns a pointer to a new display VGDevice 
// which can be used directly to draw on the screen.
VGDevice* GSystemWin32GDIPlus::CreateDisplayDevice()	
{
	GDeviceWin32GDIPlus * dispDevice = new GDeviceWin32GDIPlus( mDispDC, this );
	return dispDevice;
}

// --------------------------------------------------------------
// Creates and returns a pointer to a new memory VGDevice
//	compatible with the application's current screen. This device
// can be used to draw into a bitmap.
VGDevice* GSystemWin32GDIPlus::CreateMemoryDevice( int inWidth, int inHeight )
{
	GDeviceWin32GDIPlus * memDevice = new GDeviceWin32GDIPlus( inWidth, inHeight, this );
	return memDevice;
}

// --------------------------------------------------------------
VGDevice* GSystemWin32GDIPlus::CreateAntiAliasedMemoryDevice( int inWidth, int inHeight )
{
	return CreateMemoryDevice( inWidth, inHeight );
}

// --------------------------------------------------------------
// Creates and returns a pointer to a new memory VGDevice
// compatible with the file (pixmap) located at the specified path.
VGDevice* GSystemWin32GDIPlus::CreateMemoryDevice( const char * inPath )
{
	//The graphics file formats supported by GDI+ are BMP, GIF, JPEG, 
	//PNG, TIFF, Exif, WMF, and EMF. 
	GDeviceWin32GDIPlus * memDevice = new GDeviceWin32GDIPlus( inPath, this );
	return memDevice;
}

// --------------------------------------------------------------
// Creates and returns a pointer to a new printer VGDevice.
VGDevice* GSystemWin32GDIPlus::CreatePrinterDevice( )
{
	GDeviceWin32GDIPlus * printDevice = new GDeviceWin32GDIPlus( mPrintDC, this );
	printDevice->SetScale(1.f/6.5f, 1.f/6.5f);
	return printDevice;
}

// --------------------------------------------------------------
// - Font services ----------------------------------------------
// --------------------------------------------------------------
// "width" feature for font is not portable: has been replaced by
// horizontal scaling of device context.
const VGFont* GSystemWin32GDIPlus::CreateVGFont( const char * faceName, int size, int properties ) const
{
	// GDI+ Fonts 
	int charCount = (int)strlen( faceName );
	DWORD count = (DWORD)mbstowcs(NULL, faceName, charCount);
	WCHAR * wFamily = (WCHAR*) malloc ((count + 1) * sizeof(WCHAR));
	mbstowcs(wFamily, faceName, charCount+1);

	//std::string s(faceName);
	//std::wstring wFamily(s.length(),L' ');
	//std::copy(s.begin(), s.end(), wFamily.begin());

	int style = FontStyleRegular;
	if (properties)
	{
		if (properties & VGFont::kFontBold)			style	|= FontStyleBold;
		if (properties & VGFont::kFontItalic)		style	|= FontStyleItalic;
		if (properties & VGFont::kFontUnderline)	style	|= FontStyleUnderline;
	}

//	Font* gdiFont = new Font(wFamily, (REAL)size, (FontStyle)style, UnitPixel);
	Font* gdiFont = new Font(wFamily, (REAL)size, (FontStyle)style, UnitWorld, mFontCollection);
	GFontWin32GDIPlus * outFont = new GFontWin32GDIPlus( gdiFont, faceName, size, properties, mFontCollection);	
	free (wFamily);
	return outFont;
}

// ----------------------------------------------------------------------------
void GSystemWin32GDIPlus::SetPrinterDC(HDC printDC)
{
	mPrintDC = printDC;	
}
