/*
 * server.h
 *
 * This file is a part of the revinetd project              
 *
 * Revinetd is copyright (c) 2003-2008 by Steven M. Gill
 * and distributed under the GPL.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 *
 *                                                                        */

/* $Id: server.h,v 1.11 2008/08/28 03:24:59 necrotaur Exp $ */

#ifndef __SERVER_H__
#define __SERVER_H__

typedef struct _SockQueue {
    int sock;
    struct _SockQueue *next;
} SockQueue;

int server(char *, int, char *, int);

#endif
