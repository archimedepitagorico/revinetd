/*
 * proxy.c
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

#include "includes.h"
#include "proxy.h"
#include "misc.h"

static const char cvsid[] = "$Id: proxy.c,v 1.8 2008/08/28 03:24:59 necrotaur Exp $";

int
proxy(fd_set *active, fd_set *perm)
{
    Channels *temp;

    temp = chan;
    while (temp != NULL) {
        if (FD_ISSET(temp->source, active)) {
            if (copy_between_ports(temp->source, temp->target) < 0) {
                /* We need better handling here. */
                FD_CLR(temp->source, perm);
                FD_CLR(temp->target, perm);
                if (temp->next == NULL) {
                    chan_remove(temp);
                    return 0;
                } else {
                    temp = temp->next;
                    chan_remove(temp->prev);
                }
            }
        }
        if (FD_ISSET(temp->target, active)) {
            if (copy_between_ports(temp->target, temp->source) < 0) {
                /* We need better handling here. */
                FD_CLR(temp->source, perm);
                FD_CLR(temp->target, perm);
                if (temp->next == NULL) {
                    chan_remove(temp);
                    return 0;
                } else {
                    temp = temp->next;
                    chan_remove(temp->prev);
                }
            }
        }
        temp = temp->next;
    }
    
    return 0;
}

void
chan_remove(Channels *temp)
{

    unregister_sock(temp->source);
    unregister_sock(temp->target);

    close(temp->source);
    close(temp->target);

    if (temp->prev == NULL) {
        if (temp->next == NULL) {
            chan = NULL;
            free(temp);
        } else {
            chan = temp->next;
            chan->prev = NULL;
            free(temp);
        }
    } else {
        if (temp->next == NULL) {
            temp->prev->next = NULL;
            free(temp);
        } else {
            temp->prev->next = temp->next;
            temp->next->prev = temp->prev;
            free(temp);
        }
    }
}

Channels *
chan_add(void)
{
    Channels *temp;

    temp = (Channels *)malloc(sizeof(Channels));
    if (temp == NULL) {
        perror("malloc");
        exit(1);
    }

    memset(temp, 0, sizeof(Channels));

    return temp;
}
