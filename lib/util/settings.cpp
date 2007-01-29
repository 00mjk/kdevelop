/***************************************************************************
 *   Copyright (C) 2005 by Jens Dagerbo                                    *
 *   jens.dagerbo@swipnet.se                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <kconfig.h>
#include <kglobal.h>
#include <kconfiggroup.h>
 
#include "settings.h"

QString Settings::terminalEmulatorName( KConfig & config )
{
	config.setGroup("TerminalEmulator");
	bool useKDESetting = config.readEntry( "UseKDESetting", true );
	QString terminal;
		
	if ( useKDESetting )
	{
		KConfigGroup confGroup( KGlobal::config(), QLatin1String("General") );
		terminal = confGroup.readEntry( QLatin1String("TerminalApplication"), QString("konsole"));
	}
	else
	{
		terminal = config.readEntry( QLatin1String("TerminalApplication"), QString("konsole"));
	}
	return terminal;
}
