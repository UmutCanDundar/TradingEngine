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

struct alignas(64) Packet
{
  static constexpr size_t DATA_SIZE = 2048;

  Venue venue;
  Protocol protocol;
  std::array<char, DATA_SIZE> data;
};

inline constexpr size_t PACKET_QUEUE_CAPACITY = 1024;

using spscPacketQueue_t = boost::lockfree::spsc_queue<Packet *, boost::lockfree::capacity<PACKET_QUEUE_CAPACITY>>;
using PacketPool = std::vector<Packet>;

class Receiver
{
private:
  std::vector<uint32_t> joined_ips{};
  std::array<int, PORTS_COUNT> socks_{};
  int epoll_fd_{0};

  spscPacketQueue_t &receiver_to_parser_;
  PacketPool packet_pool_;
  spscPacketQueue_t free_pkt_list_;

  void makeSocketNonBlocking() noexcept;
  int setupEpoll() noexcept;
  std::array<int, PORTS_COUNT> Init_Sockets() noexcept;

public:
  Receiver(spscPacketQueue_t &receiver_to_parser) noexcept;

  void receive() noexcept;

  inline void releasePacket(Packet *pkt) noexcept
  {
    free_pkt_list_.push(pkt);
  }

  ~Receiver() noexcept;
};
