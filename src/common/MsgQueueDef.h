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

#ifndef __MSG_QUEUE_DEF_H__
#define __MSG_QUEUE_DEF_H__

#include "Packet.h"

#define ERROR_QUEUE_NAME               _T("{2EDAE8CC-DACE-4dc5-B7B3-ADB5318B61B5}")

#ifdef REMOTE_AGENT
   #define READ_QUEUE_NAME             _T("{1570692E-0FB1-43e8-878A-A6EC8983DECF}")
   #define WRITE_QUEUE_NAME            _T("{A0E16217-E207-4ded-ABE8-0BC0ABC4E565}")      
   #define MSG_QUEUE_READ_TIMEOUT      5000
#else
   #define WRITE_QUEUE_NAME            _T("{1570692E-0FB1-43e8-878A-A6EC8983DECF}")
   #define READ_QUEUE_NAME             _T("{A0E16217-E207-4ded-ABE8-0BC0ABC4E565}")      
   #define MSG_QUEUE_READ_TIMEOUT      600000
#endif

#define MSG_QUEUE_WRITE_TIMEOUT     1000
#define MSG_BUFFER_SIZE             Packet::BUFFER_SIZE

static HANDLE g_hReadQueue = NULL;
static HANDLE g_hWriteQueue = NULL;
static HANDLE g_hErrorQueue = NULL;

enum PACKET_TYPE {
   HCI_DATA_PACKET = 0,
   HCI_DATA_ERROR_PACKET,
   MESSAGE_PACKET
};

// messages ids for PACKET_TYPE::MESSAGE_PACKET
enum MESSAGE_ID {
   // agent related messages
   AGENT_ACK_MSG = 0,
   AGENT_PING_MSG,
   AGENT_INITIALIZE_MSG,
   AGENT_UNINITIALIZE_MSG,
   AGENT_LOGGING_ON_MSG,
   AGENT_LOGGING_OFF_MSG,

   // communication related messages
   COM_OPEN_MSG,
   COM_CLOSE_MSG
};

BOOL CreateMsgQueues() {
   MSGQUEUEOPTIONS msgQO; 
   memset( &msgQO, 0, sizeof( msgQO ) );
   msgQO.dwSize = sizeof( msgQO );
   msgQO.dwFlags = MSGQUEUE_ALLOW_BROKEN;
   msgQO.dwMaxMessages = 1; /* just one message in the queue */
   msgQO.cbMaxMessage = MSG_BUFFER_SIZE;
   msgQO.bReadAccess = FALSE;
   g_hWriteQueue = CreateMsgQueue( WRITE_QUEUE_NAME, &msgQO );
   ASSERT( NULL != g_hWriteQueue );

   msgQO.bReadAccess = TRUE;
   g_hReadQueue = CreateMsgQueue( READ_QUEUE_NAME, &msgQO );
   ASSERT( NULL != g_hReadQueue );

#ifdef REMOTE_AGENT
   msgQO.bReadAccess = FALSE;
#else
   msgQO.bReadAccess = TRUE;
#endif
   g_hErrorQueue = CreateMsgQueue( ERROR_QUEUE_NAME, &msgQO );
   ASSERT( NULL != g_hErrorQueue );

   return ( g_hWriteQueue != NULL && g_hReadQueue != NULL && g_hErrorQueue != NULL );
}

#endif //__MSG_QUEUE_DEF_H__