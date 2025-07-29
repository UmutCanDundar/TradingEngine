#include "Receiver.h"

void Receiver::makeSocketNonBlocking() noexcept
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

int Receiver::setupEpoll() noexcept
{
   int epoll_fd = epoll_create1(0);
   if (epoll_fd == -1)
      ErrorHandler::handleError(errno);

   epoll_event ev[PORTS_COUNT];
   for (uint8_t i = 0; i < PORTS_COUNT; i++)
   {
      ev[i].events = EPOLLIN | EPOLLET;
      ev[i].data.u32 = i;

      if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, socks_[i], &ev[i]) < 0)
         ErrorHandler::handleError(errno);
   }
   return epoll_fd;
}

std::array<int, PORTS_COUNT> Receiver::Init_Sockets() noexcept
{
   joined_ips.reserve(IPs_COUNT);

   std::array<int, PORTS_COUNT> socks{};
   for (uint8_t i = 0; i < PORTS_COUNT; i++)
   {
      int sock_type = (PortProtocol[i] == Protocol::FIX) ? SOCK_STREAM : SOCK_DGRAM;
      int sock = ::socket(AF_INET, sock_type, 0);

      while (true)
      {
         if (UNLIKELY(sock < 0))
         {
            ErrorHandler::handleError(errno);
            sock = ::socket(AF_INET, sock_type, 0);
            continue;
         }

         int reuse = 1;
         if (UNLIKELY(::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0))
         {
            ErrorHandler::handleError(errno);
            continue;
         }
         break;
      }

      if (PortProtocol[i] == Protocol::FIX)
      {
         sockaddr_in addr{
             AF_INET,
             htons(Ports[i]),
             {IPs[i][0]},
             {}};

         while (true)
         {
            if (UNLIKELY(::connect(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0))
            {
               ErrorHandler::handleError(errno);
               continue;
            }
            break;
         }
      }
      else
      {
         sockaddr_in addr{
             AF_INET,
             htons(Ports[i]),
             {htonl(INADDR_ANY)},
             {}};

         while (true)
         {
            if (UNLIKELY(::bind(sock, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0))
            {
               ErrorHandler::handleError(errno);
               continue;
            }
            break;
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
      }
      socks[i] = sock;
   }
   return socks;
}

Receiver::Receiver(spscQueue_t &queue) noexcept
    : socks_(Init_Sockets()),
      epoll_fd_(setupEpoll()),
      queue_(queue)
{
   makeSocketNonBlocking();
}

void Receiver::receive() noexcept
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
         int sock = socks_[events[i].data.u32];
         while (true)
         {
            Packet pkt{};
            ssize_t len = 0;

            if (PortProtocol[events[i].data.u32] == Protocol::FIX)
            {
               len = ::recv(sock, pkt.data.data(), pkt.data.size(), 0);
            }
            else
            {
               len = ::recvfrom(sock, pkt.data.data(), pkt.data.size(), 0, nullptr, nullptr);
            }

            if (len < 0)
            {
               if (errno == EAGAIN || errno == EWOULDBLOCK)
                  break;
               ErrorHandler::handleError(errno);
            }
            else
            {
               pkt.protocol = PortProtocol[events[i].data.u32];
               pkt.venue = PortVenue[events[i].data.u32];
               static int lost_package{0};
               if (!queue_.push(std::move(pkt)))
               {
                  ++lost_package;
                  // Daha sonra loglama yapılacak.
               }
            }
         }
      }
   }
}

Receiver::~Receiver() noexcept
{
   auto ip = joined_ips.begin();

   for (uint8_t i = 0; i < PORTS_COUNT; i++)
   {
      if (PortProtocol[i] != Protocol::FIX)
      {
         while (ip != joined_ips.end())
         {
            ip_mreq mreq{{*ip}, {htonl(INADDR_ANY)}};
            if (::setsockopt(socks_[i], IPPROTO_IP, IP_DROP_MEMBERSHIP, &mreq, sizeof(mreq)) < 0)
            {
               break;
            }
            else
            {
               ip++;
            }
         }
      }
      ::close(socks_[i]);
   }
}