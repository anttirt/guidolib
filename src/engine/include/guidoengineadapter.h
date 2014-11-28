#ifndef GUIDOENGINEADAPTER_H
#define GUIDOENGINEADAPTER_H

#include <iostream>
#include "GUIDOParse.h"

using namespace std;


/*!
 * \brief A structure for parser error.
 */
typedef struct {
	//! line a reference that will contain a line number in case of syntax error
	int line;
	//!	col a reference that will contain a column number in case of syntax error
	int col;
	//!	msg a string that will contain the error message
	string msg;
} ParserError;

/*!
 * \brief A structure for guido version, composed of 3 digit.
 */
typedef struct {
	//! major major number version
	int major;
	//! minor minor number version
	int minor;
	//! sub sub number version
	int sub;
	//! String representation of guido version
	string str;
} GuidoVersion;

/*!
 * \brief The GuidoEngineAdapter class
 * A C++ class to manupilate GuidoEngine.
 */
class GuidoEngineAdapter
{
    public:
		/*!
		 * \brief GuidoEngineAdapter
		 */
		GuidoEngineAdapter();
		/*!
		 * \brief ~GuidoEngineAdapter
		 */
		virtual ~GuidoEngineAdapter();


		/*!
			\addtogroup Engine Building abstract and graphic representations
			@{
		*/
		/*!
			Initialises the Guido Engine. Must be called before any attempt
			to read a Guido file or to use the Guido Factory
			\param desc the graphic environment description.
			\return a Guido error code.

			WARNING: the caller must ensure desc maintains a constant reference on a
			valid VGDevice, because Guido keeps it internally (to calculate fonts, etc.)
		*/
		GUIDOAPI(GuidoErrCode)	guidoInit(GuidoInitDesc * desc);

		/*!
		 * \brief Initialise the Guido Engine with a SVGSystem.
		 *
		 */
		GUIDOAPI(void) guidoInitWithIndependentSVG();

		/*!
			Guido Engine shutdown

			Actually release the font allocated by the engine.
			Anyway, the fonts are release when the client application exit but
			the function provides control over the time of the release.
		*/
		GUIDOAPI(void)	guidoShutdown();

		/*!
			Transforms a Guido abstract representation into a Guido graphic representation.
			The engine applies layout algorithms according to the settings given as argument.
			Default settings are applied.
			\note You can safely free the AR after the transformation.

			\param ar the handler to the abstract representation.
			\return a Guido opaque handle to a graphic music representation.
					It's the caller responsability to free the handle using GuidoFreeGR.
		*/
		GUIDOAPI(GRHandler) guidoAR2GR(ARHandler ar);

		/*!
			Transforms a Guido abstract representation into a Guido graphic representation.
			The engine applies layout algorithms according to the settings given as argument.

			\note You can safely free the AR after the transformation.

			\param ar the handler to the abstract representation.
			\param settings a pointer to the settings for the graphic layout.
			\return a Guido opaque handle to a graphic music representation.
					It's the caller responsability to free the handle using GuidoFreeGR.
		*/
		GUIDOAPI(GRHandler) guidoAR2GR(ARHandler ar, const GuidoLayoutSettings &settings);

		/*!
			Applies new layout settings to an existing Guido graphic representation.
			Default	settings are applied.
			\param gr the handler to the graphic representation.
			\return a Guido error code.
		*/
		GUIDOAPI(GuidoErrCode) guidoUpdateGR(GRHandler gr);

		/*!
			Applies new layout settings to an existing Guido graphic representation.
			\param gr the handler to the graphic representation.
			\param settings the settings for the graphic layout.
			\return a Guido error code.
		*/
		GUIDOAPI(GuidoErrCode) guidoUpdateGR(GRHandler gr, const GuidoLayoutSettings &settings);

		/*!
			Releases a Guido abstract representation.
			\param ar the handler to the abstract representation.
		*/
		GUIDOAPI(void) guidoFreeAR(ARHandler ar);

		/*!
			Releases a Guido graphic representation.
			\param gr the handler to the graphic representation.
		*/
		GUIDOAPI(void) guidoFreeGR(GRHandler gr);

		/*!
			Gives a textual description of a Guido error code.

			\param errCode a Guido error code.
			\return a string describing the error.
		*/
		GUIDOAPI(string) guidoGetErrorString(GuidoErrCode errCode);

		/*!
			Gives the default values of the layout settings.

			\param settings on output, a pointer to the settings to be filled with default values.
		*/
		GUIDOAPI(GuidoLayoutSettings) guidoGetDefaultLayoutSettings();
		/*! @} */

		/*!
			\addtogroup Pages Browsing music pages
			The Guido Engine produces pages of music and therefore, the graphic representation
			consists in a collection of pages.
			The following functions are intended to access these pages by page number as well
			as by date. Page numbers start at 1.
		@{
		*/

		/** \brief Gives the number of score pages of the graphic representation.

			\param inHandleAR a Guido opaque handle to a AR structure.
			\return the number of voices or a guido error code.
		*/
		GUIDOAPI(int) guidoCountVoices(CARHandler inHandleAR);

		/** \brief Gives the number of score pages of the graphic representation.

			\param inHandleGR a Guido opaque handle to a GR structure.
			\return a number of pages or a guido error code.
		*/
		GUIDOAPI(int) guidoGetPageCount(CGRHandler inHandleGR);

		/** \brief Gives the number of systems on a given page.

			\param inHandleGR a Guido opaque handle to a GR structure.
			\param page a page number (starts at 1).
			\return the systems count on the given page or a guido error code.
		*/
		GUIDOAPI(int) guidoGetSystemCount(CGRHandler inHandleGR, int page);

		/** \brief Returns the music duration of a score.

			The duration is expressed as a fractional value where 1 represents
			a whole note.

			\param inHandleGR a Guido opaque handle to a GR structure.
			\return the duration expressed as a fractional value.
		*/
		GUIDOAPI(GuidoDate) guidoDuration(CGRHandler inHandleGR);

		/** \brief Finds the page which has an event (note or rest) at a given date.

			\bug returns page + 1 when input date falls on the last system.
			\param inHandleGR a Guido opaque handle to a GR structure.
			\param date the target date.
			\return a page number if greater than 0,
					0 if no page found,
		*/
		GUIDOAPI(int) guidoFindEventPage(CGRHandler inHandleGR, const GuidoDate& date);

		/** \brief Finds the page which contain a given date.

			\bug returns page + 1 when input date falls on the last system.
			\param inHandleGR a Guido opaque handle to a GR structure.
			\param date the target date.
			\return a page number if greater than 0,
					0 if no page found,
		*/
		GUIDOAPI(int) guidoFindPageAt(CGRHandler inHandleGR, const GuidoDate& date);

		/** \brief Gives the time location of a Page.

			\param inHandleGR a Guido opaque handle to a GR structure.
			\param pageNum a page number (starts at 1).
			\param date on output: the page date if the page number is valid
			\return a Guido error code.
		*/
		GUIDOAPI(GuidoDate) guidoGetPageDate(CGRHandler inHandleGR, int pageNum);
		/*! @} */

		/*!
			\addtogroup Format Score drawing and pages formating
			The GuidoEngine makes use of internal units for graphic operations.
			The functions that query or set graphic dimensions always makes use of
			this internal unit. Conversion functions are provided to convert to standard
			units.
		@{
		*/

		/** \brief Draws one page of score into a graphic device.

			\param desc informations about what to draw and how to draw.
			\return a Guido error code
		*/
		GUIDOAPI(GuidoErrCode) guidoOnDraw(GuidoOnDrawDesc * desc);

		/** \brief Exports one page of score to SVG.

			\param a graphic representation.
			\param page the page number.
			\param out the output stream.
			\param fontfile path of the guido svg font file.
			\param fontspec an actual svg font if there is no font file.
			\return a Guido error code
		*/
		GUIDOAPI(GuidoErrCode) guidoSVGExport(const GRHandler handle, int page, std::ostream& out, const char* fontfile = 0, const char* fontspec = 0);

		/** \brief Exports one page of score to SVG.
			Embedded font spec is use for the export.

			\param a graphic representation.
			\param page the page number.
			\return the export in a string.
		*/
		GUIDOAPI(string) guidoSVGExport(const GRHandler handle, int page);

		/** \brief Exports an abstract representation of GUIDO draw commands.

			\param a graphic representation.
			\param page the page number.
			\param out the output stream.
			\return a Guido error code
		*/
		GUIDOAPI(GuidoErrCode) guidoAbstractExport(const GRHandler handle, int page, std::ostream& out);

		/** \brief Exports an abstract representation of GUIDO draw commands.

			\param a graphic representation.
			\param page the page number.
			\return the export in a string
		*/
		GUIDOAPI(string) guidoAbstractExport(const GRHandler handle, int page);

		/** \brief Exports an representation of GUIDO draw commands in a data-reduced dsl

			\param a graphic representation.
			\param page the page number.
			\param out the output stream.
			\return a Guido error code
		*/
		GUIDOAPI(GuidoErrCode) guidoBinaryExport(const GRHandler handle, int page, std::ostream& out);

		/** \brief Exports an representation of GUIDO draw commands in a data-reduced dsl

			\param a graphic representation.
			\param page the page number.
			\return a Guido error code
		*/
		GUIDOAPI(string) guidoBinaryExport(const GRHandler handle, int page);

		/** \brief Control bounding boxes drawing.

			\param bbMap a bits field indicating the set of bounding boxes to draw (default to none).
		*/
		GUIDOAPI(void) guidoSetDrawBoundingBoxes(int bbMap);

		/** \brief Gives bounding boxes drawing state.
		 *	\return the bit field.
		*/
		GUIDOAPI(int) guidoGetDrawBoundingBoxes();

		/** \brief Gives a score page format.

			\param inHandleGR a Guido opaque handle to a GR structure.
			\param pageNum a page number.
			\return the page format
		*/
		GUIDOAPI(GuidoPageFormat) guidoGetPageFormat(CGRHandler inHandleGR, int pageNum);

		/** \brief Sets the default score page format.

			The default page format is used when no \p \\pageFormat tag is present.
			Parameters are Guido internal units. Default values for the default page
			format are:
			- paper size: A4
			- left margin: 2cm
			- right margin: 2cm
			- top margin: 5cm
			- bottom margin: 3cm

			\param format the page format
		*/
		GUIDOAPI(void) guidoSetDefaultPageFormat(const GuidoPageFormat &format);

		/** \brief Gives the default score page format.

			\return the page format
		*/
		GUIDOAPI(GuidoPageFormat) guidoGetDefaultPageFormat();


		/** \brief Converts internal Guido units into centimeters.

			\param val the value to be converted
			\return the converted value
		*/
		GUIDOAPI(float) guidoUnit2CM(float val);

		/** \brief Converts centimeters into internal Guido units.

			\param val the value to be converted
			\return the converted value
		*/
		GUIDOAPI(float) guidoCM2Unit(float val);

		/** \brief Converts internal Guido units into inches.

			\param val the value to be converted
			\return the converted value
		*/
		GUIDOAPI(float) guidoUnit2Inches(float val);

		/** \brief Converts inches into internal Guido units.

			\param val the value to be converted
			\return the converted value
		*/
		GUIDOAPI(float) guidoInches2Unit(float val);

		/** \brief Resize the page sizes to the music size.

			\param inHandleGR a Guido opaque handle to a GR structure.
			\return a Guido error code.
		*/
		GUIDOAPI(GuidoErrCode) guidoResizePageToMusic( GRHandler inHandleGR );
		/*! @} */


		/*!
		\addtogroup Misc Miscellaneous
		Includes various functions for version management and for conversions.
		The number of version functions is due to historical reasons.
		@{
		*/
		/**	\brief Gives the library version number as three integers.

			Version number format is  MAJOR.MINOR.SUB

			\return a GuidoVersion structure.
		*/
		GUIDOAPI(GuidoVersion) guidoGetVersion();

		/**	\brief Checks a required library version number.

			\param major the major revision number.
			\param minor the minor revision number.
			\param sub the sub revision number.
			\return noErr if the library version number is greater or equal to the version number
				passed as argument.
			\return otherwise guidoErrActionFailed.
		*/
		GUIDOAPI(GuidoErrCode) guidoCheckVersionNums(int major, int minor, int sub);


		/** \brief Gives the distance between two staff lines.

			This value is constant (= 50). It does not depend on the context, it will
			probably never change in future versions of the library.

			\return the distance between two lines of staff, in Guido internal units.
		*/
		GUIDOAPI(float) guidoGetLineSpace();


		/** \brief Gives a color to all notes of a voice between a given time interval.

			\note Introduced for GUIDO/MIR; Allows the user to see where
					a musical theme appears in a voice.

			\param inHandleAR a Guido opaque handle to an AR structure.
			\param voicenum index of the voice to mark, starting from 1
			\param date the date where the color-marking must begin (whole note = 1)
			\param duration the duration that must be covered by the color marking.
			\param red the red component of the marking color, from 0 to 255.
			\param green green color component.
			\param blue blue color component.
			\return a Guido error code.
		*/
		GUIDOAPI(GuidoErrCode) guidoMarkVoice( ARHandler inHandleAR, int voicenum,
												const GuidoDate & date, const GuidoDate & duration,
												unsigned char red, unsigned char green, unsigned char blue );


		/**	\brief Makes the correspondance between an ARMusic and a path.

			\param inHandleAR the destination ARHandler.
			\param inPath the path to associate.
			\return noErr if the association has been made with success
			\return otherwise guidoErrActionFailed.
		*/
		GUIDOAPI(GuidoErrCode) guidoSetSymbolPath(ARHandler inHandleAR, const std::vector<std::string> &inPaths);


		/**	\brief Returns the path corresponding to an AR.

			\param inHandleAR the handle given to extract its path.
			\return the returned path.
			\return noErr if the association has been made with success
			\return otherwise guidoErrActionFailed.
		*/
		GUIDOAPI(GuidoErrCode) guidoGetSymbolPath(const ARHandler inHandleAR, std::vector<std::string> &inPathVector);

		/*! @} */


		/*!
		\addtogroup Guido timer
		@{
		*/
		/*!
			\brief Gets parsing time
			\param ar the ar handler given to extract the parsing time
			\return the time spent on building the AR representation (in msl) or -1 for invalid handlers
		*/
		GUIDOAPI(long) guidoGetParsingTime (const ARHandler ar);

		/** \brief Gets AR to GR procedure time

			\param gr the gr handler given to extract the AR2GR time
			\return the time spent to convert the AR representation to GR (in msl) or -1 for invalid handlers
		*/
		GUIDOAPI(long) guidoGetAR2GRTime(const GRHandler gr);

		/** \brief Gets GR drawing procedure time

			\param gr the gr handler given to extract the drawing time
			\return the time spent on the last OnDraw call (in msl) or -1 if OnDraw has not yet been called or for invalid handlers
		*/
		GUIDOAPI(long) guidoGetOnDrawTime(const GRHandler gr);

		/*! @} */
		/*!
		\addtogroup Guido parser
		@{
		*/
		/*!
			\brief Creates a new parser
			\return a guido parser.
		*/
		GUIDOAPI(GuidoParser *) guidoOpenParser();

		/*!
			\brief Close a guido parser and releases all the associated ressources
			\param p a parser previously opened with GuidoOpenParser
			\return a Guido error code.
		*/
		GUIDOAPI(GuidoErrCode) guidoCloseParser(GuidoParser *p);

		/*!
			\brief Parse a file and create the corresponding AR
			\param p a parser previously opened with GuidoOpenParser
			\param file the file to parse.
			\return a ARHandler or 0 in case of error.
		*/
		GUIDOAPI(ARHandler)	guidoFile2AR(GuidoParser *p, const string &file);

		/*!
			\brief Parse a string and create the corresponding AR
			\param p a parser previously opened with GuidoOpenParser
			\param gmnCode the string to parse.
			\return a ARHandler or 0 in case of error.
		*/
		GUIDOAPI(ARHandler) guidoString2AR(GuidoParser *parser, const string &gmnCode);

		/*!
			\brief returns the string of the GuidoStream
			\param gStream a GuidoStream
			\return a std::string.
		*/
		GUIDOAPI(string) guidoGetStream(GuidoStream *gStream);

		/*!
			\brief Parse a GuidoStream and create the corresponding AR

			\param p a parser previously opened with GuidoOpenParser
			\param stream the GuidoStream to parse.
			\return a ARHandler or 0 in case of error.
		*/
		GUIDOAPI(ARHandler) guidoStream2AR(GuidoParser *p, GuidoStream* stream);

		/*!
			\brief Get the error syntax line/column/message
			\param p a parser previously opened with GuidoOpenParser
			\return a ParserError structure.
		*/
		GUIDOAPI(ParserError) guidoParserGetErrorCode(GuidoParser *p);

		/*!
			\brief Open a guido stream

			Guido streams are intended to implement real-time input to the parser.
			In particular, streams allow to retrieve an AR in while the stream is still opened.
			\return a guido stream.
		*/
		GUIDOAPI(GuidoStream *) guidoOpenStream ();

		/*!
			\brief Close a guido stream
			\param s a GuidoStream
			\return a Guido error code.
		*/
		GUIDOAPI(GuidoErrCode) guidoCloseStream (GuidoStream *s);

		/*!
			\brief Write data to the stream

			Writing data to a stream may be viewed as writing gmn code by portion.
			Syntax errors concerning music/voice/tag/event/parameter non-closure won't be declared
			as such (GuidoWriteStream uses an automatic-closure mechanism).
			When a syntax error (other than a non-closure) occurs when writting data to the stream, the stream becomes invalid
			and should be closed. Further attempts to write data will always result in a syntax error.

			Regarding syntax errors, allowed incomplete constructs are :
				- opened music i.e. { without closing }
				- opened voice i.e. [ without closing ]
				- opened range tag i.e. ( without closing )
				- opened range parameter i.e. < without closing > but with at least one parameter
				- opened chord i.e. ( without closing ) but with at least one note
			\note for incomplete chords and range parameters, the ',' separator must always be followed by a note or a parameter.
			For example, don't write "{a," and then "b}" but "{a" and then ",b}".

			\param s a GuidoStream previoulsy opened with GuidoOpenStream
			\param str a string containing a portion of gmn code
			\return a Guido error code.
		*/
		GUIDOAPI(GuidoErrCode) guidoWriteStream (GuidoStream *s, const string &str);

		/*!
			\brief Erase all stream content in order to reuse it

			\param s a GuidoStream previoulsy opened with GuidoOpenStream
			\return a Guido error code.
		*/
		GUIDOAPI(GuidoErrCode) guidoResetStream (GuidoStream *s);

		/*! @} */

	private :
		static int nbinstantiation;
};

#endif // GUIDOENGINEADAPTER_H
