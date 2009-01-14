/**
 *   This file is part of Bluetooth for Microsoft Device Emulator
 * 
 *   Copyright (C) 2008-2009 Dmitry Klionsky <dm.klionsky@gmail.com>
 *   
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "DebugOutput.h"

DebugOutput::DebugOutput( LPCTSTR szHeader )
{
   _tcsncpy( _szHeader, szHeader, MAX_LENGTH );
}

void DebugOutput::Print( LPCTSTR szFormat, ... )
{
   TCHAR szBuffer[MAX_LENGTH];

   va_list args;
   va_start( args, szFormat );
   lstrcpy( szBuffer, _szHeader );
   ::wvsprintf( szBuffer + _tcslen( _szHeader ), szFormat, args );
   va_end( args );

   _tcscat( szBuffer, _T("\r\n") );

   ::OutputDebugString( szBuffer );
}