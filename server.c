/*
 * server.c
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

#include "revinetd.h"
#include "includes.h"
#include "proxy.h"
#include "server.h"
#include "misc.h"

static const char cvsid[] = "$Id: server.c,v 1.28 2008/08/28 03:24:59 necrotaur Exp $";

int
server(char *host,int port, char *host2, int port2)
{
    int ra_sock, proxy_sock, tmp_sock, bh_sock = -1;
    fd_set active, read;
    struct sockaddr_in tmp_sin;
    Channels *tmp_chan, *temp;
    SockQueue *tmp_sq, *sockq = NULL;
    size_t size;

    /* Make the sockets. */
    ra_sock = make_socket(host2, port2);
    proxy_sock = make_socket(host, port);
    register_sock(ra_sock);
    register_sock(proxy_sock);
    
    if (listen(proxy_sock, 1) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if (conf.verbosity != VB_QUIET)
        printf("Waiting for client on %s:%i\n", host, port);

    if (listen(ra_sock, 1) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    if (conf.verbosity != VB_QUIET)
        printf("Waiting for relay agent on %s:%i\n", host2, port2);

    /* No channels yet. */
    chan = NULL;
    
    FD_ZERO(&active);
    FD_SET(proxy_sock, &active);
    FD_SET(ra_sock, &active);
    
    while (1) {
        read = active;
        if (select(FD_SETSIZE, &read, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }
        if (FD_ISSET(proxy_sock, &read)) {
            if (bh_sock == -1) continue; /* Ignore untill relay agent is
                                            connected. */
            /* Connect with client. */
            size = sizeof(tmp_sin);
            tmp_sock = accept(proxy_sock, (struct sockaddr *)&tmp_sin,
                    &size);
            if (tmp_sock < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            register_sock(tmp_sock);

            if (conf.verbosity != VB_QUIET)
                printf("New client connection detected from %s:%hd\n",
                        inet_ntoa(tmp_sin.sin_addr),
                        ntohs(tmp_sin.sin_port));

            /* New sockq entry for half open connections. */
            if (sockq == NULL) {
                sockq = (SockQueue *)malloc(sizeof(SockQueue));
                if (sockq == NULL) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                memset(sockq, 0, sizeof(SockQueue));
                tmp_sq = sockq;
            } else {
                tmp_sq = sockq;
                while (tmp_sq->next != NULL)
                    tmp_sq = tmp_sq->next;
                tmp_sq->next = (SockQueue *)malloc(sizeof(SockQueue));
                tmp_sq = tmp_sq->next;
                if (tmp_sq == NULL) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }
                memset(tmp_sq, 0, sizeof(SockQueue));
            }
            tmp_sq->sock = tmp_sock;

            /* Now we send a request for a new backhaul channel. */
            printf("Requesting new proxy pair from relay agent\n");
            send_comm_message(bh_sock, RA_TARGET_UP);
        } else if (FD_ISSET(ra_sock, &read)) {
            /* Connect with client. */
            size = sizeof(tmp_sin);
            tmp_sock = accept(ra_sock, (struct sockaddr *)&tmp_sin, &size);
            if (tmp_sock < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            register_sock(tmp_sock);

            if (bh_sock == -1) {
                if (conf.verbosity != VB_QUIET)
                    printf("Relay agent comm channel established from %s:%hd\n",
                            inet_ntoa(tmp_sin.sin_addr),
                            ntohs(tmp_sin.sin_port));
                bh_sock = tmp_sock;
                FD_SET(bh_sock, &active);
            } else {
                if (sockq == NULL) {
                    printf("Unrequested connection from relay agent.\n");
                } else {

                if (conf.verbosity != VB_QUIET)
                    printf("New relay agent connection established from %s:%hd\n",
                            inet_ntoa(tmp_sin.sin_addr),
                            ntohs(tmp_sin.sin_port));

                    tmp_sq = sockq;
                    sockq = tmp_sq->next;
                 
                    if (chan == NULL) {
                        chan = chan_add();
                        tmp_chan = chan;
                    } else {
                        tmp_chan = chan;
                        while (tmp_chan->next != NULL)
                            tmp_chan = tmp_chan->next;
                        tmp_chan->next = chan_add();
                        tmp_chan->next->prev = tmp_chan;
                        tmp_chan = tmp_chan->next;
                    }

                    tmp_chan->source = tmp_sq->sock;
                    tmp_chan->target = tmp_sock;

                    FD_SET(tmp_chan->source, &active);
                    FD_SET(tmp_chan->target, &active);
                    
                    free(tmp_sq);
                }
            }
        } else if (FD_ISSET(bh_sock, &read)) {
            /* We don't expect anything back. This should
               mean that comm is dropped.  relay agent is
               disconnected so reset state and wait for 
               another relay agent. */
            /* Slight amend, now we either get an error or a
             * heart beat... Hopefully this won't break anything. */
            if (get_comm_message(bh_sock) == SV_HEART_BEAT) {
                send_comm_message(bh_sock, RA_SERVER_ALIVE);
            } else {
                if (conf.verbosity != VB_QUIET) 
                    printf("Relay agent disconnected.  Resetting...\n");
                unregister_sock(bh_sock);
                FD_CLR(bh_sock, &active);
                bh_sock = -1;
                temp = chan;
                while (temp != NULL) {
                    chan_remove(temp);
                    FD_CLR(temp->source, &active);
                    FD_CLR(temp->target, &active);
                }           
            }
        }    
        proxy(&read, &active);
    }

    return(0);
}
