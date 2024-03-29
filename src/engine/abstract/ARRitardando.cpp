/*
  GUIDO Library
  Copyright (C) 2002  Holger Hoos, Juergen Kilian, Kai Renz
  Copyright (C) 2002-2013 Grame

  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.

  Grame Research Laboratory, 11, cours de Verdun Gensoul 69002 Lyon - France
  research@grame.fr

*/

#include <iostream>
#include <string.h>
#include "ARRitardando.h"
#include "TagParameterList.h"
#include "TagParameterString.h"
#include "TagParameterFloat.h"
#include "ListOfStrings.h"

ListOfTPLs ARRitardando::ltpls(1);

ARRitardando::ARRitardando() : ARMTParameter()
{
	rangesetting = ONLY;
    
    tempo    = 0;
    abstempo = 0;
	font     = 0;
	fattrib  = 0;
	fsize    = 0;
}

ARRitardando::~ARRitardando() 
{
	delete tempo;
	delete abstempo;
	delete font;
	delete fattrib;
	delete fsize;
};

void ARRitardando::setTagParameterList(TagParameterList& tpl)
{
	if (ltpls.empty())
	{
		// create a list of string ...

		ListOfStrings lstrs; // (1); std::vector test impl
		lstrs.AddTail(
			( "S,tempo,,o;S,abstempo,,o;S,font,Times new roman,o;S,fattrib,,o;U,fsize,9pt,o" ));
		CreateListOfTPLs(ltpls,lstrs);
	}

	TagParameterList * rtpl = 0;
	int ret = MatchListOfTPLsWithTPL(ltpls,tpl,&rtpl);

	if (ret>=0 && rtpl)
	{
		// we found a match!
		if (ret == 0)
		{
			// Get The TagParameters ...
			tempo = TagParameterString::cast(rtpl->RemoveHead());
			//assert(text);
			abstempo = TagParameterString::cast(rtpl->RemoveHead());
			
			// the font
			font = TagParameterString::cast(rtpl->RemoveHead());
			assert(font);
			// the font attrib ...
			fattrib = TagParameterString::cast(rtpl->RemoveHead());
			assert(fattrib);
			if (fattrib->TagIsNotSet())
			{
				delete fattrib;
				fattrib = 0;
			}
			// the size ...
			fsize = TagParameterFloat::cast(rtpl->RemoveHead());
			assert(fsize);
		}

		delete rtpl;
	}
	else
	{
		// failure
	}

	tpl.RemoveAll();
}

bool ARRitardando::MatchEndTag(const char *s)
{
	if (ARMusicalTag::MatchEndTag(s))
		return true;

	if (!getRange() && !strcmp("\\ritEnd",s))
		return true;
	return false;
}

void ARRitardando::printName(std::ostream& os) const
{
    os << "ARRitardando";
}

void ARRitardando::printGMNName(std::ostream& os) const
{
    os << "\\ritardando";
}

void ARRitardando::printParameters(std::ostream& os) const
{
    if (tempo)
        os << "tempo: " << tempo->getValue() << "; ";

    if (abstempo)
        os << "abstempo: " << abstempo->getValue() << "; ";

    if (font)
        os << "font: " << font->getValue() << "; ";

    if (fattrib)
        os << "fattrib: " << fattrib->getValue() << "; ";

    if (fsize)
        os << "fsize: " << fsize->getValue() << "; ";

    ARMusicalTag::printParameters(os);
}
