/*
 * revinetd.h
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

/* $Id: revinetd.h,v 1.16 2008/08/28 03:24:59 necrotaur Exp $ */

#ifndef __REVINETD_H__
#define __REVINETD_H__

/* Define truth. Too bad we can only do that on a computer. */
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE !FALSE
#endif

#include "includes.h"

/* Values for server flag */
#define FLAG_NONE 0
#define FLAG_RELAY -1
#define FLAG_SERVER 1
/* Values for verbosity */
#define VB_NORMAL 0
#define VB_QUIET -1
#define VB_VERBOSE 1

/* Structures defs for configuration. */
typedef struct _OpenSockets {
    int sock;
    struct _OpenSockets *next;
    struct _OpenSockets *prev;
} OpenSockets;

typedef struct _Conf {
    int server_flag;
    int daemonize;
    int port;
    int port2;
    int verbosity;
    char *host;
    char *host2;
    long int keepalive;
    OpenSockets *open_sock;
} Conf;

/* Global variable. */
Conf conf;

char *exec_name;

/* Function prototypes. */
void init_conf(void);
void clean_exit(int);
void usage(void);
unsigned short parse_host_str(char *);

#endif
