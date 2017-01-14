//
// TouchBarDinoXServer.cpp
//
// Modified based on Christopher M. Kohlhoff's Boost Tutorial File
//

#include <ctime>
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include "SimpleJSON/JSON.h"
#include "SimpleJSON/JSONValue.h"
#include "DBManager.cpp"

using boost::asio::ip::tcp;

std::string make_daytime_string() {
  using namespace std;
  time_t now = time(0);
  return ctime(&now);
}

class tcp_connection
  : public boost::enable_shared_from_this<tcp_connection> {
public:
  typedef boost::shared_ptr<tcp_connection> pointer;

  static pointer create(boost::asio::io_service& io_service) {
    return pointer(new tcp_connection(io_service));
  }

  tcp::socket& socket() {
    return socket_;
  }
  
  void print_data_error() {
    timestamp_ = make_daytime_string();
    std::cout << timestamp_.substr(0, timestamp_.size()-1) << " " << "Data error." << std::endl;
    
    message_ = "Data error.\n";
    
    /*boost::asio::async_write(socket_, boost::asio::buffer(message_),
        boost::bind(&tcp_connection::handle_write, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));*/
  }

  void start() {
    message_ = "Welcome to TouchBarDinoXServer\n";

    /*boost::asio::async_write(socket_, boost::asio::buffer(message_),
        boost::bind(&tcp_connection::handle_write, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));*/
    
    for(;;) {
      boost::array<char, 1024> buf;
      boost::system::error_code error;

      size_t len = socket_.read_some(boost::asio::buffer(buf), error);

      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error)
        throw boost::system::system_error(error); // Some other error.

      std::string json_string(buf.begin(), buf.begin()+len);
      const char *json_string_c = json_string.c_str();
      JSONValue *data = JSON::Parse(json_string_c);
      
      if (data == NULL || data->IsObject() == false) {
        print_data_error();
        return;
      }
      
      std::string player_id;
      std::string player_name;
      double player_score;
      std::string player_ip;
      std::string player_system;
      std::string timestamp;
      
      std::wstring temp;
      
      JSONObject root = data->AsObject();
      if (root.find(L"player_id")!= root.end() && root[L"player_id"]->IsString()) {
        temp = root[L"player_id"]->AsString();
        player_id = std::string(temp.begin(), temp.end());
      } else {
        print_data_error();
        return;
      }
      
      if (root.find(L"player_name")!= root.end() && root[L"player_name"]->IsString()) {
        temp = root[L"player_name"]->AsString();
        player_name = std::string(temp.begin(), temp.end());
      } else {
        print_data_error();
        return;
      }
      
      if (root.find(L"player_score")!= root.end() && root[L"player_score"]->IsNumber()) {
        player_score = root[L"player_score"]->AsNumber();
      } else {
        print_data_error();
        return;
      }
      
      player_ip = socket_.remote_endpoint().address().to_string();
      
      if (root.find(L"player_system")!= root.end() && root[L"player_system"]->IsString()) {
        temp = root[L"player_system"]->AsString();
        player_system = std::string(temp.begin(), temp.end());
      } else {
        print_data_error();
        return;
      }
      
      std::time_t seconds = std::time(nullptr);
      std::stringstream ss;
      timestamp = ss.str();
      
      int rank = -1;
      
      database_manager *dbm = new database_manager();
      if (dbm->write(player_id, player_name, player_score, player_ip, player_system, timestamp, &rank)) {
        message_ = "{\"status\":0, \"rank\":"+std::to_string(rank)+"}\n";
      } else {
        message_ = "{\"status\":1}\n";
      }
      
      boost::asio::async_write(socket_, boost::asio::buffer(message_),
        boost::bind(&tcp_connection::handle_write, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
      
    }
  }

private:
  tcp_connection(boost::asio::io_service& io_service)
    : socket_(io_service) {
  }

  void handle_write(const boost::system::error_code& /*error*/,
      size_t /*bytes_transferred*/) {
    timestamp_ = make_daytime_string();
    std::cout << timestamp_.substr(0, timestamp_.size()-1) << " " << "Sent: " << message_ << std::endl;
  }

  tcp::socket socket_;
  std::string message_;
  std::string timestamp_;
};

class tcp_server {
public:
  tcp_server(boost::asio::io_service& io_service)
    : acceptor_(io_service, tcp::endpoint(tcp::v4(), 1300)) {
    start_accept();
  }

private:
  void start_accept() {
    tcp_connection::pointer new_connection =
      tcp_connection::create(acceptor_.get_io_service());

    acceptor_.async_accept(new_connection->socket(),
        boost::bind(&tcp_server::handle_accept, this, new_connection,
          boost::asio::placeholders::error));
  }

  void handle_accept(tcp_connection::pointer new_connection,
      const boost::system::error_code& error) {
    if (!error) {
      new_connection->start();
    }

    start_accept();
  }

  tcp::acceptor acceptor_;
};

int main() {
  try {
    boost::asio::io_service io_service;
    tcp_server server(io_service);
    io_service.run();
  }
  catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
