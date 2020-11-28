#ifndef P2P_SERVER_H_
#define P2P_SERVER_H_

#include <memory>
#include <string>

// nlohmann/json
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/system/error_code.hpp>

#include "p2p_session.h"
#include "rtc/rtc_manager.h"
#include "util.h"

typedef P2PSessionConfig P2PServerConfig;

class P2PServer : public std::enable_shared_from_this<P2PServer> {
  P2PServer(boost::asio::io_context& ioc,
            boost::asio::ip::tcp::endpoint endpoint,
            RTCManager* rtc_manager,
            P2PServerConfig config);

 public:
  static std::shared_ptr<P2PServer> Create(
      boost::asio::io_context& ioc,
      boost::asio::ip::tcp::endpoint endpoint,
      RTCManager* rtc_manager,
      P2PServerConfig config) {
    return std::shared_ptr<P2PServer>(
        new P2PServer(ioc, endpoint, rtc_manager, std::move(config)));
  }
  void Run();

 private:
  void DoAccept();
  void OnAccept(boost::system::error_code ec);

 private:
  boost::asio::io_context& ioc_;
  boost::asio::ip::tcp::acceptor acceptor_;
  boost::asio::ip::tcp::socket socket_;

  RTCManager* rtc_manager_;
  P2PServerConfig config_;
};

#endif
