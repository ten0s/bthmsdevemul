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

#include "Packet.h"
#include <assert.h>
#include <string>

#define WRITE_VALUE( value )   \
   size_t size = sizeof( value );   \
   if ( BUFFER_SIZE < _writePos + size ) return false;  \
   memcpy( _data + _writePos, &value, size );   \
   _writePos += size;   \
   return true;

#define WRITE_ARRAY( array, length )   \
   size_t size_length = sizeof( length );   \
   if ( BUFFER_SIZE < _writePos + size_length + length ) return false;  \
   memcpy( _data + _writePos, &length, size_length );  \
   _writePos += size_length;   \
   memcpy( _data + _writePos, array, length );   \
   _writePos += length;   \
   return true;

#define READ_VALUE( type_t )  \
   type_t value = 0; \
   size_t size = sizeof( value );   \
   if ( _writePos < _readPos + size ) return -1;  \
   memcpy( &value, _data + _readPos, size ); \
   _readPos += size; \
   return value;

#define READ_ARRAY( array, length ) \
   if ( !array ) return 0; \
   size_t size_length = sizeof( size_t ); \
   if ( _writePos < _readPos + size_length ) return 0;   \
   size_t str_length = 0;  \
   memcpy( &str_length, _data + _readPos, size_length ); \
   _readPos += size_length;   \
   if ( length < size_length ) return 0;  \
   if ( _writePos < _readPos + str_length ) return 0; \
   memcpy( array, _data + _readPos, str_length );  \
   _readPos += str_length; \
   return str_length;


Packet::Packet()
{
   _data = new unsigned char[ BUFFER_SIZE ];
   memset( _data, 0, BUFFER_SIZE );
   assert( _data );
   _readPos = 0;
   _writePos = 0;
}

Packet::~Packet()
{
   if ( _data )
   {
     delete[] _data; 
     _data = NULL;
   }
   
   _readPos = 0;
   _writePos = 0;   
}

bool Packet::writeChar( char value )
{
   WRITE_VALUE( value );
}

bool Packet::writeShort( short value )
{
   WRITE_VALUE( value );
}

bool Packet::writeInt( int value )
{
   WRITE_VALUE( value );
}

bool Packet::writeLong( long value )
{
   WRITE_VALUE( value );
}

bool Packet::writeUChar( unsigned char value )
{
   WRITE_VALUE( value );
}

bool Packet::writeUShort( unsigned short value )
{
   WRITE_VALUE( value );
}

bool Packet::writeUInt( unsigned int value )
{
   WRITE_VALUE( value );
}

bool Packet::writeULong( unsigned long value )
{
   WRITE_VALUE( value );
}

bool Packet::writeFloat( float value )
{
   WRITE_VALUE( value );
}

bool Packet::writeDouble( double value )
{
   WRITE_VALUE( value );
}

bool Packet::writeString( const char* value )
{
   if ( value )
   {
      size_t length = ( strlen( value ) + 1 ) * sizeof( value[0] );
      WRITE_ARRAY( value, length );            
   }

   return false;   
}

bool Packet::writeWString( const wchar_t* value )
{
   if ( value )
   {
      size_t length = ( wcslen( value ) + 1 ) * sizeof( value[0] );
      WRITE_ARRAY( value, length );
   }

   return false;   
}

bool Packet::writeUCharArray( unsigned char* array, size_t length )
{
   WRITE_ARRAY( array, length );
}

char Packet::readChar()
{
   READ_VALUE( char );
}

short Packet::readShort()
{
   READ_VALUE( short );      
}

int Packet::readInt()
{
   READ_VALUE( int );
}

long Packet::readLong()
{
   READ_VALUE( long );
}

unsigned char Packet::readUChar()
{
   READ_VALUE( unsigned char );
}

unsigned short Packet::readUShort()
{
   READ_VALUE( unsigned short );
}

unsigned int Packet::readUInt()
{
   READ_VALUE( unsigned int );
}

unsigned long Packet::readULong()
{
   READ_VALUE( unsigned long );
}

float Packet::readFloat()
{
   READ_VALUE( float );
}

double Packet::readDouble()
{
   READ_VALUE( double );
}

size_t Packet::readString( char* value, size_t length )
{
   READ_ARRAY( value, length );
}

size_t Packet::readWString( wchar_t* value, size_t length )
{
   READ_ARRAY( value, length );
}

size_t Packet::readUCharArray( unsigned char* array, size_t length )
{
   READ_ARRAY( array, length );
}

size_t Packet::length() const
{
   return _writePos;
}

size_t Packet::serialize( void* buffer, size_t size ) const
{
   assert( _data );
   assert( buffer );
   assert( size <= length() );

   if ( _data && buffer && size <= length() )
   {
      memcpy( buffer, _data, length() );
      return size;
   }

   return 0;   
}

size_t Packet::deserialize( const void* buffer, size_t size )
{
   assert( _data );
   assert( buffer );
   assert( size <= BUFFER_SIZE );

   if ( _data && buffer && size <= BUFFER_SIZE )
   {  
      memcpy( _data, buffer, size );
      _readPos = 0;
      _writePos = size;
      
      return size;
   }

   return 0;   
}