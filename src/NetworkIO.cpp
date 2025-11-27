// `index` represents the session order as defined in the configuration file.
// The same index is used for all arrays associated with sockets and sessions.

#include "NetworkIO.h"

NetworkIO::NetworkIO(spscPacketQueue_t &receiver_to_parser, spscInPacketPayloadQueue_t &builder_to_sender, SoupBinTcp &sbt, SessionManager &sessions) noexcept
    : socks_(Init_Sockets()),
      epoll_fd_(createEpoll()),
      receiver_to_parser_(receiver_to_parser),
      builder_to_sender_(builder_to_sender),
      sbt_(sbt),
      sessions_(sessions)
{

   for (size_t i = 0; i < PACKET_QUEUE_CAPACITY; i++)
      free_pkt_list_.push(&packet_pool_[i]);
}

std::array<int, MAX_SESSIONS> NetworkIO::Init_Sockets() noexcept
{
   std::array<int, MAX_SESSIONS> socks{};
   for (size_t index = 0; index < MAX_SESSIONS; index++)
   {
      auto* cxt = sessions_.getSessionContext(index); // to ensure session contexts are initialized before socket creation
      if(!cxt)
         break;

      int sock = createSocket(index, cxt->protocol);
      sock = setupSocket(sock, *cxt);
      socks[index] = sock;
   }
  
   return socks;
}

int NetworkIO::createEpoll() noexcept
{
   int epoll_fd;
   while (true)
   {
      epoll_fd = epoll_create1(0);
      if (epoll_fd != -1)
         break;

      if (errno == EINTR)
         continue; 
   }

   return epoll_fd;
}

void NetworkIO::addEpoll(const int sock, const uint64_t epoll_data, const uint32_t events) noexcept
{
   epoll_event ev{};
   ev.events = events;
   ev.data.u64 = epoll_data;

   while (true)
   {
      if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, sock, &ev) != -1)
         break;

      if (errno == EINTR)
         continue;
   }
}

void NetworkIO::modEpoll(const int sock, const uint64_t epoll_data, const uint32_t events) noexcept
{

   epoll_event ev{};
   ev.events = events;
   ev.data.u64 = epoll_data;

   while (true)
   {
      if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, sock, &ev) != -1)
         break;

      if (errno == EINTR)
         continue;
   }
}

int NetworkIO::createSocket(const size_t index, Protocol prot) noexcept
{
   int sock_type = (prot == Protocol::ITCH) ? SOCK_DGRAM : SOCK_STREAM;
   int sock = -1;
   
   while (true)
   {
      // socket creation
      sock = ::socket(AF_INET, sock_type, 0);
      if(UNLIKELY(sock < 0)) 
      {
         switch (errno)
         {
            case EINTR:
               continue; 
            case EMFILE:
            case ENFILE:
            case ENOMEM:
            case EAFNOSUPPORT:
            case EPROTONOSUPPORT:
            default:
               LOG_ERROR("socket() failed with errno {} for index {}", errno, index);
               return -1;
         }
      }
      // socket created successfully

      // set socket options
      int reuse = 1;
      if (UNLIKELY(::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0))
      {
         switch (errno)
         {
            case EBADF:    
            case ENOTSOCK:
               LOG_ERROR("setsockopt failed (errno {}) for index {}, retrying with new socket", errno, index);
               ::close(sock);
               continue;
            case EINVAL:
            case ENOPROTOOPT:
            case EACCES:
            case EFAULT:
            default:
               LOG_ERROR("setsockopt(SO_REUSEADDR) failed (errno {}) for index {}", errno, index);
               ::close(sock);
               return -1;
         }
      }
      // socket options set successfully
      break;
   }
   
   return sock;
}

void NetworkIO::makeSocketNonBlocking(const int sock) noexcept
{
   int flags;

   while (true)
   {
      flags = fcntl(sock, F_GETFL, 0);
      if (flags != -1)
         break;

      if (errno == EINTR)
         continue;
   }

   while (true)
   {
      if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) != -1)
         break;

      if (errno == EINTR)
         continue;
   }
}

int NetworkIO::setupSocket(const int sock, const SessionContext& cxt) noexcept 
{
   uint8_t index = cxt.socket_index;
   uint64_t epoll_data = epollPackData(index, cxt.venue, cxt.protocol);
   socket_states_[index].epoll_data = epoll_data;

   sockaddr_in addr{};
   addr.sin_family = AF_INET;
   addr.sin_port = htons(cxt.port);
   memset(addr.sin_zero, 0, sizeof(addr.sin_zero));

   // socket connection for TCP
   if (cxt.protocol == Protocol::OUCH || cxt.protocol == Protocol::FIX) 
   {
      addr.sin_addr.s_addr = cxt.ip; 

      makeSocketNonBlocking(sock);

      if (UNLIKELY(::connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0))
      {
         switch (errno)
         {
         case EISCONN:
            return sock; 
         case EINPROGRESS:
            addEpoll(sock, index, EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLET);
            socket_states_[index].connection_pending = true;
            break;
         case ECONNREFUSED:
         case ETIMEDOUT:
         case EADDRINUSE:
            if(ErrorHandler::handleError(errno))
            {
               reconnect(index);
            }
            else
            {
               LOG_ERROR("Process Aborted after {} retries", MAX_RETRY_COUNT);
               closeSocket(index);
               return -1;
            }
            break;
         case ENETUNREACH:
         case EHOSTUNREACH:
         case EINVAL:
         case EAFNOSUPPORT:
         case EACCES:
         case EFAULT:
            ErrorHandler::handleError(errno);
            closeSocket(index);
            return -1;
         default:
            LOG_ERROR("Unknown error on connect: {}", errno);
            closeSocket(index);
            return -1;
         }
      } 
      else 
      {
         addEpoll(sock, epoll_data, EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);
         sessions_.setSessionConnected(index, true);

         if (cxt.protocol == Protocol::OUCH)
            sbtLogin(sock, index, socket_states_[index]);
      }
   }
      
   // socket binding for UDP
   else 
   {
      addr.sin_addr.s_addr = htonl(INADDR_ANY);
     
      if (UNLIKELY(::bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0))
      {
         switch (errno)
         {
            case EINTR:
            case ENOBUFS:
            case ENOMEM:
               
               if (ErrorHandler::handleError(errno))
               {
                  reconnect(index); 
               }
               else
               {
                  LOG_ERROR("Bind aborted after {} retries", MAX_RETRY_COUNT);
                  closeSocket(index);
                  return -1;
               }

            case EADDRINUSE:
            case EADDRNOTAVAIL:
            case EACCES:
            case EINVAL:
               ErrorHandler::handleError(errno);
               closeSocket(index);
               return -1;

            default:
               LOG_ERROR("Unknown bind error: {}", errno);
               closeSocket(index);
               return -1;
         }
      }
      else
      {
         makeSocketNonBlocking(sock);

         //multicast join and record joined IPs
         joined_ips[index] = 0;
         
            ip_mreq mreq{{cxt.ip}, {htonl(INADDR_ANY)}};
            
            if (UNLIKELY(::setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0))
            {
               switch (errno)
               {
               case EADDRINUSE:
                  break;

               case EADDRNOTAVAIL:
                  ErrorHandler::handleError(errno);
                  closeSocket(index);
                  return -1;

               case EINVAL:
                  ErrorHandler::handleError(errno);
                  closeSocket(index);
                  return -1;
               default:
                  LOG_ERROR("Unknown join-multicast error: {}", errno);
                  closeSocket(index);
                  return -1;
               }
            }
            sessions_.setSessionMulticast(index, true);
            joined_ips[index] = cxt.ip;
         
      
         if(joined_ips[index] == 0)
         {
            LOG_ERROR("No multicast groups joined for port {}", cxt.port);
            closeSocket(index);
            return -1;
         }

         addEpoll(sock, epoll_data, EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);
        
      }
   } 
   // socket connected/bound

   return sock;
}

void NetworkIO::closeSocket(const uint8_t index) noexcept
{
   int sock = socks_[index];

   if (sock == -1)
      return; // already closed

   epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, sock, nullptr);
   ::close(sock);
   sessions_.ResetState(index);
   socket_states_[index] = {};     
   pending_write_[index].clear();
   socks_[index] = -1;
}

int NetworkIO::reconnect(uint8_t index) noexcept
{
   auto *cxt = sessions_.getSessionContext(index);

   closeSocket(index);
   int sock = createSocket(index, cxt->protocol);
   setupSocket(sock, *cxt);
   return sock;
}

void NetworkIO::recv_send() noexcept
{
   constexpr int MAX_EVENTS = 10;
   epoll_event events[MAX_EVENTS];
   
   while (true)
   {
      InPacketPayload inPkt_payload;
      while (builder_to_sender_.pop(inPkt_payload))
      {
         InPacket* inPkt = &inpkt_pool[next_inpkt++ & (PACKET_POOL_CAPACITY -1)];
         std::memcpy(inPkt->data.data(), inPkt_payload.data, inPkt_payload.len);
         inPkt->len = inPkt_payload.len;
         inPkt->sock_index = inPkt_payload.sock_index;

         trySend(inPkt);
      }
      
      int nfds = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
      if (UNLIKELY(nfds < 0))
      {
         if (LIKELY(errno == EINTR))
            continue;
      }

      for (int i = 0; i < nfds; ++i)
      {
         uint64_t epoll_data = events[i].data.u64;
         size_t index = epollUnpackData<size_t>(epoll_data);
         int sock = socks_[index];
         auto& states = socket_states_[index];

         if(UNLIKELY(sock == -1)) // Double-check for closed sockets
            continue;

         //inPacket sending
         if (events[i].events & EPOLLOUT) // Check if event is writable (EPOLLOUT) ---
         {
            if(UNLIKELY(states.connection_pending))
            {
               handleEINPROGRESS(sock,states,index);
               continue;
            }
            else
            {
               send(states.active_pkt, sock, index, states);

               InPacket *inPkt;

               while (!states.active_pkt && pending_write_[index].pop(inPkt))
               {
                  send(inPkt, sock, index, states);      
               }

               if (!states.active_pkt)
               {
                  modEpoll(sock, states.epoll_data, EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);
                  states.is_EPOLLOUT_set = false;
               }
            }
         }

         // outPacket receiving
         if (events[i].events & EPOLLIN)
         {
            auto& pend_read = pending_read_[index];  

            while (true)
            {
               OutPacket *pending_pkt = nullptr;
               while (pend_read.pop(pending_pkt))
               {
                  if (!receiver_to_parser_.push(pending_pkt))
                  {
                     pend_read.push(pending_pkt);
                     break;
                  }
               }
              
               OutPacket *outPkt = nullptr;
               if (!free_pkt_list_.pop(outPkt))
                  break;

               ssize_t len = 0;    
               outPkt->protocol = epollUnpackData<Protocol>(epoll_data);
               outPkt->venue = epollUnpackData<Venue>(epoll_data);

               if (!(outPkt->protocol == Protocol::ITCH))
               {
                  len = ::recv(sock, outPkt->data.data(), outPkt->data.size(), 0);
                  outPkt->len = len;
               }
               else
               {
                  len = ::recvfrom(sock, outPkt->data.data(), outPkt->data.size(), 0, nullptr, nullptr);
                  outPkt->len = len;
               }


               if (len < 0)
               {
                  if (errno == EAGAIN || errno == EWOULDBLOCK)
                  {
                     break;
                  }
                  else if (errno == EINTR)
                  {
                     continue;
                  }
                  else
                  {
                     closeSocket(index);
                     break;
                  }
               }
               else if (len == 0)
               {
                  closeSocket(index);
                  break;
               }
               else
               {
                  if (outPkt->protocol == Protocol::OUCH)
                  {
                     uint16_t data_offset = 0;
                     auto *next_pkt_start = outPkt->data.data();
                     
                     OutPacket *outPkt_pending;
                     while (pend_read.pop(outPkt_pending))
                     {
                        if (outPkt_pending->half)
                        {
                           if(!outPkt_pending->is_len_received)
                           {
                              outPkt_pending->len = (outPkt_pending->len << 8) | *next_pkt_start;
                              outPkt_pending->last_pkt_remaining_len = outPkt_pending->len - 2;
                              outPkt_pending->is_len_received = true;
                              next_pkt_start += 1;
                              outPkt->len -= 1;
                              data_offset += 1;
                           }

                           uint16_t remaining = std::min(outPkt_pending->last_pkt_remaining_len, outPkt->len);

                           std::memcpy(outPkt_pending->data.data() + (outPkt_pending->len - outPkt_pending->last_pkt_remaining_len), 
                                       next_pkt_start, 
                                       remaining);

                           uint16_t remaining_after_next_pkt = outPkt_pending->last_pkt_remaining_len - remaining; 
                           
                           if (remaining_after_next_pkt == 0) // Packet completed
                           {            
                              outPkt_pending->half = false;
                              data_offset += remaining;
                              next_pkt_start += remaining;

                              if (!receiver_to_parser_.push(outPkt_pending))
                              {
                                 pend_read.push(outPkt_pending);
                                 break;
                              }
                           }
                           else // Wait next stream 
                           {
                              outPkt_pending->last_pkt_remaining_len -= remaining;
                              pend_read.push(outPkt_pending);
                              break;
                           }
                        }

                        if (!receiver_to_parser_.push(outPkt_pending))
                        {
                           pend_read.push(outPkt_pending);
                        }
                     }

                     while (len > data_offset)
                     {
                        uint16_t pkt_len;
                        bool len_half = false; 
                        
                        if (len - data_offset == 1)
                        {
                           pkt_len = *next_pkt_start;
                           len_half = true;
                        }
                        else
                           pkt_len = Endian::read_u16_be(next_pkt_start);

                        
                        if (pkt_len + data_offset > len || len_half)
                        {
                           OutPacket *outPkt_half = half_packet_pool_[next_halfpkt++ & (HALF_PACKET_POOL_CAPACITY - 1)];

                           std::memcpy(outPkt_half->data.data(), next_pkt_start, len - data_offset);
                           outPkt_half->half = true;
                           outPkt_half->len = pkt_len;

                           if(len_half)
                              outPkt_half->is_len_received = false;

                           outPkt_half->last_pkt_remaining_len = pkt_len - (len - data_offset);
                           outPkt_half->protocol = outPkt->protocol;
                           outPkt_half->venue = outPkt->venue;
                           pend_read.push(outPkt_half);

                           break;
                        }

                        auto sbt_pair = sbt_.pkt_handler(next_pkt_start, index);
                        data_offset += pkt_len;
                        next_pkt_start += pkt_len;

                        switch (sbt_pair.first)
                        {
                        case SBTAction::ToParser:
                           outPkt->offsets[outPkt->msg_count++] = data_offset;
                           break;

                        case SBTAction::Reconnect:
                           reconnect(index);
                           break;

                        case SBTAction::Disconnect:
                           closeSocket(index);
                           break;

                        default:
                           break;
                        }  
                     }
                  }
                  
                  if (!receiver_to_parser_.push(outPkt))
                  {
                     pend_read.push(outPkt);
                     
                     if (pend_read.back() && (*pend_read.back())->half)
                       pend_read.swap_last_two();
                     
                     break;
                  }
               }
            }
         }
      }
   }
}

void NetworkIO::trySend(InPacket *inPkt) noexcept
{
   size_t index = inPkt->sock_index;
   int sock = socks_[index];
   auto& states = socket_states_[index];

   if (UNLIKELY(!sessions_.isSessionLoggedIn(index)))
   {
      return;
   }

   if (states.active_pkt)
   {
      pending_write_[index].push(inPkt);
      return;
   }
   else
   {
      send(inPkt, sock, index, states);
   }
}

void NetworkIO::send(InPacket *inPkt, const int sock, const uint8_t index, SocketState &states) noexcept
{
   while (inPkt->offset < inPkt->len)
   {
      ssize_t sent = ::send(sock, inPkt->data.data() + inPkt->offset, inPkt->len - inPkt->offset,0);

      if(sent > 0)
      {
         inPkt->offset += sent;
         continue;
      }
      else if (sent < 0)
      {
         if (errno == EAGAIN || errno == EWOULDBLOCK)
         {
            if (!states.is_EPOLLOUT_set)
            {
               modEpoll(sock, states.epoll_data, EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLET);
               states.is_EPOLLOUT_set = true;
            }

            states.active_pkt = inPkt;
            return;
         }
         else
         {
            if (errno == EINTR)
               continue;

            closeSocket(index);
            LOG_ERROR("Fatal error ({}) occurred on send", errno);
            return;
         }
      }
   }
   states.active_pkt = nullptr;
}

void NetworkIO::handleEINPROGRESS(const int sock, SocketState& states, const uint8_t index) noexcept 
{
      int error = 0;
      socklen_t len = sizeof(error);

      if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0)
      {
         LOG_ERROR("getsockopt failed for index {}", index);
         states.connection_pending = false;

         if (ErrorHandler::handleError(errno))
         {
            reconnect(index);
         }
         else
         {
            LOG_ERROR("Process Aborted after {} retries", MAX_RETRY_COUNT);
            closeSocket(index);
         }

         return;
      }

      if (error != 0)
      {
         LOG_ERROR("Connection failed for index {} with error {}", index, error);
         states.connection_pending = false;

         if (ErrorHandler::handleError(error))
         {
            reconnect(index);
         }
         else
         {
            LOG_ERROR("Process Aborted after {} retries", MAX_RETRY_COUNT);
            closeSocket(index);
         }

         return;
      }

      states.connection_pending = false;
      sessions_.setSessionConnected(index, true);
      modEpoll(sock, states.epoll_data, EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET);
      states.is_EPOLLOUT_set = false;

      if(sessions_.getSessionContext(index)->protocol == Protocol::OUCH) 
        sbtLogin(sock, index, states);
      
   }

NetworkIO::~NetworkIO() noexcept
{
   for (uint8_t index = 0; index < MAX_SESSIONS; index++)
   {
      if (joined_ips[index] != 0)
      {
            ip_mreq mreq{{joined_ips[index]}, {htonl(INADDR_ANY)}};
            if (::setsockopt(socks_[index], IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) 
               LOG_ERROR("DROP_MEMBER_ERROR");
      }
      closeSocket(index);
   }
}