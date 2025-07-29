// Receiver.h
#pragma once
#include "ErrorHandler.h"
#include "common.h"
#include "GeneratedIpPort.h"

#include <arpa/inet.h>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <cerrno>
#include <boost/lockfree/spsc_queue.hpp>
#include <array>
#include <vector>

inline constexpr size_t DATA_SIZE = 2048;
inline constexpr size_t QUEUE_CAPACITY = 1024;

struct Packet
{
  Venue venue;
  Protocol protocol;
  std::array<char, DATA_SIZE> data;
};

using spscQueue_t = boost::lockfree::spsc_queue<Packet, boost::lockfree::capacity<QUEUE_CAPACITY>>;

class Receiver
{
private:
  std::vector<uint32_t> joined_ips{};
  std::array<int, PORTS_COUNT> socks_{};
  int epoll_fd_{0};
  spscQueue_t &queue_;

  void makeSocketNonBlocking() noexcept;

  int setupEpoll() noexcept;

  std::array<int, PORTS_COUNT> Init_Sockets() noexcept;

public:
  Receiver(spscQueue_t &queue) noexcept;

  void receive() noexcept;

  ~Receiver() noexcept;
};
