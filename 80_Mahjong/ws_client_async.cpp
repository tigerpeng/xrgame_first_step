#include "ws_client_async.h"
#include "url.hpp"


ws_session::ws_session(boost::asio::io_context& ioc,WSCALLBACK cb)
: resolver_(ioc)
, ws_(ioc)
, sending_(true)
, callback_(cb)
{
}

void ws_session::run(std::string const&url,char const* text)
{
    Url    parseUrl(url.c_str());
    std::string host        = parseUrl.GetHost();
    std::string port        = parseUrl.GetPort();
    std::string target      = parseUrl.GetPath() + parseUrl.GetQuery();
    
    run(host.c_str(),port.c_str(),target.c_str(),text);
}


// Start the asynchronous operation
void ws_session::run(char const* host,char const* port,char const* path,char const* text)
{
    // Save these for later
    host_   = host;
    path_   = path;
    text_   = text;
    
    if(path_.empty())
        path_="/";
    
    
    // Look up the domain name
    resolver_.async_resolve(
                            host,
                            port,
                            std::bind(
                                      &ws_session::on_resolve,
                                      shared_from_this(),
                                      std::placeholders::_1,
                                      std::placeholders::_2));
}

void ws_session::stop()
{
    std::lock_guard<std::mutex> lck(read_mtx_);
    
    callback_=nullptr;
    boost::beast::websocket::close_reason cr("shutdown by myself");
    ws_.close(cr);
}
void ws_session::write(std::string& strText)
{
    std::lock_guard<std::mutex> lck(mtx_);
    if(strText.empty())
    {
        //spd::get("toyCatch")->warn("空数据");
        return ;
    }
    
    if(sending_)
    {
        texts_.push(strText);
        return ;
    }
    
    sending_=true;

    text_=strText;
    
    
    
    //spd::get("console")->error("dump client sending text={}",text_);
    // Send the message
    ws_.text(true);
    ws_.async_write(
                    boost::asio::buffer(text_),
                    std::bind(
                              &ws_session::on_write,
                              shared_from_this(),
                              std::placeholders::_1,
                              std::placeholders::_2));
    
    
}


void ws_session::on_resolve(
           boost::system::error_code ec,
           tcp::resolver::results_type results)
{
    if(ec)
        return fail(ec, "resolve");
    
    // Make the connection on the IP address we get from a lookup
    boost::asio::async_connect(
                               ws_.next_layer(),
                               results.begin(),
                               results.end(),
                               std::bind(
                                         &ws_session::on_connect,
                                         shared_from_this(),
                                         std::placeholders::_1));
}

void ws_session::on_connect(boost::system::error_code ec)
{
    if(ec)
        return fail(ec, "connect");
    
    // Perform the websocket handshake
    ws_.async_handshake(host_, path_,
                        std::bind(
                                  &ws_session::on_handshake,
                                  shared_from_this(),
                                  std::placeholders::_1));
}

void ws_session::on_handshake(boost::system::error_code ec)
{
    if(ec)
        return fail(ec, "handshake");
    
    if(!text_.empty())
    {
        //spd::get("console")->error("dump client sending text={}",text_);
        // Send the message
        ws_.text(true);
        ws_.async_write(
                        boost::asio::buffer(text_),
                        std::bind(
                                  &ws_session::on_write,
                                  shared_from_this(),
                                  std::placeholders::_1,
                                  std::placeholders::_2));
    }else
    {
        sending_=false;
    }
    
    
    
    // Read a message into our buffer
    ws_.async_read(
                   buffer_,
                   std::bind(
                             &ws_session::on_read,
                             shared_from_this(),
                             std::placeholders::_1,
                             std::placeholders::_2));
}



void ws_session::on_write(boost::system::error_code ec,std::size_t bytes_transferred)
{
    std::lock_guard<std::mutex> lck(mtx_);
    
    boost::ignore_unused(bytes_transferred);
    
    if(ec)
        return fail(ec, "write");
    
    try {
        if(texts_.size()>0)
        {
            sending_=true;
            
            
            text_=texts_.front();
            texts_.pop();
            
            // Send the message
            ws_.text(true);
            ws_.async_write(
                            boost::asio::buffer(text_),
                            std::bind(
                                      &ws_session::on_write,
                                      shared_from_this(),
                                      std::placeholders::_1,
                                      std::placeholders::_2));
        }else
        {
            sending_=false;
        }
    } catch (...) {
         //spd::get("console")->error("未知的异常，可能导致程序崩溃");
    }
    

}

void ws_session::on_read(boost::system::error_code ec,std::size_t bytes_transferred)
{
    std::lock_guard<std::mutex> lck(read_mtx_);
    
    boost::ignore_unused(bytes_transferred);

    if(ec)
    {
        // Close the WebSocket connection
        ws_.async_close(websocket::close_code::normal,
                        std::bind(
                                  &ws_session::on_close,
                                  shared_from_this(),
                                  std::placeholders::_1));
        
        return fail(ec, "read");
    }
    
    
    try {
        std::string strReturn=buffers_to_string(buffer_.data());
        
        if(callback_)
            callback_(ec,strReturn);
        
        
        //清空缓存
        buffer_.consume(buffer_.size());
        // loop Read a message into our buffer
        ws_.async_read(
                       buffer_,
                       std::bind(
                                 &ws_session::on_read,
                                 shared_from_this(),
                                 std::placeholders::_1,
                                 std::placeholders::_2));
    } catch (...) {
            //spd::get("console")->error("未知的异常，可能导致程序崩溃");
    }
}

void ws_session::on_close(boost::system::error_code ec)
{
    sending_=true;
    if(callback_)
        callback_(ec,"");
    
    if(ec)
        return fail(ec, "close");
    
    //// If we get here then the connection is closed gracefully
    //// The buffers() function helps print a ConstBufferSequence
    //std::cout << boost::beast::buffers(buffer_.data()) << std::endl;
    //spd::get("console")->error("on_close::the connection is closed gracefully buffer={}", boost::beast::buffers(buffer_.data()));
}
