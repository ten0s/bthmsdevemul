/**
 *   This file is part of Bluetooth for Microsoft Device Emulator
 * 
 *   Copyright (C) 2008 Dmitry Klionsky <dm.klionsky@gmail.com>
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

#ifndef __PACKET_H__
#define __PACKET_H__

class Packet
{
public:
   enum { BUFFER_SIZE = 4 * 1024 };

public:
   Packet();
   ~Packet();

public: // write data.
   bool writeChar( char value );
   bool writeShort( short value );
   bool writeInt( int value );
   bool writeLong( long value );
   bool writeUChar( unsigned char value );
   bool writeUShort( unsigned short value );
   bool writeUInt( unsigned int value );
   bool writeULong( unsigned long value );
   bool writeFloat( float value );
   bool writeDouble( double value );
   bool writeString( const char* value );
   bool writeWString( const wchar_t* value );
   bool writeUCharArray( unsigned char* array, size_t length );

public: // read data.
   char readChar();
   short readShort();
   int readInt();
   long readLong();
   unsigned char readUChar();
   unsigned short readUShort();
   unsigned int readUInt();
   unsigned long readULong();
   float readFloat();
   double readDouble();
   size_t readString( char* value, size_t length );
   size_t readWString( wchar_t* value, size_t length );
   size_t readUCharArray( unsigned char* array, size_t length );

public: // utility methods.   
   size_t length() const;   

public: // serialization methods.
   size_t serialize( void* buffer, size_t size ) const;
   size_t deserialize( const void* buffer, size_t size );

private:
   Packet( const Packet& packet );
   Packet& operator=( const Packet& packet );

private:
   unsigned char* _data;
   size_t _readPos;
   size_t _writePos;
};


#endif //__PACKET_H__