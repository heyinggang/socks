//
//  sockserver.h
//
//  Created by 吴怡生 on 15/09/2018.
//  Copyright © 2018 yeshen.org. All rights reserved.
//

#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <map>
#include <list>
#include <sys/epoll.h>

#define IPADDRESS "0.0.0.0"
#define PORT 1080
#define MAXSIZE 1024
#define LISTENQ 5
#define FDSIZE 1000
#define EPOLLEVENTS 1024

using namespace std;

class Buff
{
private:
  int size;
  char *buf;

public:
  Buff(int _size, char *_buf) : size(_size), buf(_buf){};
  ~Buff()
  {
    if (buf)
      delete buf;
  }
};

enum SectionStatus
{
  INIT,
  CONNECT,
  REQUEST,
  READY,
  CLOSE
};

class TcpSection
{
private:
  SectionStatus status;
  queue<Buff *> in_buff;
  queue<Buff *> out_buff;

public:
  int in_fd;
  int out_fd;
  TcpSection() : status(INIT){};
  ~TcpSection()
  {
    if (this->in_fd)
      close(this->in_fd);
    if (this->out_fd)
      close(this->out_fd);
    while (!this->in_buff.empty())
      delete this->in_buff.pop();
    while (!this->out_buff.empty())
      delete this->out_buff.pop();
  }
  inline bool create(int watch_fd);
  inline bool ready() {}
  inline bool read(int fd);
  inline bool write(int fd);
};

class Server
{
private:
  map<int, TcpSection *> data;
  int port;
  int watch_fd;
  inline void closeSection(int fd);
  inline void createSection(int fd);
  inline void handle(struct epoll_event &ev);

public:
  Server(int _port) : port(_port){};
  ~Server()
  {
    for (map<int, TcpSection *>::iterator it = this->data.begin(); it != this.data.end(); ++it)
      delete it->second;
  }
  inline bool init();
  inline void waitToCheck();
  inline void close();
}

namespace EpollUtil
{
  static int gobal_epoll_fd;
  static struct epoll_event gobal_events[EPOLLEVENTS];
  inline void bool set_nonblocking(int fd)
  {
    int sock_flags = fcntl(fd, F_GETFL, 0);
    sock_flags |= O_NONBLOCK;
    return fcntl(fd, F_SETFL, sock_flags) != -1;
  };
  inline void static add_event(int fd, int state)
  {
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(gobal_epoll_fd, EPOLL_CTL_ADD, fd, &ev);
  };
  inline void static del_event(int fd, int state)
  {
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(gobal_epoll_fd, EPOLL_CTL_DEL, fd, &ev);
  };
  inline void static modify_event(int fd, int state)
  {
    struct epoll_event ev;
    ev.events = state;
    ev.data.fd = fd;
    epoll_ctl(gobal_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
  };
}; // namespace EpollUtil

#endif