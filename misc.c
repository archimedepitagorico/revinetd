/*
 * misc.c
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
#include "misc.h"

static const char cvsid[] = "$Id: misc.c,v 1.26 2008/08/28 03:24:59 necrotaur Exp $";

int
read_from_client(int filedes)
{
    char buffer[1024];
    int nbytes;
    
    nbytes = read(filedes, buffer, 1024);
    if (nbytes < 0) {
        /* Read error. */
        perror("read");
        exit(EXIT_FAILURE);
    } else if (nbytes == 0) {
        return -1;
    } else {
        /* Data read. */
        fprintf(stderr, "Server: got message: `%s'\n", buffer);
        return 0;
    }
}

int
copy_between_ports(int src_file, int dst_file)
{
    char buffer[SZ_READ_BUFFER];
    int nbytes;
    
    nbytes = read(src_file, buffer, SZ_READ_BUFFER);
    if (nbytes < 0) {
        /* Read error. */
        perror("read");
        exit(EXIT_FAILURE);
    } else if (nbytes == 0) {
        return -1;
    } else {
        /* Data read. */
        /* fprintf (stderr, "Server: got message: `%s'\n", buffer); */
        write(dst_file, buffer, nbytes);
        return 0;
    }
}

int
get_comm_message(int src_file)
{
    char buffer[SZ_READ_BUFFER];
    int nbytes;
    
    nbytes = read(src_file, buffer, SZ_READ_BUFFER);
    if (nbytes < 0) {
        /* Read error. */
        perror("read");
        exit(EXIT_FAILURE);
    } else if (nbytes == 0) {
        return -1;
    } else {
        /* Data read. */
        /* fprintf (stderr, "Server: got message: `%s'\n", buffer); */
        return (int)*buffer; 
    }
}

int
send_comm_message(int dst_file, int message)
{
    char *buffer = (char*)&message;
    int nbytes = sizeof(buffer);
    
    write(dst_file, buffer, nbytes);
    
    return 0;
}

int
make_socket(const char *hostname, unsigned short port)
{
    int sock, sock_opt = 1;
    struct sockaddr_in name;
    struct hostent *hostinfo;
    struct in_addr saddr;
    
    /* Create the socket. */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    /* Set SO_REUSEADDR */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(sock_opt));

    
    /* Give the socket a name. */
    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    /* check if ip address or resolve hostname */
#ifdef HAVE_INET_ATON
    if (!inet_aton(hostname, &saddr)) {
#else
    saddr.s_addr = inet_addr(hostname);
    if (saddr.s_addr == -1) {
#endif
        hostinfo = gethostbyname(hostname);
        if (hostinfo == NULL) {
            fprintf(stderr, "Unknown host %s.\n", hostname);
            exit(EXIT_FAILURE);
        }
        name.sin_addr = *(struct in_addr *)hostinfo->h_addr;
    } else {
        name.sin_addr = saddr;
    }
/*    name.sin_addr.s_addr = htonl(INADDR_ANY); */
    if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    
    return sock;
}

int
init_sockaddr(struct sockaddr_in *name, const char *hostname,
        unsigned short port)
{
    int sock;
    struct hostent *hostinfo;
    struct in_addr saddr;
    
    /* Create the socket. */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    name->sin_family = AF_INET;
    name->sin_port = htons(port);
#ifdef HAVE_INET_ATON
    if (!inet_aton(hostname, &saddr)) {
#else
    saddr.s_addr = inet_addr(hostname);
    if (saddr.s_addr == -1) {
#endif
        hostinfo = gethostbyname(hostname);
        if (hostinfo == NULL) {
            fprintf(stderr, "Unknown host %s.\n", hostname);
            exit(EXIT_FAILURE);
        }
        name->sin_addr = *(struct in_addr *)hostinfo->h_addr;
    } else {
        name->sin_addr = saddr;
   }
    memset(name->sin_zero, '\0', 8);   

    return sock;
}

void
register_sock(int sock)
{
    OpenSockets *open_sock;

    /* Get a new OpenSockets struct. */
    open_sock = malloc(sizeof(OpenSockets));
    if (open_sock == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    memset(open_sock, 0, sizeof(OpenSockets));

    /* Fill the struct. */
    open_sock->sock = sock;

    /* Insert the struct. */
    open_sock->prev = NULL;
    open_sock->next = conf.open_sock;
    if (conf.open_sock != NULL) {
        conf.open_sock->prev = open_sock;
    }
    conf.open_sock = open_sock;

    return;
}

int
unregister_sock(int sock)
{
    OpenSockets *open_sock;

    open_sock = conf.open_sock;
    while (open_sock != NULL) {
        if (open_sock->sock == sock) {
            /* Remove link from previous or first entry. */
            if (open_sock->prev != NULL)
                open_sock->prev->next = open_sock->next;
            else
                conf.open_sock = open_sock->next;
            /* Remove link from next entry. */
            if (open_sock->next != NULL)
                open_sock->next->prev = open_sock->prev;
            /* Free OpenSockets struct. */
            free(open_sock);
            /* Close sock. */
            if (shutdown(sock, SHUT_RDWR) == -1)
                return 0; /* Socket couldn't be closed. */
            close(sock);
            /* Return success. */
            return 1;
        }
        open_sock = open_sock->next;
    }

    /* Socket not found. */
    return 0;
}
