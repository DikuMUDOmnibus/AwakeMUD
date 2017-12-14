/* ************************************************************************
*   File: comm.h                                        Part of CircleMUD *
*  Usage: header file: prototypes of public communication functions       *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#ifndef _comm_h_
#define _comm_h_

enum {
  SUCCESS = 0,
  FAILURE,
  FATAL_SIGNAL
};

void shutdown(int code = FAILURE);

#define COPYOVER_FILE "copyover.dat"

/* comm.c */
void    send_to_all(char *messg);
void    send_to_char(char *messg, struct char_data *ch);
void    send_to_char(const char *messg, struct char_data *ch);
void    send_to_char(struct char_data *ch, const char * const messg, ...);
void    send_to_room(char *messg, int room);
void    send_to_driver(char *messg, struct veh_data *veh);
void    send_to_host(vnum_t room, char *messg, struct matrix_icon *icon, bool needsee);
void    send_to_icon(struct matrix_icon *icon, const char * const messg, ...);
void    send_to_veh(char *messg, struct veh_data *veh, struct char_data *ch, bool torig, ...);
void    send_to_veh(char *messg, struct veh_data *veh, struct char_data *ch, struct char_data *cha, bool torig);
void    send_to_outdoor(char *messg);
void    send_to_obj(struct obj_data *obj, const char * const messg, ...);
void    free_editing_structs(descriptor_data *d);
void    close_socket(struct descriptor_data *d);

void    perform_act(char *orig, struct char_data *ch, struct obj_data *obj,
                    void *vict_obj, struct char_data *to);

void    act(char *str, int hide_invisible, struct char_data *ch,
            struct obj_data *obj, void *vict_obj, int type);

#ifndef connect

int connect();
#endif

int     write_to_descriptor(int desc, char *txt);
void    write_to_q(char *txt, struct txt_q *queue, int aliased);
void    write_to_output(const char *txt, struct descriptor_data *d);
void    page_string(struct descriptor_data *d, char *str, int keep_internal);

#define SEND_TO_Q(messg, desc)  write_to_output((messg), desc)

#define USING_SMALL(d)  ((d)->output == (d)->small_outbuf)
#define USING_LARGE(d)  (!USING_SMALL(d))

typedef void sigfunc(int);

#ifdef __COMM_CC__

/* system calls not prototyped in OS headers */
#if     defined(_AIX)
#define POSIX_NONBLOCK_BROKEN
#include <sys/select.h>
int accept(int s, struct sockaddr * addr, int *addrlen);
int bind(int s, struct sockaddr * name, int namelen);
int getpeername(int s, struct sockaddr * name, int *namelen);
int getsockname(int s, struct sockaddr * name, int *namelen);
int gettimeofday(struct timeval * tp, struct timezone * tzp);
int listen(int s, int backlog);
int setsockopt(int s, int level, int optname, void *optval, int optlen);
int socket(int domain, int type, int protocol);
#endif

#if defined(apollo)
#include <unistd.h>
#endif

#if defined(__hpux)
int accept(int s, void *addr, int *addrlen);
int bind(int s, const void *addr, int addrlen);
int getpeername(int s, void *addr, int *addrlen);
int getsockname(int s, void *name, int *addrlen);
int gettimeofday(struct timeval * tp, struct timezone * tzp);
int listen(int s, int backlog);
int setsockopt(int s, int level, int optname,
               const void *optval, int optlen);
int socket(int domain, int type, int protocol);
#endif

#if     defined(interactive)
#include <net/errno.h>
#include <sys/fcntl.h>
#endif

#if     defined(linux)
//extern "C" int accept(int s, struct sockaddr * addr, int *addrlen);
//extern "C" int bind(int s, const struct sockaddr * name, int namelen);
//int listen(int s, int backlog);
//extern "C" int close(int fd);
int getpeername(int s, struct sockaddr * name, int *namelen);
int getsockname(int s, struct sockaddr * name, int *namelen);
int gettimeofday(struct timeval * tp, struct timezone * tzp);
// int select(int width, fd_set * readfds, fd_set * writefds,
//               fd_set * exceptfds, const struct timeval * timeout);
int socket(int domain, int type, int protocol);
#endif

#if  defined(freebsd)
int getpeername(int s, struct sockaddr * name, int *namelen);
int getsockname(int s, struct sockaddr * name, int *namelen);
int gettimeofday(struct timeval * tp, struct timezone * tzp);
int socket(int domain, int type, int protocol);
#endif

#if     defined(MIPS_OS)
extern int errno;
#endif

#if     defined(NeXT)
int close(int fd);
int chdir(const char *path);
int fcntl(int fd, int cmd, int arg);
#if     !defined(htons)
u_short htons(u_short hostshort);
#endif
#if     !defined(ntohl)
u_long ntohl(u_long hostlong);
u_long htonl(u_long hostlong);
#endif
int read(int fd, char *buf, int nbyte);
int select(int width, fd_set * readfds, fd_set * writefds,
           fd_set * exceptfds, struct timeval * timeout);
int write(int fd, char *buf, int nbyte);
#endif

#if     defined(sequent)
int accept(int s, struct sockaddr * addr, int *addrlen);
int bind(int s, struct sockaddr * name, int namelen);
int close(int fd);
int fcntl(int fd, int cmd, int arg);
int getpeername(int s, struct sockaddr * name, int *namelen);
int getsockname(int s, struct sockaddr * name, int *namelen);
int gettimeofday(struct timeval * tp, struct timezone * tzp);
#if     !defined(htons)
u_short htons(u_short hostshort);
#endif
int listen(int s, int backlog);
#if     !defined(ntohl)
u_long ntohl(u_long hostlong);
#endif
int read(int fd, char *buf, int nbyte);
int select(int width, fd_set * readfds, fd_set * writefds,
           fd_set * exceptfds, struct timeval * timeout);
int setsockopt(int s, int level, int optname, caddr_t optval,
               int optlen);
int socket(int domain, int type, int protocol);
int write(int fd, char *buf, int nbyte);
#endif

/*
 * This includes Solaris SYSV as well
 */
#if defined(sun)
int getrlimit(int resource, struct rlimit *rlp);
int setrlimit(int resource, const struct rlimit *rlp);
int setitimer(int which, struct itimerval *value, struct itimerval *ovalue);
int accept(int s, struct sockaddr * addr, int *addrlen);
int bind(int s, struct sockaddr * name, int namelen);
int close(int fd);
int getpeername(int s, struct sockaddr * name, int *namelen);
int getsockname(int s, struct sockaddr * name, int *namelen);
int gettimeofday(struct timeval * tp, struct timezone * tzp);
int listen(int s, int backlog);
int select(int width, fd_set * readfds, fd_set * writefds,
           fd_set * exceptfds, struct timeval * timeout);
int setsockopt(int s, int level, int optname, const char *optval,
               int optlen);
int socket(int domain, int type, int protocol);
#endif

#if defined(ultrix)
void bzero(char *b1, int length);
int setitimer(int which, struct itimerval *value, struct itimerval *ovalue);
int accept(int s, struct sockaddr * addr, int *addrlen);
int bind(int s, struct sockaddr * name, int namelen);
int close(int fd);
int getpeername(int s, struct sockaddr * name, int *namelen);
int getsockname(int s, struct sockaddr * name, int *namelen);
int gettimeofday(struct timeval * tp, struct timezone * tzp);
int listen(int s, int backlog);
int select(int width, fd_set * readfds, fd_set * writefds,
           fd_set * exceptfds, struct timeval * timeout);
int setsockopt(int s, int level, int optname, void *optval,
               int optlen);
int socket(int domain, int type, int protocol);
#endif

#if !defined(NeXT) && (!defined(WIN32) || defined(__CYGWIN__))
#include <unistd.h>
#endif

#endif /* #ifdef __COMM_C__ */

#endif /* #ifdef _comm_h_ */
