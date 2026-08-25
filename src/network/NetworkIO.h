// =============================================================================
// NetworkIO
//
// PURPOSE:
// - Handles network I/O for market data.
// - Manages non-blocking socket send and receive operations.
//
// THREAD SAFETY:
// - Single-writer model assumed (network event loop thread).
// - No internal synchronization or locking.
// - Not safe for concurrent writers.
//
// PERFORMANCE & DESIGN NOTES:
// - Hot-path component with strict latency requirements.
// - Uses preallocated packet pools to avoid dynamic allocation.
// - Uses epoll event mechanism.
// - Kernel-bypass techniques are not used due to hardware limitations.
// - Receive and send operations intentionally share the same execution path.
//
// DEVELOPER NOTES (PRE-PROFILING):
// - After instruction-cache profiling, trySend() may be marked as noinline.
// - This class must be allocated on the heap due to large internal buffers.
// - If future profiling reveals sustained queue or buffer pressure send and 
//   receive paths may be split into seperate threads.
// =============================================================================

#pragma once

#include "common.h"
#include "SessionManager.h"
#include "NetworkPackets.h"
#include "CustomQueue.h"
#include "Protocol-Venue.h"

#include <array>
#include <cstdint>
#include <cstddef>

using common_function::to_underlying;

class LoginController;
class SoupBinTcp;
template <typename T, size_t Capacity> requires (std::is_pointer_v<T> && std::has_single_bit(Capacity))
class CustomQueue;

struct SocketState
{
  TxPacket* active_pkt = nullptr;
  uint64_t epoll_data = 0;
  bool is_EPOLLOUT_set = false;
  bool connection_pending = false;

  SocketState() noexcept = default;
  SocketState(uint64_t epoll_data) noexcept : epoll_data(epoll_data) {}
};

class NetworkIO
{
public: /// Debugging and monitoring fields. Will be removed after profiling and debugging.
    alignas(64) std::atomic<uint64_t> pipeline_seq{0}; // Used for debugging and monitoring the processing sequence of packets through the pipeline. Will be removed after profiling and debugging.
    char pad[56];

private:
public: /// Debugging and monitoring fields. Will be removed after profiling and debugging.
  static constexpr uint8_t MAX_RETRY_COUNT = 3;
  static constexpr size_t PARTIAL_PACKET_POOL_CAPACITY = 16;
  static constexpr size_t PENDING_CAPACITY = 256;

  using PendingIN = std::array<CustomQueue<TxPacket *, PENDING_CAPACITY>, MAX_SESSIONS>;
  using PendingOUT = std::array<CustomQueue<RxPacket *, PENDING_CAPACITY>, MAX_SESSIONS>; 
  using PacketPool = std::array<RxPacket, PACKET_POOL_CAPACITY>;
  using PartialPacketPool = std::array<RxPacket*, PARTIAL_PACKET_POOL_CAPACITY>;
  size_t next_partialpkt = 0;
  
  spscRxPacketQueue_t& receiver_to_parser_;
  spscTxPacketQueue_t& builder_to_sender_;
  SessionManager& sess_mngr_;
  SoupBinTcp& sbt_;
  LoginController& login_;
  TxPacketPoolManager& txPkt_pool_;

  std::array<uint32_t, MAX_SESSIONS> joined_ips{};
  int epoll_fd_{-1};
  std::array<SocketState, MAX_SESSIONS> socket_states_;
  std::array<int, MAX_SESSIONS> socks_{};

  PacketPool packet_pool_;
  spscRxPacketQueue_t free_pkt_list_;
  PartialPacketPool partial_packet_pool_;
  PendingIN pending_write_;
  PendingOUT pending_read_;

  const std::atomic<bool> &running_engine_;
  
public:
  NetworkIO(spscRxPacketQueue_t &receiver_to_parser, spscTxPacketQueue_t &builder_to_sender, SessionManager &sess_mngr, SoupBinTcp &sbt, LoginController &login, TxPacketPoolManager &txPkt_pool, const std::atomic<bool>& running_engine);
  ~NetworkIO();

  void recv_send();
  
  void releasePacket(RxPacket *pkt) noexcept
  {
    pkt->msg_count = 0;
    free_pkt_list_.push(pkt);
  }

  private:
  public: /// Debugging and monitoring fields. Will be removed after profiling and debugging.
    //COLD 
    std::array<int, MAX_SESSIONS> Init_Sockets();
    void addEpoll(int sock, uint64_t epoll_data, uint32_t events) const;
    void modEpoll(int sock, uint64_t epoll_data, uint32_t events) const noexcept;
    int setupSocket(int sock, const SessionContext& cxt);
    int reconnect(uint8_t index);
    void closeSocket(uint8_t index) noexcept;
    static void makeSocketNonBlocking(int sock) noexcept;
    static int createEpoll() noexcept;
    static int createSocket(size_t index, Protocol prot);
   
    //HOT
    void send(TxPacket &txPkt, int sock, uint8_t index, SocketState &states);
    void handleEINPROGRESS(int sock, SocketState &states, uint8_t index);
    bool SBTPacketHandler(RxPacket* rxPkt, size_t len, CustomQueue<RxPacket *, PENDING_CAPACITY>& pend_read, size_t index);
    
    void trySend(TxPacket &txPkt) 
    {
      size_t index = txPkt.sock_index;
      int sock = socks_[index];
      auto &states = socket_states_[index];
      
      if(LIKELY(!states.connection_pending))
      {
        if (UNLIKELY(!sess_mngr_.isSessionLoggedIn(index)) && !txPkt.is_login_msg)
          return;
      }
      else
      {
        pending_write_[index].push(&txPkt);
        return;
      }

      if (states.active_pkt)
      {
        pending_write_[index].push(&txPkt);
        return;
      }
      else
      {
        send(txPkt, sock, index, states);
      }
    }

    static uint64_t epollPackData(size_t socket_index, Venue ven, Protocol prot) noexcept
    {
      uint64_t data = 0;
      data |= static_cast<uint64_t>(socket_index);
      data |= (to_underlying(ven)) << 8;
      data |= (to_underlying(prot)) << 16;

      return data;
    } 

    template <typename T>
    static T epollUnpackData(uint64_t data) noexcept
    {
      if constexpr (std::is_same_v<T, size_t>)
        return static_cast<size_t>(data & 0xFF);
      else if constexpr (std::is_same_v<T, Venue>)
        return static_cast<Venue>((data >> 8) & 0xFF);
      else if constexpr (std::is_same_v<T, Protocol>)
        return static_cast<Protocol>((data >> 16) & 0xFF);
      else
        __builtin_unreachable();
      
    }

    
};
