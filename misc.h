/*
 * getopt.h
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

/* $Id: misc.h,v 1.14 2008/08/28 03:24:59 necrotaur Exp $ */

#ifndef __MISC_H__
#define __MISC_H__

#define SZ_READ_BUFFER 2048

/* COMM Messages */

/* Server commands RA to bring up target */
#define RA_TARGET_UP        1

/* RA commands server to acknowledge that it's alive. */
#define SV_HEART_BEAT       2

/* Server confirms that it's alive. */
#define RA_SERVER_ALIVE     3

int make_socket(const char *, unsigned short);
int init_sockaddr(struct sockaddr_in *, const char *, unsigned short);
int read_from_client(int);
int copy_between_ports(int, int);
void register_sock(int);
int unregister_sock(int);
int send_comm_message(int, int);
int get_comm_message(int);

#endif
