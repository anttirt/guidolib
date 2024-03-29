/*
  GUIDO Library
  Copyright (C) 2014 Grame

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr

*/

#include <math.h>

#include "ARMusic.h"
#include "ARRest.h"
#include "ARNoteFormat.h"
#include "ARChordComma.h"
#include "ARBar.h"
#include "GUIDOEngine.h"
#include "MusicalSymbols.h"
#include "PianoRollTrajectory.h"
#include "VGDevice.h"
#include "VGSystem.h"
#include "VGFont.h"
#include "TagParameterString.h"

using namespace std;

//--------------------------------------------------------------------------
PianoRollTrajectory::PianoRollTrajectory(ARMusic *arMusic) :
    PianoRoll(arMusic), fCurrentDate(0)
{
    init();
}

//--------------------------------------------------------------------------
PianoRollTrajectory::PianoRollTrajectory(const char *midiFileName) :
    PianoRoll(midiFileName), fCurrentDate(0)
{
    init();
}

//--------------------------------------------------------------------------
PianoRollTrajectory::~PianoRollTrajectory()
{
    delete previousEventInfos;
    delete currentEventInfos;
}

//--------------------------------------------------------------------------
void PianoRollTrajectory::init()
{
    previousEventInfos = new std::vector<EventInfos>;
    currentEventInfos  = new std::vector<EventInfos>;
}

//--------------------------------------------------------------------------
void PianoRollTrajectory::DrawVoice(ARMusicalVoice* v, DrawParams &drawParams)
{
    if (fVoicesColors != NULL) {
        int voiceNum = v->getVoiceNum();
        
        for (unsigned int i = 0; i < fVoicesColors->size() && fColors->empty(); i++) {
            std::pair<int, VGColor *> pair = fVoicesColors->at(i);

            if (pair.first == voiceNum)
                fColors->push(pair.second);
        }
    }
    
    if (!fColors->empty() || fVoicesAutoColored) {
        if (fColors->empty()) {
            int r, g, b;

            drawParams.colorHue += kGoldenRatio;
            drawParams.colorHue  = fmod(drawParams.colorHue, 1);

            HSVtoRGB((float) drawParams.colorHue, 0.5f, 0.9f, r, g, b);

            fColors->push(new VGColor(r, g, b, 255));
        }
        
        drawParams.dev->PushFillColor(*fColors->top());
    }

    fChord              = false;
	ObjectList   *ol    = (ObjectList *) v;
	GuidoPos      pos   = ol->GetHeadPosition();
    TYPE_DURATION finalDur;

	while(pos)
	{
		ARMusicalObject  *e    = ol->GetNext(pos);
		TYPE_DURATION     dur  = e->getDuration();
		TYPE_TIMEPOSITION date = e->getRelativeTimePosition();

        finalDur = (dur ? dur : finalDur);

        if (fChord) {
            dur   = fChordDuration;
            date -= dur;
        }

		TYPE_TIMEPOSITION end = date + dur;

		if (date >= fStartDate) {
            if (date < fEndDate) {
                if (end > fEndDate)
                    dur = fEndDate - date;

                DrawMusicalObject(e, date, dur, drawParams);
            }
		}
		else if (end > fStartDate) { // to make the note end appear
			date = fStartDate;	
			
            if (end > fEndDate)
                dur = fEndDate - date;
			else
                dur = end - date;
			
            DrawMusicalObject(e, date, dur, drawParams);
		}

        if (static_cast<ARRest *>(e->isARRest())) {
            handleRest(date, drawParams);
			fChord = false;
        }
        else if (static_cast<ARChordComma *>(e->isARChordComma()))
			fChord = true;
        else if (static_cast<ARNoteFormat *>(e->isARNoteFormat()))
            handleColor(static_cast<ARNoteFormat *>(e->isARNoteFormat()), drawParams);
        else if (static_cast<ARBar *>(e->isARBar()) && fMeasureBarsEnabled)
            DrawMeasureBar(date, drawParams);
	}

    DrawLinks(drawParams);                // Draws link to final event
    DrawFinalEvent(finalDur, drawParams); // Draws link after final event

    while (!fColors->empty())
        popColor(drawParams);

    previousEventInfos->clear();
    currentEventInfos->clear();
    fCurrentDate = 0;
}

//--------------------------------------------------------------------------
void PianoRollTrajectory::DrawNote(int pitch, double date, double dur, DrawParams &drawParams)
{
    float    x     = date2xpos(date, drawParams.width, drawParams.untimedLeftElementWidth);
    float    y     = pitch2ypos(pitch, drawParams);
    VGColor *color = ((fColors == NULL || fColors->empty()) ? new VGColor(0, 0, 0) : fColors->top());

    if (fCurrentDate == date)
        currentEventInfos->push_back(createNoteInfos(x, y, *color));
    else {
        DrawLinks(drawParams);
        
        delete previousEventInfos;
        previousEventInfos = new std::vector<EventInfos>(*currentEventInfos);

        currentEventInfos->clear();
        currentEventInfos->push_back(createNoteInfos(x, y, *color));

        fCurrentDate = date;
    }
}

//--------------------------------------------------------------------------
void PianoRollTrajectory::DrawLinks(DrawParams &drawParams) const
{
    if (!previousEventInfos->empty() && !currentEventInfos->empty())
        DrawAllLinksBetweenTwoEvents(drawParams);
}

//--------------------------------------------------------------------------
void PianoRollTrajectory::DrawFinalEvent(double dur, DrawParams &drawParams)
{
    if (!currentEventInfos->empty()) {
        float w = duration2width(dur, drawParams.width, drawParams.untimedLeftElementWidth);

        for (unsigned int i = 0; i < currentEventInfos->size(); i++) {
            if (!currentEventInfos->at(i).isRest) {
                VGColor *color = &currentEventInfos->at(i).color;

                if (color != NULL)
                    drawParams.dev->PushFillColor(*color);

                float xCoords[4] = {
                    roundFloat(currentEventInfos->at(i).x),
                    roundFloat(currentEventInfos->at(i).x + w),
                    roundFloat(currentEventInfos->at(i).x + w),
                    roundFloat(currentEventInfos->at(i).x)
                };

                float yCoords[4] = {
                    roundFloat(currentEventInfos->at(i).y - 0.5f * drawParams.noteHeight),
                    roundFloat(currentEventInfos->at(i).y - 0.5f * drawParams.noteHeight),
                    roundFloat(currentEventInfos->at(i).y + 0.5f * drawParams.noteHeight),
                    roundFloat(currentEventInfos->at(i).y + 0.5f * drawParams.noteHeight)
                };

                drawParams.dev->Polygon(xCoords, yCoords, 4);
                
                if (color != NULL)
                    drawParams.dev->PopFillColor();
            }
        }
    }
}

//--------------------------------------------------------------------------
void PianoRollTrajectory::DrawAllLinksBetweenTwoEvents(DrawParams &drawParams) const
{
    for (unsigned int i = 0; i < previousEventInfos->size(); i++) {
        for (unsigned int j = 0; j < currentEventInfos->size(); j++) {
            if (!previousEventInfos->at(i).isRest)
                DrawLinkBetween(previousEventInfos->at(i), currentEventInfos->at(j), drawParams);
        }
    }
}

//--------------------------------------------------------------------------
void PianoRollTrajectory::DrawLinkBetween(PianoRollTrajectory::EventInfos leftEvent, PianoRollTrajectory::EventInfos rightEvent, DrawParams &drawParams) const
{
    VGColor *color = &leftEvent.color;

    if (color != NULL)
        drawParams.dev->PushFillColor(*color);

    float xCoords[4] = {
        roundFloat(leftEvent.x),
        roundFloat(rightEvent.x),
        roundFloat(rightEvent.x),
        roundFloat(leftEvent.x)
    };

    float y1 = leftEvent.y - 0.5f * drawParams.noteHeight;
    float y4 = leftEvent.y + 0.5f * drawParams.noteHeight;
    float y2 = y1;
    float y3 = y4;

    if (!rightEvent.isRest) {
        y2 = rightEvent.y - 0.5f * drawParams.noteHeight;
        y3 = rightEvent.y + 0.5f * drawParams.noteHeight;
    }

    float yCoords[4] = {
        roundFloat(y1),
        roundFloat(y2),
        roundFloat(y3),
        roundFloat(y4) };

    drawParams.dev->Polygon(xCoords, yCoords, 4);

    if (color != NULL)
        drawParams.dev->PopFillColor();
}

//--------------------------------------------------------------------------
void PianoRollTrajectory::handleRest(double date, DrawParams &drawParams)
{
    if (!fChord) {
        DrawLinks(drawParams);

        float x = date2xpos(date, drawParams.width, drawParams.untimedLeftElementWidth);

        delete previousEventInfos;
        previousEventInfos = new std::vector<EventInfos>(*currentEventInfos);

        currentEventInfos->clear();
        currentEventInfos->push_back(createRestInfos(x)); 
    }
}

//--------------------------------------------------------------------------
/*void PianoRollTrajectory::handleEmpty(double date)
{
    handleRest(date);
}*/

//--------------------------------------------------------------------------
PianoRollTrajectory::EventInfos PianoRollTrajectory::createNoteInfos(float x, float y, VGColor color) const
{
    EventInfos newNoteInfos;

    newNoteInfos.color  = color;
    newNoteInfos.x      = x;
    newNoteInfos.y      = y;
    newNoteInfos.isRest = false;
    
    return newNoteInfos;
}

//--------------------------------------------------------------------------
PianoRollTrajectory::EventInfos PianoRollTrajectory::createRestInfos(float x) const
{
    EventInfos newRestInfos;

    newRestInfos.x      = x;
    newRestInfos.isRest = true;

    return newRestInfos;
}


#ifdef MIDIEXPORT

//--------------------------------------------------------------------------
void PianoRollTrajectory::DrawMidiSeq(MidiSeqPtr seq, int tpqn, DrawParams &drawParams)
{
	MidiEvPtr ev       = FirstEv(seq);
	int       tpwn     = tpqn * 4;
	double    start    = double(fStartDate);
	double    end      = double(fEndDate);
    double    finalDur = 0;

	while (ev) {
		if (EvType(ev) == typeNote) {
			double date = double(Date(ev)) / tpwn;
			double dur  = double(Dur(ev))  / tpwn;

            finalDur = (dur > 0 ? dur : finalDur);

			if (date >= start) {
                if (date < end) {
                    double remain = end - date;
                    DrawNote(Pitch(ev), date, (dur > remain ? remain : dur), drawParams);
                }
			}
			else if ((date + dur) > start) {
				dur -= (start - date);
				double remain = end - start;
				DrawNote(Pitch(ev), start, (dur > remain ? remain : dur), drawParams);
			}
		}

		ev = Link(ev);
	}

    DrawLinks(drawParams);                // Draws link to final event
    DrawFinalEvent(finalDur, drawParams); // Draws link after final event

    previousEventInfos->clear();
    currentEventInfos->clear();
    fCurrentDate = 0;
}

#endif
