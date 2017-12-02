/*
 * revinetd.c
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
#include "server.h"
#include "misc.h"

static const char cvsid[] = "$Id: revinetd.c,v 1.29 2008/08/28 03:24:59 necrotaur Exp $";

int
main(int argc, char **argv)
{
    /* All variables are initialized to a known invalid value so that they
       are not used un-initialized. To use them should trip tests or cause
       segfaults. */
    static struct option long_options[] = {
        /* These options set a flag. */
        {"help", no_argument, 0, 'h'},
        {"daemonize", no_argument, 0, 'd'},
        {"relay-agent", no_argument, 0, 'r'},
        {"server", no_argument, 0, 's'},
        {"listen-client", required_argument, 0, 'c'},
        {"listen-relay", required_argument, 0, 'l'},
        {"server-host", required_argument, 0, 'b'},
        {"target-host", required_argument, 0, 't'},
        {"verbose", no_argument, 0, 'v'},
        {"quiet", no_argument, 0, 'q'},
        {"keep-alive", required_argument, 0, 'k'},
        {0, 0, 0, 0}};
    /* getopt_long stores the option index here. */
    int c, option_index = 0;
    pid_t pid;
   
    printf("%s - Copyright (c) 2003-2008 Steven M. Gill\n", *argv);
    printf("%s is distributed under the terms of the GPL\n", *argv);
    printf("http://revinetd.sourceforge.net\n\n");
    exec_name = strdup(*argv);
    
    init_conf();

    while (1) {
        c = getopt_long(argc, argv, "hdrsc:l:b:t:qvk:", long_options,
                &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c) {
        case 0:
            /* If this option set a flag, do nothing else now. */
            if (long_options[option_index].flag != 0)
                break;
            printf("option %s", long_options[option_index].name);
            if (optarg)
                printf(" with arg %s", optarg);
            printf("\n");
            break;

        case 'd':
            conf.daemonize = 1;
            conf.verbosity = VB_QUIET;
            break;
            
        case 's':
            if (conf.server_flag == FLAG_NONE) {
                conf.server_flag = FLAG_SERVER;
            } else {
                fprintf(stderr, "You can only set one mode, either Server or "
                                "Relay Agent, not both.\n\n");
                usage();
            }
            break;

        case 'r':
            if (conf.server_flag == FLAG_NONE) {
                conf.server_flag = FLAG_RELAY;
            } else {
                fprintf(stderr, "You can only set one mode, either Server or "
                                "Relay Agent, not both.\n\n");
                usage();
            }
            break;

        case 'c':
            if (conf.server_flag == FLAG_SERVER) {
                conf.host = strdup(optarg);
                conf.port = parse_host_str(conf.host);
            } else {
                fprintf(stderr, "You must select Server mode to use -c or "
                                "--listen-client.\n\n");
                usage();
            }
            break;

        case 'l':
            if (conf.server_flag == FLAG_SERVER) {
                conf.host2 = strdup(optarg);
                conf.port2 = parse_host_str(conf.host2);
            } else {
                fprintf(stderr, "You must select Server mode to use -l or "
                                "--listen-relay.\n\n");
                usage();
            }
            break;

        case 'b':
            if (conf.server_flag == FLAG_RELAY) {
                conf.host = strdup(optarg);
                conf.port = parse_host_str(conf.host);
            } else {
                fprintf(stderr, "You must select Relay Agent mode to use -c "
                                "or --server-host.\n\n");
                usage();
            }
            break;
    
        case 't':
            if (conf.server_flag == FLAG_RELAY) {
                conf.host2 = strdup(optarg);
                conf.port2 = parse_host_str(conf.host2);
            } else {
                fprintf(stderr, "You must select Relay Agent mode to use -t "
                                "or --target-host.\n\n");
                usage();
            }
            break;
        case 'h':
            usage();
            break;

        case 'k':
            conf.keepalive = atoi(optarg);
            break;
            
        case 'v':
            if (conf.daemonize == 0) {
                conf.verbosity = VB_VERBOSE;
            }
            break;
            
        case 'q':
            conf.verbosity = VB_QUIET;
            break;
            
        case '?':
            /* FALL THROUGH */
        default:
            usage();
        }
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc) {
        usage();
    }

    if (conf.daemonize == TRUE) {
        pid = fork();

        if (pid > 0) { /* parent */
            exit(0);
        } else if (pid < 0) { /* error */
            perror("fork");
            exit(1);
        } /* child */
        setsid();

        close(0); close(1); close(2);
    }
    
    signal(SIGTERM, clean_exit);
    signal(SIGINT, clean_exit);
    signal(SIGQUIT, clean_exit);
    signal(SIGHUP, clean_exit);
    
    if (conf.server_flag == FLAG_SERVER) {
        if (conf.port >= 0 && conf.port2 >= 0) {
            server(conf.host, conf.port, conf.host2, conf.port2);
        } else {
            fprintf(stderr, "Two ports, port1 (-l and --client-port) and "
                            "port2 (-L and --ra-port) must be "
                            "provided.\n\n");
            usage();
        }
    } else if (conf.server_flag == FLAG_RELAY) {
        if (conf.host != NULL && conf.host2 != NULL) {
            relay_agent(conf.host, conf.port, conf.host2, conf.port2);
        } else {
            fprintf(stderr, "Both a server (-b and --server-host) and a "
                            "target (-t and --target-host)\nmust be "
                            "specified.\n\n");
            usage();
        }
    } else {
        usage();
    }

    exit(0);
}

void
usage(void)
{

    fprintf(stderr, "Usage: revinetd -s -c HOST1:PORT1 -l HOST2:PORT2 [-d]"
                    "\n"
                    "       revinetd -r -b HOST1:PORT1 -t "
                    "HOST2:PORT2 [-d] [-k seconds]\n"
                    "\n"
                    "Global arguments:\n"
                    "  -s  --server\t\tRuns in server mode\n"
                    "  -r  --relay-agent\tRuns as the relay agent\n"
                    "  -d  --daemonize\tRuns as a daemon (implies -q)\n"
                    "  -h  --help\t\tPrints this message and exit\n"
                    "  -q  --quiet\t\tSurpress all messages\n"
                    "  -v  --verbose\t\tIncrease verbiosity\n"
                    "\n"
                    "Server arguments:\n"
                    "  -c HOST1:PORT1 --client-port=HOST1:PORT1\tListen for a client on IP and Port\n"
                    "  -l HOST2:PORT2 --ra-port=HOST2:PORT2    \tListen for a relay agent on IP and Port\n"
                    "Relay agent arguments:\n"
                    "  required arguments:\n"
                    "  -b HOST1:PORT1 --server-host=HOST1:PORT1\tRevinetd server "
                    "host and port\n"
                    "  -t HOST2:PORT2 --target-host=HOST2:PORT2\tThe target host "
                    "and port\n\n"
                    "  optional arguments:\n"
                    "  -k seconds --keep-alive=seconds\tKeep-alive signal interval\n"
                    "                                 \tdefaults to 180 seconds\n\n");

    clean_exit(SIGQUIT);
}

void
clean_exit(int sig)
{
    OpenSockets *open_sock;
    
    /* Close all registered sockets. */
    open_sock = conf.open_sock;
    while (open_sock != NULL) {
        if (shutdown(open_sock->sock, SHUT_RDWR) == -1) {
            perror("shutdown");
        }
        close(open_sock->sock);
        open_sock = open_sock->next;
    }
    
    exit(0);
}

unsigned short
parse_host_str(char *host_str)
{
    unsigned short port;
    int j = 0;
    
    /* Skip any leading spaces. */
    while (isspace(*host_str)) { host_str++; }
    
    /* Loop through the line until we reach a ':'. Exit with errors if we
       stumble on an illegal character. */
    while (*(host_str + j) != ':') {
        if (*(host_str + j) == '\0' || isspace(*(host_str + j))) {
            fprintf(stderr, "Invalid hostname format.\n");
            usage();
        }
        j++;
    }

    /* Insert a NUL byte at the position of the ':' to delimitate the
       hostname. And then skip ahead one byte. */
    *(host_str + j++) = '\0';
   
    /* Grab the port. */
    port = (unsigned short)atoi(host_str + j);

    return port;
}

void
init_conf(void)
{

    memset(&conf, 0, sizeof(Conf));
    conf.port = -1;
    conf.port2 = -1;
    conf.verbosity = VB_NORMAL;
    conf.keepalive = 180L;
}
