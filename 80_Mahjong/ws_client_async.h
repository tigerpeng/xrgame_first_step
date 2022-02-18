//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket client, asynchronous
//
//------------------------------------------------------------------------------
#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <queue>
#include <mutex>

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>


////https://github.com/gabime/spdlog
//#include "spdlog/spdlog.h"
//namespace spd = spdlog;
//------------------------------------------------------------------------------


// Sends a WebSocket message and prints the response
class ws_session : public std::enable_shared_from_this<ws_session>
{
private:
    typedef std::function<void(boost::system::error_code ec,std::string const&)>        WSCALLBACK;
    
    // Report a failure
    void
    fail(boost::system::error_code ec, char const* what)
    {
        //spd::get("console")->error("client side----->fail what={} e.msg={}",what,ec.message());
        //std::cerr << what << ": " << ec.message() << "\n";
    }
    
    tcp::resolver resolver_;
    websocket::stream<tcp::socket> ws_;
    boost::beast::multi_buffer buffer_;
    std::string host_;
    std::string path_;
    std::string text_;
    
    bool                                        sending_;//正在发送
    std::queue<std::string>                     texts_;
    std::mutex                                  mtx_;
    
    std::mutex                                  read_mtx_;
    
    
    WSCALLBACK                                  callback_;
public:
    // Start the asynchronous operation
    void run(std::string const&url,char const* text);
    void run(char const* host,char const* port,char const* path,char const* text);
    void stop();
    
    void write(std::string& strText);
    
    
    // Resolver and socket require an io_context
    explicit ws_session(boost::asio::io_context& ioc,WSCALLBACK cb);
private:
    void on_resolve(boost::system::error_code ec,tcp::resolver::results_type results);
    void on_connect(boost::system::error_code ec);
    void on_handshake(boost::system::error_code ec);
    void on_write(boost::system::error_code ec,std::size_t bytes_transferred);
    void on_read(boost::system::error_code ec,std::size_t bytes_transferred);
    void on_close(boost::system::error_code ec);
};
