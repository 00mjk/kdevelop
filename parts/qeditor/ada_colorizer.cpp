/* $Id$
 * Copyright (C) 2003 Oliver Kellogg
 * okellogg@users.sourceforge.net
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include "ada_colorizer.h"
#include "qeditor.h"
#include "paragdata.h"

#include <qfont.h>
#include <qapplication.h>
#include <qsettings.h>
#include <private/qrichtext_p.h>
#include <kdebug.h>

static const char *ada_keywords[] = {
    "abort",
    "abs",
    "accept",
    "access",
    "all",
    "and",
    "array",
    "at",
    "begin",
    "body",
    "case",
    "constant",
    "declare",
    "delay",
    "delta",
    "digits",
    "do",
    "else",
    "elsif",
    "end",
    "entry",
    "exception",
    "exit",
    "for",
    "function",
    "generic",
    "goto",
    "if",
    "in",
    "is",
    "limited",
    "loop",
    "mod",
    "new",
    "not",
    "null",
    "of",
    "or",
    "others",
    "out",
    "package",
    "pragma",
    "private",
    "procedure",
    "raise",
    "range",
    "record",
    "rem",
    "renames",
    "return",
    "reverse",
    "select",
    "separate",
    "subtype",
    "task",
    "terminate",
    "then",
    "type",
    "use",
    "when",
    "while",
    "with",
    "xor",
    0
};

AdaColorizer::AdaColorizer (QEditor * editor)
    : QSourceColorizer (editor)
{
    //default context
    HLItemCollection* context0 = new HLItemCollection (0);
    context0->appendChild (new StartsWithHLItem ("--", Comment, 0));
    context0->appendChild (new KeywordsHLItem (ada_keywords, Keyword, Normal, 0));
    context0->appendChild (new WhiteSpacesHLItem (Normal, 0));
    context0->appendChild (new StringHLItem ("\"", String, 1));
    context0->appendChild (new NumberHLItem (Constant, 0));
    context0->appendChild (new RegExpHLItem ("[0-9][0-9]*#[A-Fa-f0-9]*#", Constant, 0));

    HLItemCollection* context1 = new HLItemCollection (String);
    context1->appendChild (new StringHLItem ("\"", String, 0));

    m_items.append (context0);
    m_items.append (context1);
}

AdaColorizer::~AdaColorizer ()
{
}

int AdaColorizer::computeLevel (QTextParagraph* parag, int startLevel)
{
    int level = startLevel;

    QString s = editor ()->text (parag->paragId ());
    ParagData* data = (ParagData*) parag->extraData ();
    if (!data || s.isEmpty ()) {
        kdDebug() << "AdaColorizer::computeLevel: early return" << endl;
        return startLevel;
    }

    data->setBlockStart (false);

    // Level starters for Ada:  begin, do, loop, then
    if (s.contains ("begin") ||
        s.contains ("do") ||
        s.contains ("loop") ||
	s.contains ("then"))
        ++level;
    // Level terminators for Ada: end
    else if (s.contains ("end"))
        --level;

    if (level > startLevel) {
        data->setBlockStart (true);
    }

    kdDebug() << "AdaColorizer::computeLevel called, startLevel="
              << startLevel << ", text: '" << s
              << "', level=" << level << endl;
    return level;
}

