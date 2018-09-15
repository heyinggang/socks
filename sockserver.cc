//
//  sockserver.cc
//
//  Created by 吴怡生 on 15/09/2018.
//  Copyright © 2018 yeshen.org. All rights reserved.
//

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <map>
#include "sockserver.h"

using namespace EpollUtil;

inline bool TcpSection::create(int watch_fd)
{
  int client_fd;
  struct sockaddr_in clientaddr;
  socklen_t clientlen;
  client_fd = accept(watch_fd, (struct sockaddr *)&clientaddr, &clientlen);
  if (client_fd == -1)
  {
    perror("fail to accept a connect when create section");
    return false;
  }
  else
  {
    add_event(client_fd, EPOLLIN);
    set_nonblocking(client_fd);
    return true;
  }
}

inline bool TcpSection::read(int fd)
{
}

inline bool TcpSection::write(int fd)
{
}

inline bool Server::init()
{
  this->watch_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (this->watch_fd == -1)
    return false;
  struct sockaddr_in addr;
  bzero(&addr, sizeof(addr));
  addr.sin_family = AF_INET;
  inet_pton(AF_INET, IPADDRESS, &addr.sin_addr);
  addr.sin_port = htons(this->port);
  if (bind(this->watch_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
    return false;
  gobal_epoll_fd = epoll_create1(0) if (gobal_epoll_fd == -1)
  {
    this->close();
    return false;
  }
  add_event(this->watch_fd, EPOLLIN);
  set_nonblocking(this->watch_fd);
  return true;
}
inline void Server::waitToCheck()
{
  int count = epoll_wait(gobal_epoll_fd, gobal_events, EPOLLEVENTS, -1);
  for (int i = 0; i < count; i++)
  {
    this.handle(gobal_events[i]);
  }
}
inline void Server::handle(struct epoll_event &ev)
{
  int fd = ev.data.fd;
  if (fd == this->watch_fd && ev.events & EPOLLIN)
  {
    this->createSection(fd);
  }
  else if (ev.events & EPOLLERR || ev.events & EPOLLHUP)
  {
    this->closeSection(fd);
  }
  else if (ev.events & EPOLLIN)
  {
    // copy data for kernel-space to user-space
    if (this->data.find(fd) == this->data.end())
    {
      cout << "can't find this fd in section pool,inside" << fd << endl;
      return;
    }
    TcpSection *section = this->data[fd];
    if (!section->read(fd))
    {
      this->closeSection(fd);
    }
    else if (section->ready() && this->data.find(section->out_fd) == this->data.end())
    {
      this->data[section->out_fd] = section;
    }
  }
  else if (ev.events & EPOLLOUT)
  {
    // copy data into kernel
    if (this->data.find(fd) == this->data.end())
    {
      cout << "can't find this fd in section pool,outside" << fd << endl;
      return;
    }
    TcpSection *section = this->data[fd];
    if (!section->write(fd))
    {
      this->closeSection(fd);
    }
  }
}
inline void Server::closeSection(int fd)
{
  if (this->data.find(fd) == this->data.end())
  {
    cout << "can't find this fd in section pool,when close" << fd << endl;
    return;
  }
  TcpSection *section = this->data[fd];
  cout << "close connect at fd " << section->in_fd << "," << section->out_fd << end;
  if (section->in_fd)
    this->data.erase(section->in_fd);
  if (section->out_fd)
    this->data.erase(section->out_fd);
  delete section;
}
inline void Server::createSection(int fd)
{
  while (true)
  {
    TcpSection *section = new TcpSection();
    if (!section->create(fd))
    {
      break;
    }
    this->data[section->in_fd] = section;
    cout << "create a new connect at fd" << section->in_fd << end;
  }
}

inline void Server::close()
{
  if (this->watch_fd)
    close(this->watch_fd);
  if (gobal_epoll_fd)
    close(gobal_epoll_fd);
}

static volatile int keepRunning = 1;
static void sig_handler(int sig)
{
  if (sig == SIGINT)
  {
    keepRunning = 0;
  }
}

int main(int argc, const char *argv[])
{
  signal(SIGINT, sig_handler);

  Server server(PORT);
  while (!server.init())
  {
    perror("init error, double try");
  }
  cout << "watching connection" << gobal_epoll_fd << endl;
  while (keepRunning)
  {
    server.waitToCheck();
  }
  server.close();
}