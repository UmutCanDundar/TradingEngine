// MulticastReceiver.h
#pragma once
#include "ErrorHandler.h"
#include "common.h"
#include "GeneratedIpPort.h"

#include <arpa/inet.h>  // IP address conversion (inet - string <-> binary)
#include <cstring>      // C-style mem ops
#include <netinet/in.h> // socket address structs
#include <sys/socket.h> // socket functions
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h> // close()
#include <cerrno>
#include <boost/lockfree/spsc_queue.hpp>
#include <array>
#include <vector>

inline constexpr size_t PACKET_SIZE = 2048;
inline constexpr size_t QUEUE_CAPACITY = 1024;
using spscQueue_t = boost::lockfree::spsc_queue<std::array<char, PACKET_SIZE>, boost::lockfree::capacity<QUEUE_CAPACITY>>;

class MulticastReceiver
{
private:
  std::vector<uint32_t> joined_ips{};
  std::array<int, PORTS_COUNT> socks_{};
  int epoll_fd_{0};
  spscQueue_t &queue_;

  void makeSocketNonBlocking()
  {
    for (auto sock : socks_)
    {
      int flags = fcntl(sock, F_GETFL, 0);
      if (flags == -1)
        ErrorHandler::handleError(errno);
      if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
        ErrorHandler::handleError(errno);
    }
  }

  int setupEpoll()
  {
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
      ErrorHandler::handleError(errno);

    epoll_event ev[PORTS_COUNT];
    for (uint8_t i = 0; i < PORTS_COUNT; i++)
    {

      ev[i].events = EPOLLIN | EPOLLET;
      ev[i].data.fd = socks_[i];

      if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socks_[i], &ev[i]) < 0)
        ErrorHandler::handleError(errno);
    }
    return epoll_fd;
  }

  std::array<int, PORTS_COUNT> Init_Sockets()
  {
    std::array<int, PORTS_COUNT> socks{};
    for (uint8_t i = 0; i < PORTS_COUNT; i++)
    {
      int sock = ::socket(AF_INET, SOCK_DGRAM, 0);
      while (true)
      {
        if (UNLIKELY(sock < 0))
        {
          ErrorHandler::handleError(errno);
          sock = ::socket(AF_INET, SOCK_DGRAM, 0);
          continue;
        }

        int reuse = 1;
        if (UNLIKELY(::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0))
        {
          ErrorHandler::handleError(errno);
          continue;
        }
      }

      sockaddr_in localAddr{
          AF_INET,
          htons(Ports[i]),
          {htonl(INADDR_ANY)},
          {}};

      while (true)
      {
        if (UNLIKELY(::bind(sock, reinterpret_cast<sockaddr *>(&localAddr), sizeof(localAddr)) < 0))
        {
          ErrorHandler::handleError(errno);
          continue;
        }
      }

      for (const auto &ip : IPs[i])
      {

        ip_mreq mreq{{ip}, {htonl(INADDR_ANY)}};
        while (true)
        {
          if (UNLIKELY(::setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0))
          {
            ErrorHandler::handleError(errno);
            continue;
          }
          joined_ips.push_back(ip);
          break;
        }
      }

      socks[i] = sock;
    }
    return socks;
  }

public:
  MulticastReceiver(spscQueue_t queue) noexcept
      : socks_(Init_Sockets()),
        epoll_fd_(setupEpoll()),
        queue_(queue)
  {
    makeSocketNonBlocking();
  }

  void run() noexcept
  {

    constexpr int MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];

    while (true)
    {
      int nfds = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
      if (UNLIKELY(nfds < 0))
      {
        if (LIKELY(errno == EINTR))
          continue;
        ErrorHandler::handleError(errno);
      }

      for (int i = 0; i < nfds; ++i)
      {
        int sock = events[i].data.fd;
        while (true)
        {
          std::array<char, PACKET_SIZE> packet{};
          ssize_t len = recvfrom(sock, packet.data(), packet.size(), 0, nullptr, nullptr);
          if (len < 0)
          {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
              break;
            ErrorHandler::handleError(errno);
          }
          else
          {
            static int lost_package{0};
            if (!queue_.push(std::move(packet)))
            {
              ++lost_package;
            }
          }
        }
      }
    }
  }

  ~MulticastReceiver()
  {
    auto ip = joined_ips.begin();

    for (const auto &sock : socks_)
    {
      while (ip != joined_ips.end())
      {
        ip_mreq mreq{{*ip}, {htonl(INADDR_ANY)}};
        if (::setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
        {
          break;
        }
        else
        {
          ip++;
        }
      }
      ::close(sock);
    }
  }
};
