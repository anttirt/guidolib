/*
  GUIDO Library
  Copyright (C) 2002  Holger Hoos, Juergen Kilian, Kai Renz
  Copyright (C) 2003, 2004  Grame

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr

*/

/** \file GUIDOEngine.cpp

	The main Guido API
 */

// Last released version: 1.2.2, current version: 1.3.1 (see GuidoInternal.h)

#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

//#define TIMING
#include "GuidoTiming.h"

// - Guido AR
#include "ARFactory.h"
#include "ARPageFormat.h"
#include "ARMusic.h"
#include "ARMusicalVoice.h"
#include "TagParameterFloat.h"

// - Guido GR
#include "GRMusic.h"
#include "GRSpringForceIndex.h"
#include "GRStaffManager.h"
#include "GRSpring.h"
#include "GRPage.h"

// - Guido Misc
#include "GUIDOInternal.h"
#include "GUIDOParse.h"
#include "guido.h"
#include "VGDevice.h"
#include "GuidoParser.h"

#include "GUIDOEngine.h"
#include "secureio.h"
#include "FontManager.h"

#include "SVGSystem.h"
#include "SVGDevice.h"
#include "SVGMapDevice.h"
#include "SVGFont.h"

#include "AbstractSystem.h"
#include "AbstractDevice.h"
#include "AbstractFont.h"

#include "BinarySystem.h"
#include "BinaryDevice.h"
#include "BinaryFont.h"

#include "guido2.h"

// ==========================================================================
// - Guido Global variables
// ==========================================================================
const int GUIDOENGINE_MAJOR_VERSION = 1;
const int GUIDOENGINE_MINOR_VERSION = 5;
const int GUIDOENGINE_SUB_VERSION   = 7;
const char* GUIDOENGINE_VERSION_STR = "1.5.7";

ARPageFormat * gARPageFormat = 0;

// - Misc globals
GuidoGlobalSettings gGlobalSettings = { /*1 to draw the springs*/0, 0, 1, 0 };

bool gInited = false;		// GuidoInit() Flag
int gARHandlerRefCount = 0;

int	gParseErrorLine = -1;
//int gParseErrorCol = -1;

int gBoundingBoxesMap = kNoBB;	// a bits field to control bounding boxes draxing [added on May 11 2009 - DF ]

// ==========================================================================
// - Guido Main API
// ==========================================================================

// --------------------------------------------------------------------------
//		- Building abstract and graphic representations -
// --------------------------------------------------------------------------

GUIDOAPI(GuidoErrCode) GuidoInitWithIndependentSVG()
{
	GuidoInitDesc desc;

	VGSystem * gSystem= new SVGSystem(______src_guido2_svg);
	desc.graphicDevice = gSystem->CreateMemoryDevice(20,20);
	desc.musicFont = "Guido2";
	desc.textFont  = "Times";
	GuidoErrCode errcode = GuidoInit (&desc);
	return errcode;
}

GUIDOAPI(GuidoErrCode) GuidoInit( GuidoInitDesc * desc )
{
	if( desc == 0 ) return guidoErrBadParameter;

	// - Music Font (keep previous settings if desc = 0 ? )
	const char * musicFont = desc->musicFont ? desc->musicFont : FontManager::kDefaultMusicFont;
	// - Text Font
	const char * textFont = desc->textFont ? desc->textFont : FontManager::kDefaultTextFont;

	if( desc->graphicDevice )
		gGlobalSettings.gDevice = desc->graphicDevice;

	// - Skip if already initialized
    if( gInited == false )
    {
		// gets the standard-scriabin font at 4 times LSPACE (4*2*HSPACE)
		NVstring musicFontStr ( musicFont );
		FontManager::gFontScriab = FontManager::FindOrCreateFont((int)(4 * LSPACE), &musicFontStr );

	        // gets the standard Text-Font..
		NVstring textFontStr ( textFont );
		FontManager::gFontText = FontManager::FindOrCreateFont((int)(1.5f * LSPACE), &textFontStr );

		gInited = true;
	}
	// Create default page format
	gARPageFormat = new ARPageFormat();
	return guidoNoErr;
}

GUIDOAPI(GuidoErrCode) GuidoNotifySize(VGDevice* vgdevice, int newWidth, int newHeight)
{
	if(vgdevice == NULL)
		return guidoErrBadParameter;
	vgdevice->NotifySize(newWidth, newHeight);
	return guidoNoErr;
}

GUIDOAPI(GuidoErrCode) GuidoDestroyVGSystem(VGSystem* system)
{
	if(system == NULL)
		return guidoErrBadParameter;
	delete system;
	return guidoNoErr;
}

GUIDOAPI(GuidoErrCode) GuidoDestroyVGDevice(VGDevice* device)
{
	if(device == NULL)
		return guidoErrBadParameter;
	delete device;
	return guidoNoErr;
}

// ------------------------------------------------------------------------
GUIDOAPI(void) GuidoShutdown()
{
	FontManager::ReleaseAllFonts();
	gInited = false;
}


static int checkUnicode(FILE *fd)
{
	rewind( fd );
	if ( fd ) {
		int c = fgetc( fd );
		if( c == 0xff || c == 0xfe) {
			c = fgetc( fd );
			if( c == 0xfe || c == 0xff )
				return 1;
		}
	}
	return 0;
}

static void uniconv(const char *filename)
{
	FILE * fd = fopen(filename,"rb");	// open file
	if (!fd) return;

	if (checkUnicode (fd)) {
		fseek(fd, 0, SEEK_END);
		long int len = ftell(fd);
		char * content = new char[len+1];
		if( content ) {
			rewind( fd );
			fread (content, 1, len, fd);
			fclose(fd);

			fd = fopen(filename,"wt");
			if (fd) {
				int i;
				for( i = 0; i < len; i++ ) {
					if( content[i] > 0 )
						fputc(content[i], fd);
				}
				fclose( fd );
			}
			delete [] content;
		}
	}
	else fclose(fd);
}

// ------------------------------------------------------------------------
GUIDOAPI(GuidoErrCode) GuidoParseFile(const char * filename, ARHandler * ar)
{
	if( !filename || !ar )	return guidoErrBadParameter;
	
	*ar = 0;

 	// first convert unicode files
	uniconv(filename);
 	// Check if file exists.
    ARHandler music = 0;
    
    GuidoParser *parser = GuidoOpenParser();
    music = GuidoFile2AR(parser, filename);
    GuidoCloseParser(parser);

	if (music == 0)
		return guidoErrMemory;

	// - Use the filename as the new music name
	music->armusic->setName( filename );

	*ar = music;

	return guidoNoErr;
}

// --------------------------------------------------------------------------
GUIDOAPI(GuidoErrCode) GuidoParseString (const char * str, ARHandler* ar)
{
	if( !str || !ar )	return guidoErrBadParameter;
	
	*ar = 0;

    ARHandler music = 0;

    GuidoParser *parser = GuidoOpenParser();
    music = GuidoString2AR(parser, str);
    GuidoCloseParser(parser);

	if (!music) // Something failed, do some cleanup
		return music ? guidoErrUserCancel : guidoErrParse;

	music->armusic->setName(""); // - Use the filename as the new music name

	*ar = music;
	return guidoNoErr;
}

#ifdef TESTTIMEMAP
#include "GUIDOScoreMap.h"
#include <iostream>

std::ostream& operator<< (std::ostream& os, const GuidoDate& date)
{
	os << date.num << "/" << date.denom;
	return os;
}

std::ostream& operator<< (std::ostream& os, const TimeSegment& s)
{
	os << s.first << " - " << s.second;
	return os;
}
// --------------------------------------------------------------------------
class TestTimeMap : public TimeMapCollector
{
	public:
		virtual ~TestTimeMap() {}
		virtual void Time2TimeMap( const TimeSegment& from, const TimeSegment& to ) {
			std:: cout << "TestTimeMap " << from << " -> " << to << std::endl;
		}
};
#endif

// --------------------------------------------------------------------------
GUIDOAPI(GuidoErrCode) GuidoAR2GR( ARHandler ar, const GuidoLayoutSettings * settings, GRHandler * gr)
{
	if( gr == 0 ) 	return guidoErrBadParameter;
	if( ar == 0 )	return guidoErrInvalidHandle;
	if( !gInited )	return guidoErrNotInitialized;

	*gr = 0;

#ifdef TESTTIMEMAP
	TestTimeMap timeCollector;
	GuidoGetTimeMap (ar, timeCollector);
#endif

	// - Find the AR object corresponding to inHandleAR.
	ARMusic * arMusic = ar->armusic; // (JB) was guido_PopARMusic()

	if (arMusic == 0)
		return guidoErrInvalidHandle;

	// - Now create the GRMusic object from  the abstract representation.
    long startTime = GuidoTiming::getCurrentmsTime();
	// Create new gr music object with a copy of default pageFormat.
	GRMusic *grMusic = new GRMusic(arMusic, new ARPageFormat(*gARPageFormat), settings, false);
    long endTime = GuidoTiming::getCurrentmsTime();

	if (grMusic == 0)
        return guidoErrMemory;
    else {
        grMusic->setAR2GRTime(endTime - startTime);

#ifdef TIMING
        long AR2GRTime = grMusic->getAR2GRTime();
        std::cerr << "  --> " << AR2GRTime << "ms spent during AR to GR procedure" << std::endl;
#endif
    }

	// - The GR structure needs its AR equivalent during all its lifetime.
	ar->refCount++;

	// - Propagate the music name
	grMusic->setName(arMusic->getName().c_str());

	//  - Add the GRMusic object to the global list
	GRHandler outHandleGR = guido_RegisterGRMusic(grMusic, ar);

	*gr = outHandleGR;

	return guidoNoErr;
}

GUIDOAPI(GRHandler) GuidoAR2GRParameterized(ARHandler ar, const GuidoGrParameters* gp)
{
	if( ar == 0 )	return 0;
	if( !gInited )	return 0;

#ifdef TESTTIMEMAP
	TestTimeMap timeCollector;
	GuidoGetTimeMap (ar, timeCollector);
#endif

	// - Find the AR object corresponding to inHandleAR.
	ARMusic * arMusic = ar->armusic; // (JB) was guido_PopARMusic()

	if (arMusic == 0)
		return 0;

	ARPageFormat * pf;
	const GuidoLayoutSettings * settings = 0;
	// Apply settings
	if(gp) {
		settings = &gp->layoutSettings;
		// A new page format is created.
		pf = new ARPageFormat(gp->pageFormat.width, gp->pageFormat.height,
							  gp->pageFormat.marginleft, gp->pageFormat.margintop,
							  gp->pageFormat.marginright, gp->pageFormat.marginbottom);
	} else {
		// Copy of the default page format.
		pf = new ARPageFormat(*gARPageFormat);
	}

	// - Now create the GRMusic object from  the abstract representation.
	long startTime = GuidoTiming::getCurrentmsTime();

	GRMusic *grMusic = new GRMusic(arMusic, pf, settings, false);
	long endTime = GuidoTiming::getCurrentmsTime();

	if (grMusic == 0)
		return 0;
	else {
		grMusic->setAR2GRTime(endTime - startTime);

#ifdef TIMING
		long AR2GRTime = grMusic->getAR2GRTime();
		std::cerr << "  --> " << AR2GRTime << "ms spent during AR to GR procedure" << std::endl;
#endif
	}

	// - The GR structure needs its AR equivalent during all its lifetime.
	ar->refCount++;

	// - Propagate the music name
	grMusic->setName(arMusic->getName().c_str());

	//  - Add the GRMusic object to the global list
	return guido_RegisterGRMusic(grMusic, ar);
}

// --------------------------------------------------------------------------
GUIDOAPI(GuidoErrCode) GuidoUpdateGR( GRHandler gr, const GuidoLayoutSettings * settings)
{
	if ( !gr )			return guidoErrInvalidHandle;
	if ( !gr->grmusic )	return guidoErrInvalidHandle;

	gr->grmusic->createGR(new ARPageFormat(*gARPageFormat), settings);
	return guidoNoErr;
}


GUIDOAPI(GuidoErrCode)	GuidoUpdateGRParameterized( GRHandler gr, const GuidoGrParameters* gp)
{
	if ( !gr )			return guidoErrInvalidHandle;
	if ( !gr->grmusic )	return guidoErrInvalidHandle;

	ARPageFormat * pf;
	const GuidoLayoutSettings * settings = 0;
	// Apply settings
	if(gp) {
		// A new page format is created.
		pf = new ARPageFormat(gp->pageFormat.width, gp->pageFormat.height,
							  gp->pageFormat.marginleft, gp->pageFormat.margintop,
							  gp->pageFormat.marginright, gp->pageFormat.marginbottom);
		settings = &gp->layoutSettings;
	} else {
		// Copy of the default page format.
		pf = new ARPageFormat(*gARPageFormat);
	}

	gr->grmusic->createGR(pf, settings);
	return guidoNoErr;
}

// --------------------------------------------------------------------------
GUIDOAPI(void)	GuidoFreeAR( ARHandler ar )
{
	if ( !ar ) return;

	ar->refCount -= 1;
	if( !ar->refCount )	// No more GR using this AR
	{
		delete ar->armusic;
		delete ar;
	}
}

// --------------------------------------------------------------------------
GUIDOAPI(void)	GuidoFreeGR (GRHandler gr)
{
	if( gr == 0 ) return;

	// The GR does not use its AR anymore, so we decrement the  
	// reference counter of the AR - and frees it if necessary.
	GuidoFreeAR( gr->arHandle );
    delete gr->grmusic;
    delete gr;
}

// --------------------------------------------------------------------------
GUIDOAPI(GuidoDate *) 	GuidoMakeDate( int num, int denom )
{
  GuidoDate *date =  new GuidoDate();
  date->num = num;
  date->denom = denom;
  return date;
}

// --------------------------------------------------------------------------

GUIDOAPI(const char *) GuidoGetErrorString( GuidoErrCode errCode )
{
	switch( errCode )
	{
		case guidoNoErr:			return "No error";
		case guidoErrParse:			return "Parse error";
		case guidoErrMemory:		return "Memory error";
		case guidoErrFileAccess:	return "File error";
		case guidoErrUserCancel:	return "Canceled by user";
		case guidoErrNoMusicFont:	return "Music font not found";
		case guidoErrNoTextFont:	return "Text font not found";
		case guidoErrBadParameter:	return "Invalid parameter";
		case guidoErrInvalidHandle:	return "Invalid Guido handle";
		case guidoErrNotInitialized:return "Guido was not initialized before parse";
		case guidoErrActionFailed: 	return "Action failed";
		default: return "Unknown internal error";
	}
}

// --------------------------------------------------------------------------
// this function is obsolete with the new reentrant parser
// syntax error line and columns should be retrieved from the parser
GUIDOAPI(int) GuidoGetParseErrorLine()
{
	return gParseErrorLine;
}


// --------------------------------------------------------------------------
GUIDOAPI(void)	
GuidoGetDefaultLayoutSettings (GuidoLayoutSettings *settings)
{
	settings->systemsDistance       = kSettingDefaultSystemDistance;
 	settings->systemsDistribution   = kSettingDefaultSystemDistrib;
 	settings->systemsDistribLimit   = kSettingDefaultDistribLimit;
	settings->force                 = kSettingDefaultForce;
    settings->spring                = kSettingDefaultSpring;
	settings->neighborhoodSpacing   = kSettingDefaultNeighborhood;
	settings->optimalPageFill       = kSettingDefaultOptimalPageFill;
    settings->resizePage2Music      = kSettingDefaultResizePage2Music;
    settings->proportionalRenderingForceMultiplicator = kSettingDefaultProportionalRendering;
}

// --------------------------------------------------------------------------
//		- Browsing music pages -
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
GUIDOAPI(int) GuidoCountVoices( CARHandler inHandleAR)
{
	if ((!inHandleAR) || ( inHandleAR->armusic == 0 ))
		return guidoErrInvalidHandle;
		
	return inHandleAR->armusic->countVoices();
}

// --------------------------------------------------------------------------
GUIDOAPI(int) GuidoGetPageCount( CGRHandler inHandleGR )
{
	if ( !inHandleGR )
		return guidoErrInvalidHandle;
	return inHandleGR->grmusic ? inHandleGR->grmusic->getNumPages() : guidoErrInvalidHandle;
}

// --------------------------------------------------------------------------
GUIDOAPI(int) GuidoGetSystemCount( CGRHandler inHandleGR, int page )
{
	if ( !inHandleGR )
		return guidoErrInvalidHandle;
	return inHandleGR->grmusic ? inHandleGR->grmusic->getNumSystems(page) : guidoErrInvalidHandle;
}

// --------------------------------------------------------------------------
GUIDOAPI(GuidoErrCode)	GuidoDuration( CGRHandler inHandleGR, GuidoDate * date )
{
	if( date == 0 )			return guidoErrBadParameter;
	if ( inHandleGR == 0 )	return guidoErrBadParameter;

	const GRMusic * grMusic = inHandleGR->grmusic;
	if( grMusic )
	{
		const TYPE_DURATION & dur = grMusic->getDuration();
	
		date->num = dur.getNumerator();
		date->denom = dur.getDenominator();
        return guidoNoErr;
	}
	else
	{
		date->num = 0;
		date->denom = 1;
		return guidoErrInvalidHandle;
	}
}

// --------------------------------------------------------------------------
// was GuidoGetPageNum
GUIDOAPI(int) GuidoFindEventPage( CGRHandler inHandleGR, const GuidoDate & date )
{
	if ( !inHandleGR )
		return 0;
	return inHandleGR->grmusic ? inHandleGR->grmusic->getPageNum(date.num, date.denom) : 0;
}

// --------------------------------------------------------------------------
// was GuidoFindPageNumForDate
GUIDOAPI(int) GuidoFindPageAt( CGRHandler inHandleGR, const GuidoDate & date )
{
	if ( !inHandleGR )
		return 0;
	return inHandleGR->grmusic ? inHandleGR->grmusic->getPageNumForTimePos( date.num, date.denom ) : 0;
}

// --------------------------------------------------------------------------
// was GuidoGetPageRTP
GUIDOAPI(GuidoErrCode) GuidoGetPageDate( CGRHandler inHandleGR, int pageNum, GuidoDate * date)
{
	date->num = 0;
	date->denom = 1;

	if ( ( !inHandleGR ) || ( !inHandleGR->grmusic ) )
		return guidoErrInvalidHandle;

	bool result = inHandleGR->grmusic->getRTPofPage( pageNum, &date->num, &date->denom );
	return result ? guidoNoErr : guidoErrBadParameter;
}

// --------------------------------------------------------------------------
//		- Score drawing and pages formating -
// --------------------------------------------------------------------------

GUIDOAPI(GuidoErrCode) GuidoOnDraw( GuidoOnDrawDesc * desc )
{
	if( desc == 0 ) return guidoErrBadParameter;
  	if( desc->hdc == 0 ) return guidoErrBadParameter;
  	if( desc->page <= 0 ) return guidoErrBadParameter;
  	if( desc->handle == 0 ) return guidoErrInvalidHandle;
  	if( desc->handle->grmusic == 0 ) return guidoErrInvalidHandle;
 
  	GuidoErrCode result = guidoErrActionFailed;
 	
    long startTime = GuidoTiming::getCurrentmsTime();

	const bool drawReady = desc->hdc->BeginDraw();

	if(drawReady) {
		//if (desc->page <= 0)
		//	desc->page = iter->page;

		// - Ready to Draw
#if 0	// test color settings
		VGColor myTestColor(200,120,120);
		desc->hdc->SelectPenColor(myTestColor);
		desc->hdc->SelectFillColor(myTestColor);
		desc->hdc->SetFontColor(myTestColor);
#endif
		desc->handle->grmusic->OnDraw( *(desc->hdc), *desc );

		result = guidoNoErr;
	}

    /**** MAPS ****/

    if (SVGMapDevice *mapDevice = dynamic_cast<SVGMapDevice *>(desc->hdc)) {
        int voicesNumber = GuidoCountVoices(desc->handle->arHandle);

        for (int i = 1; i <= voicesNumber; i++) {
            Time2GraphicMap voiceMap;
            Time2GraphicMap staffMap;
            
            result = GuidoGetVoiceMap(desc->handle, desc->page, (float) desc->sizex, (float) desc->sizey, i, voiceMap);
            result = GuidoGetStaffMap(desc->handle, desc->page, (float) desc->sizex, (float) desc->sizey, i, staffMap);

            mapDevice->addVoiceMap(voiceMap);
            mapDevice->addStaffMap(staffMap);
        }

        Time2GraphicMap systemMap;
        result = GuidoGetSystemMap(desc->handle, desc->page, (float) desc->sizex, (float) desc->sizey, systemMap);
        mapDevice->addSystemMap(systemMap);
    }

    /**************/
		
	desc->hdc->EndDraw(); // must be called even if BeginDraw has failed.
    
    long endTime = GuidoTiming::getCurrentmsTime();

    desc->handle->grmusic->setDrawTime(endTime - startTime);

#ifdef TIMING
    long drawingTime = desc->handle->grmusic->getDrawTime();
    std::cerr << "  --> " << drawingTime << "ms spent for GR drawing" << std::endl;
#endif

	return result;
}

// --------------------------------------------------------------------------
//		- Score export to an abstract graphical representation -
// --------------------------------------------------------------------------
GUIDOAPI(GuidoErrCode) 	GuidoAbstractExport( const GRHandler handle, int page, std::ostream& out)
{
 	AbstractSystem sys;
	AbstractDevice dev (out, &sys);
    
    GuidoOnDrawDesc desc;              // declare a data structure for drawing
    desc.handle = handle;

    GuidoPageFormat	pf;
    GuidoResizePageToMusic (handle);
    GuidoGetPageFormat (handle, page, &pf);

    desc.hdc = &dev;                    // we'll draw on the svg device
    desc.page = page;
    desc.updateRegion.erase = true;     // and draw everything
    desc.scrollx = desc.scrolly = 0;    // from the upper left page corner
    desc.sizex = (int) pf.width;
    desc.sizey = (int) pf.height;
    dev.NotifySize(desc.sizex, desc.sizey);
    dev.SelectPenColor(VGColor(0,0,0));

    return GuidoOnDraw (&desc);
}

// --------------------------------------------------------------------------
//		- Score export to a Binary representation -
// --------------------------------------------------------------------------
GUIDOAPI(GuidoErrCode) 	GuidoBinaryExport( const GRHandler handle, int page, std::ostream& out)
{
 	BinarySystem sys;
	BinaryDevice dev (out, &sys);
    
    GuidoOnDrawDesc desc;              // declare a data structure for drawing
    desc.handle = handle;

	GuidoPageFormat	pf;
	GuidoResizePageToMusic (handle);
	GuidoGetPageFormat (handle, page, &pf);
 
    desc.hdc = &dev;                    // we'll draw on the binary device
        desc.page = page;
        desc.updateRegion.erase = true;     // and draw everything
	desc.scrollx = desc.scrolly = 0;    // from the upper left page corner
    desc.sizex = (int) pf.width;
    desc.sizey = (int) pf.height;
        dev.NotifySize(desc.sizex, desc.sizey);
        dev.SelectPenColor(VGColor(0,0,0));
        return GuidoOnDraw (&desc);
}

// --------------------------------------------------------------------------
//		- Score export to svg -
// --------------------------------------------------------------------------
GUIDOAPI(GuidoErrCode) GuidoGR2SVG( const GRHandler handle, int page, std::ostream& out, bool embedFont, const char* font, const int mappingMode )
{
	const char * fontUsed = font;
	if(embedFont) {
		fontUsed = ______src_guido2_svg;
	}

	SVGSystem sys(fontUsed);
	VGDevice    *dev    = sys.CreateDisplayDevice(out, mappingMode);

	GuidoOnDrawDesc desc;              // declare a data structure for drawing
	desc.handle = handle;

	GuidoPageFormat	pf;
	GuidoResizePageToMusic (handle);
	GuidoGetPageFormat (handle, page, &pf);

	desc.hdc = dev;                    // we'll draw on the svg device
	desc.page = page;
	desc.updateRegion.erase = true;     // and draw everything
	desc.scrollx = desc.scrolly = 0;    // from the upper left page corner
	desc.sizex = int(pf.width/SVGDevice::kSVGSizeDivider);
	desc.sizey = int(pf.height/SVGDevice::kSVGSizeDivider);
	dev->NotifySize(desc.sizex, desc.sizey);
	dev->SelectPenColor(VGColor(0,0,0));

	GuidoErrCode error = GuidoOnDraw (&desc);

	delete dev;
	return error;
}

GUIDOAPI(GuidoErrCode) GuidoSVGExport( const GRHandler handle, int page, std::ostream& out, const char* fontfile, const int mappingMode )
{
    return GuidoSVGExportWithFontSpec( handle, page, out, fontfile, 0, mappingMode);
}

GUIDOAPI(GuidoErrCode) GuidoSVGExportWithFontSpec(const GRHandler handle, int page, std::ostream& out, const char* fontfile, const char* fontspec, const int mappingMode )
{
	const char * font = fontspec;
	unsigned int size;
	if(fontfile != 0 && (size = strlen(fontfile)) > 0 && size <= 260)
		font = fontfile;
	return GuidoGR2SVG( handle, page, out, false, font, mappingMode);
}

// --------------------------------------------------------------------------
GUIDOAPI(void) 	GuidoDrawBoundingBoxes(int bbMap)	{ gBoundingBoxesMap = bbMap; }
GUIDOAPI(int) 	GuidoGetDrawBoundingBoxes()			{ return gBoundingBoxesMap; }

// --------------------------------------------------------------------------
GUIDOAPI(void) 	GuidoSetDefaultPageFormat( const GuidoPageFormat * inFormat)
{
	delete gARPageFormat;
	gARPageFormat = 0;

	if( inFormat )
	{
		const GuidoPageFormat & pf = *inFormat;
		gARPageFormat = new ARPageFormat( pf.width, pf.height,
											pf.marginleft, pf.margintop,
											pf.marginright, pf.marginbottom );

	}
}

// --------------------------------------------------------------------------
// was GuidoGetDefaultPageSizeCm
// Works in Guido internal units.
GUIDOAPI(void) 	GuidoGetDefaultPageFormat( GuidoPageFormat * outFormat )
{
	if( outFormat == 0 ) return;

	float osx, osy, oml, omt, omr, omb;
	if (gARPageFormat)
	{
		gARPageFormat->getPageFormat( &osx, &osy, &oml, &omt, &omr, &omb );
	}
	else
	{
		osx = DF_SX * kCmToVirtual;
		osy = DF_SY * kCmToVirtual;
		oml = DF_ML * kCmToVirtual;
		omr = DF_MR * kCmToVirtual;
		omt = DF_MT * kCmToVirtual;
		omb = DF_MB * kCmToVirtual;
	}

	outFormat->width = osx;
	outFormat->height = osy;
	outFormat->marginleft = oml;
	outFormat->margintop = omt;
	outFormat->marginright = omr;
	outFormat->marginbottom = omb;
}

// --------------------------------------------------------------------------
// // in internal units
GUIDOAPI(void) 	GuidoGetPageFormat(	CGRHandler inHandleGR, int pageNum, GuidoPageFormat* format )
{
	if ( inHandleGR == 0 ) return;
	if( inHandleGR->grmusic == 0 ) return;
	if( format == 0 ) return;

	const GRPage * thePage = inHandleGR->grmusic->getPage( pageNum );
	if( thePage )
		thePage->getPageFormat( format );
}

// --------------------------------------------------------------------------
GUIDOAPI(float) GuidoUnit2CM(float val)
{
	return val * kVirtualToCm;
}

// --------------------------------------------------------------------------
GUIDOAPI(float) GuidoCM2Unit(float val)
{
	return val * kCmToVirtual;
}

// --------------------------------------------------------------------------
GUIDOAPI(float) GuidoUnit2Inches(float val)
{
	return val * kVirtualToInch;
}

// --------------------------------------------------------------------------
GUIDOAPI(float) GuidoInches2Unit(float val)
{
	return val * kInchToVirtual;
}

// --------------------------------------------------------------------------
GUIDOAPI(GuidoErrCode) 	GuidoResizePageToMusic( GRHandler inHandleGR )
{
	if ( (!inHandleGR) || ( inHandleGR->grmusic == 0 ))
	{
		return guidoErrInvalidHandle;
	}
	inHandleGR->grmusic->adjustPageSize();
	return guidoNoErr;
}

// --------------------------------------------------------------------------
//		- Miscellaneous -
// --------------------------------------------------------------------------

GUIDOAPI(void) GuidoGetVersionNums(int * major, int * minor, int * sub)
{
	*major = GUIDOENGINE_MAJOR_VERSION;
	*minor = GUIDOENGINE_MINOR_VERSION;
	*sub   = GUIDOENGINE_SUB_VERSION;
}

GUIDOAPI(const char*) GuidoGetVersionStr()
{
	return GUIDOENGINE_VERSION_STR;
}

// --------------------------------------------------------------------------
GUIDOAPI(GuidoErrCode) GuidoCheckVersionNums(int major, int minor, int sub)
{
  if((GUIDOENGINE_MAJOR_VERSION > major) || 
     ((GUIDOENGINE_MAJOR_VERSION == major) && (GUIDOENGINE_MINOR_VERSION > minor)) || 
     ((GUIDOENGINE_MAJOR_VERSION == major) && (GUIDOENGINE_MINOR_VERSION == minor) && (GUIDOENGINE_SUB_VERSION >= sub)))
    return guidoNoErr;
  
  return guidoErrActionFailed; // was -99
}


// --------------------------------------------------------------------------
// was GuidoGetLSPACE
GUIDOAPI(float) GuidoGetLineSpace()
{
	return (float)LSPACE;
}

// --------------------------------------------------------------------------
GUIDOAPI(GuidoErrCode) GuidoMarkVoice( ARHandler inHandleAR, int voicenum, 
										const GuidoDate & date, const GuidoDate & duration, 
										unsigned char red, unsigned char green, unsigned char blue )
{
	// TODO: color implementation (currently always red).
	if ((!inHandleAR) || ( inHandleAR->armusic == 0 ))
		return guidoErrInvalidHandle;

	inHandleAR->armusic->MarkVoice( voicenum, date.num, date.denom, 
                                    duration.num, duration.denom,
                                    red, green, blue );
	return guidoNoErr;
}

// --------------------------------------------------------------------------
GUIDOAPI(GuidoErrCode) GuidoSetSymbolPath(ARHandler inHandleAR, const std::vector<std::string> &inPaths)
{
    if (!inHandleAR)
        return guidoErrInvalidHandle;

    if (!inHandleAR->armusic)
        return guidoErrInvalidHandle;

    inHandleAR->armusic->setPath(inPaths);

    return guidoNoErr;
}

// --------------------------------------------------------------------------
GUIDOAPI(GuidoErrCode) GuidoGetSymbolPath(const ARHandler inHandleAR, std::vector<std::string> &inPathVector)
{
    if (!inHandleAR)
        return guidoErrInvalidHandle;

    if (!inHandleAR->armusic)
        return guidoErrInvalidHandle;

    inPathVector = inHandleAR->armusic->getPath();

    return guidoNoErr;
}

// --------------------------------------------------------------------------
GUIDOAPI(long)  GuidoGetParsingTime (const ARHandler ar)
{
    ARMusic *arMusic = ar ? ar->armusic : 0;
	return arMusic ? arMusic->getParseTime() : -1;
}

// --------------------------------------------------------------------------
GUIDOAPI(long) 	GuidoGetAR2GRTime(const GRHandler gr)
{
    GRMusic *grMusic = gr ? gr->grmusic : 0;
    return grMusic ? grMusic->getAR2GRTime() : -1;
}

// --------------------------------------------------------------------------
GUIDOAPI(long) 	GuidoGetOnDrawTime(const GRHandler gr)
{
    GRMusic *grMusic = gr ? gr->grmusic : 0;
    return grMusic ? grMusic->getDrawTime() : -1;
}

