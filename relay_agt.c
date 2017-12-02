/*
 * relay_agt.c
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
#include "revinetd.h"
#include "relay_agt.h"
#include "misc.h"
#include "proxy.h"

static const char cvsid[] = "$Id: relay_agt.c,v 1.28 2008/08/28 03:24:59 necrotaur Exp $";

int
relay_agent(char *callback, int port, char *target, int port2)
{
    int comm_chan, retval, tmp_sock, message = -1;
    time_t timer;
    struct sockaddr_in sa_comm, tmp_sa;
    struct timeval timeout;
    fd_set active, read;
    Channels *tmp_chan;

    /* init time. */
    timer = time(NULL);
    
    /* Establish the callback channel. */

    comm_chan = init_sockaddr(&sa_comm, callback, port);
    if (connect(comm_chan, (struct sockaddr *)&sa_comm,
                sizeof(sa_comm)) == -1) {
        perror("connect");
        return 1;
    }
    register_sock(comm_chan);

    if (conf.verbosity != VB_QUIET)
        printf("Server comm channel connection established to %s:%i\n", callback, port);

    /* Initialize the channels */
    chan = NULL;

    FD_ZERO(&active);
    FD_SET(comm_chan, &active);

    while (1) {
        read = active;
        timeout.tv_sec = conf.keepalive; 
        timeout.tv_usec = 0L;  /* 0 micro seconds, also as a long */
        if ((retval = select(FD_SETSIZE, &read, NULL, NULL, &timeout)) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        if (retval == 0) { /* timeout. */
            /* If the heart beat fail, the socket should throw us an
             * error. */
            send_comm_message(comm_chan, SV_HEART_BEAT);
            timer = time(NULL);
            continue;
        }
        if ((timer + conf.keepalive) < time(NULL)) {
            send_comm_message(comm_chan, SV_HEART_BEAT);
            timer = time(NULL);
        }
        if (FD_ISSET(comm_chan, &read)) {
            if (conf.verbosity == VB_VERBOSE)
                printf("Received message on comm channel: ");

            message = get_comm_message(comm_chan);
            /* We either get a heart beat reply or a request for new
             * channel. */
	    if (message == -1){
	      //EOF, i.e. socket closed
	      printf("Comm channel closed, quitting\n");
	      exit(3);
	    }
            if (message == RA_TARGET_UP) {
                if (conf.verbosity != VB_QUIET)
                    printf("New relay agent proxy pair requested from %s:%i\n", callback, port);

                tmp_sock = init_sockaddr(&tmp_sa, callback, port);
                if (connect(tmp_sock, (struct sockaddr *)&tmp_sa,
                            sizeof(tmp_sa)) == -1) {
                    perror("connect");
                    return 1;
                }
                register_sock(tmp_sock);

                if (conf.verbosity != VB_QUIET)
                    printf("New relay agent connection established to %s:%i\n", callback, port);

                
                if (chan == NULL) {
                    chan = chan_add();
                    tmp_chan = chan;
                } else {
                    tmp_chan = chan;
                    while (tmp_chan->next != NULL) {
                        tmp_chan = tmp_chan->next;
                    }
                    tmp_chan->next = chan_add();
                    tmp_chan->next->prev = tmp_chan;
                    tmp_chan = tmp_chan->next;
                }

                tmp_chan->source = tmp_sock;

                if (conf.verbosity != VB_QUIET)
                    printf("Initializing new target connection to %s:%i\n", target, port2);
            

                tmp_sock = init_sockaddr(&tmp_sa, target, port2);
                if (connect(tmp_sock, (struct sockaddr *)&tmp_sa,
                            sizeof(tmp_sa)) == -1) {
                    perror("connect");
                    return 1;
                }
                register_sock(tmp_sock);

                if (conf.verbosity != VB_QUIET)
                    printf("New target connection established to %s:%i\n", target, port2);

                tmp_chan->target = tmp_sock;

                FD_SET(tmp_chan->source, &active);
                FD_SET(tmp_chan->target, &active);
            }
        }
        proxy(&read, &active);
    }
    
    return 0;
}
