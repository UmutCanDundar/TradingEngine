// NetworkIO.h
#pragma once
#include "ErrorHandler.h"
#include "common.h"
#include "GeneratedIpPort.h"
#include "Logger.h"
#include "SoupBinTcp.h"
#include "SessionManager.h"

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

struct InPacketPayload
{
  char *data;
  uint16_t len;
  uint8_t sock_index;

  InPacketPayload() noexcept = default;
  InPacketPayload(char* data, uint16_t len, uint16_t sock_index) noexcept : data(data), len(len), sock_index(sock_index) 
  {}
};

struct InPacket // Inbound Message Struct
{
  static constexpr size_t DATA_SIZE = 2048;

  std::array<char, DATA_SIZE> data;
  uint16_t len;
  uint16_t offset;
  uint8_t sock_index;
 
  InPacket() noexcept = default;
  InPacket(char *msg, size_t len, uint8_t sock_index) noexcept : len(len), offset(0), sock_index(sock_index)
  {
    std::memcpy(data.data(), msg, len);
  }
};

struct OutPacket  // Outbound Message Struct
{
  static constexpr size_t DATA_SIZE = 2048;
  static constexpr size_t MAX_APP_MSG_IN_ONE_DATA = 32;

  std::array<char, DATA_SIZE> data;
  std::array<uint16_t, MAX_APP_MSG_IN_ONE_DATA> offsets;
  uint16_t len;
  uint16_t last_pkt_remaining_len = 0;
  uint8_t msg_count = 0;
  bool half = false;
  bool is_len_received = true;
  Venue venue;
  Protocol protocol;
};

struct SocketState
{
  InPacket* active_pkt = nullptr;
  uint64_t epoll_data = 0;
  bool is_EPOLLOUT_set = false;
  bool connection_pending = false;

  SocketState() noexcept = default;
  SocketState(uint64_t epoll_data) noexcept : epoll_data(epoll_data) {}
};

inline constexpr size_t PACKET_QUEUE_CAPACITY = 1024;
using spscInPacketPayloadQueue_t = boost::lockfree::spsc_queue<InPacketPayload, boost::lockfree::capacity<PACKET_QUEUE_CAPACITY>>;
using spscPacketQueue_t = boost::lockfree::spsc_queue<OutPacket *, boost::lockfree::capacity<PACKET_QUEUE_CAPACITY>>;

class NetworkIO
{
private:
  static constexpr uint8_t MAX_RETRY_COUNT = 3;
  static constexpr size_t PACKET_POOL_CAPACITY = 1024;
  static constexpr size_t HALF_PACKET_POOL_CAPACITY = 16;

  using PacketPool = std::array<OutPacket, PACKET_POOL_CAPACITY>;
  using HalfPacketPool = std::array<OutPacket*, HALF_PACKET_POOL_CAPACITY>;
  size_t next_halfpkt = 0;
  using InPacketPool = std::array<InPacket, PACKET_POOL_CAPACITY>;
  size_t next_inpkt = 0;

  std::array<uint32_t, MAX_SESSIONS> joined_ips{};
  std::array<int, MAX_SESSIONS> socks_{};
  int epoll_fd_{-1};
  std::array<SocketState, MAX_SESSIONS> socket_states_{};
  
  PacketPool packet_pool_;
  spscPacketQueue_t free_pkt_list_;
  HalfPacketPool half_packet_pool_;
  InPacketPool inpkt_pool;

  spscPacketQueue_t &receiver_to_parser_;
  spscInPacketPayloadQueue_t &builder_to_sender_;
  SoupBinTcp &sbt_;
  SessionManager &sessions_;

public:
  NetworkIO(spscPacketQueue_t &receiver_to_parser, spscInPacketPayloadQueue_t &builder_to_sender, SoupBinTcp &sbt, SessionManager &sessions) noexcept;

  void recv_send() noexcept;
  
  inline void releasePacket(OutPacket *pkt) noexcept
  {
    pkt->msg_count = 0;
    free_pkt_list_.push(pkt);
  }

  ~NetworkIO() noexcept;

  private:
    void makeSocketNonBlocking(int sock) noexcept;
    int createEpoll() noexcept;
    void modEpoll(const int sock, const uint64_t epoll_data, const uint32_t events) noexcept;
    void addEpoll(const int sock, const uint64_t epoll_data, const uint32_t events) noexcept;
    int setupSocket(int sock, const SessionContext& cxt) noexcept;
    int createSocket(size_t index, Protocol prot) noexcept;
    void send(InPacket *inPkt, const int sock, const uint8_t index, SocketState &states) noexcept;
    void trySend(InPacket *inPkt) noexcept;
    int reconnect(uint8_t index) noexcept;
    void closeSocket(uint8_t index) noexcept;
    void handleEINPROGRESS(const int sock, SocketState &states, const uint8_t index) noexcept;
    std::array<int, MAX_SESSIONS> Init_Sockets() noexcept;

    inline void sbtLogin(const int sock, const uint8_t index, SocketState &states) noexcept
    {
      InPacket *login_pkt = &inpkt_pool[next_inpkt++ & (PACKET_POOL_CAPACITY - 1)];

      if (!sessions_.isSessionLoggedInBefore(index))
        sbt_.buildFirstLoginRequest(login_pkt->data.data(), index);
      else
        sbt_.buildReLoginRequest(login_pkt->data.data(), index);

      login_pkt->len = 47;
      login_pkt->sock_index = index;

      send(login_pkt, sock, index, states);
    }

    static inline uint64_t epollPackData(size_t socket_index, Venue ven, Protocol prot) noexcept
    {
      uint64_t data = 0;
      data |= static_cast<uint64_t>(socket_index) & 0xFFFF;
      data |= (static_cast<uint64_t>(static_cast<uint8_t>(ven)) & 0xFF) << 16;
      data |= (static_cast<uint64_t>(static_cast<uint8_t>(prot)) & 0xFF) << 24;

      return data;
    } 

    template <typename T>
    static inline T epollUnpackData(uint64_t data) noexcept
    {
      if constexpr (std::is_same_v<T, size_t>)
        return static_cast<size_t>(data & 0xFFFF);
      else if constexpr (std::is_same_v<T, Venue>)
        return static_cast<Venue>((data >> 16) & 0xFF);
      else if constexpr (std::is_same_v<T, Protocol>)
        return static_cast<Protocol>((data >> 24) & 0xFF);
      else
        __builtin_unreachable();
      
    }

    template <typename T, size_t N>
    class PendingQueue
    {
      T buf[N];
      size_t head = 0;
      size_t tail = 0;

    public:
      bool push(T b) noexcept
      {
        size_t next = (tail + 1) & (N-1);
        if (next == head)
          return false; // full
        buf[tail] = b;
        tail = next;
        return true;
      }

      bool pop(T &b) noexcept
      {
        if (head == tail)
          return false; // empty
        b = buf[head];
        head = (head + 1) & (N-1);
        return true;
      }

      void clear() noexcept
      {
        head = tail = 0;
      }

      bool empty() noexcept 
      {
        return head == tail;
      }

      T* back() noexcept
      {
        if(head == tail) 
          return nullptr; // empty

        size_t back = (tail == 0) ? (N - 1) : (tail - 1); 
        return &buf[back];
      }

      void swap_last_two() noexcept 
      {
        if ((tail == head) || ((tail + N - 1) & (N - 1)) == head)
          return; 

        size_t pre_last = (tail + N - 2) & (N - 1); 
        size_t last = (tail + N - 1) & (N - 1);
        std::swap(buf[pre_last], buf[last]);
      }

    };

    static constexpr size_t PENDING_CAPACITY = 256;
    std::array<PendingQueue<InPacket*, PENDING_CAPACITY>, MAX_SESSIONS> pending_write_;
    std::array<PendingQueue<OutPacket*, PENDING_CAPACITY>, MAX_SESSIONS> pending_read_;
};
