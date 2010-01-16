/*
 * This file is part of KDevelop
 *
 * Copyright 2009 Milian Wolff <mail@milianw.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KDEV_COLORCACHE_H
#define KDEV_COLORCACHE_H

#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtGui/QColor>

#include "../languageexport.h"

namespace KDevelop {

class CodeHighlightingColors;

/**
 * A singleton which holds the global default colors, adapted to the current color scheme
 */
class KDEVPLATFORMLANGUAGE_EXPORT ColorCache : public QObject
{
  Q_OBJECT

  public:
    ~ColorCache();

    /// access the global color cache
    static ColorCache* self();

    /// setup the color cache, this needs to be called before self() is accessed
    static void initialize();

    /// adapt a given foreground color to the current color scheme
    /// @p ratio between 0 and 255 where 0 gives @see m_foregroundColor
    /// and 255 gives @p color
    QColor blend(QColor color, uchar ratio) const;

    /// adapt a given background color to the current color scheme
    /// @p ratio between 0 and 255 where 0 gives @see m_backgroundColor
    /// and 255 gives @p color
    QColor blendBackground(QColor color, uchar ratio) const;

    /// blend a color for local colorization according to the user settings
    /// @see blend()
    QColor blendLocalColor(QColor color) const;

    /// blend a color for global colorization according to the user settings
    /// @see blend()
    QColor blendGlobalColor(QColor color) const;

    /// access the default colors
    CodeHighlightingColors* defaultColors() const;

    /// access the generated colors
    /// @see validColorCount()
    QColor generatedColor(uint num) const;

    /// returns the number of valid generated colors
    /// @see generatedColor()
    uint validColorCount() const;

  public slots:
    /// will regenerate colors after taking changes in the color configuration into account
    /// @see colorsGotChanged()
    void adaptToColorChanges();

  signals:
    /// will be emitted whenever the colors got changed
    /// @see adaptToColorChanges()
    void colorsGotChanged();

  private:
    ColorCache(QObject *parent = 0);
    static ColorCache* m_self;

    /// setup colors for the current settings
    void setupColors();

    /// get @p count colors from the color wheel and adapt them to the current color scheme
    void generateColors(uint count);

    /// the default colors for the different types
    CodeHighlightingColors* m_defaultColors;

    /// the generated colors
    QList<QColor> m_colors;

    /// Must always be m_colors.count()-1, because the last color must be the fallback text color
    uint m_validColorCount;

    /// Maybe make this configurable: An offset where to start stepping through the color wheel
    uint m_colorOffset;

    /// the text color for the current color scheme
    QColor m_foregroundColor;

    /// the background color for the current color scheme
    QColor m_backgroundColor;

    /// How generated colors for local variables should be mixed with the foreground color.
    /// Between 0 and 255, where 255 means only foreground color, and 0 only the chosen color.
    uchar m_localColorRatio;

    /// How global colors (i.e. for types, uses, etc.) should be mixed with the foreground color.
    /// Between 0 and 255, where 255 means only foreground color, and 0 only the chosen color.
    uchar m_globalColorRatio;
};

}

#endif // KDEV_COLORCACHE_H

// kate: space-indent on; indent-width 2; replace-trailing-space-save on; show-tabs on; tab-indents on; tab-width 2;
