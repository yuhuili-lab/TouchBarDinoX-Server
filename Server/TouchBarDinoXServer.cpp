//
// TouchBarDinoXServer.cpp
//
// Modified based on Christopher M. Kohlhoff's Boost Tutorial File
//

#include <ctime>
#include <memory>
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
#include <sqlite3.h>
#include "SimpleJSON/JSON.h"
#include "SimpleJSON/JSONValue.h"

using boost::asio::ip::tcp;

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]); 
    snprintf(buf.get(), size, format.c_str(), args ... );
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}

std::string make_daytime_string() {
  using namespace std; // For time_t, time and ctime;
  time_t now = time(0);
  return ctime(&now);
}

class database_manager {
public:
  bool write(std::string& player_id, std::string& player_name, double player_score, std::string& player_ip, std::string& player_system, std::string& timestamp, int *r) {
    rc = sqlite3_open("dino-test.db", &db);
    if (rc) {
      return false;
    }
    
    query = "INSERT INTO scoreboard (player_id, player_name, player_score, player_ip, player_system, timestamp) VALUES ('"+player_id+"','"+player_name+"',"+std::to_string(player_score)+",'"+player_ip+"','"+player_system+"','"+timestamp+"');";
    
    const char *cquery = query.c_str();
    
    rc = sqlite3_exec(db, cquery, 0, 0, &zErrMsg);
    if (rc!=SQLITE_OK){
      std::cout << "Database error" << std::endl;
      sqlite3_free(zErrMsg);
      return false;
    }
    
    lastInsertRow = sqlite3_last_insert_rowid(db);
    
    
    query = "SELECT \
        (\
          SELECT  COUNT(*)\
          FROM    scoreboard AS s2\
          WHERE   s2.player_score > s1.player_score\
        )\
        FROM    scoreboard AS s1\
        WHERE   s1.ROWID = "+std::to_string(lastInsertRow)+"";
        
    const char *cquery2 = query.c_str();
    int rank = 0;
    
    rc = sqlite3_exec(db, cquery2, insertCallback, &rank, &zErrMsg);
    if (rc!=SQLITE_OK){
      std::cout << "Database error" << std::endl;
      sqlite3_free(zErrMsg);
      return false;
    }
    
    *r = rank;
    
    std::cout << rank << std::endl;
    sqlite3_close(db);
    return true;
  }
  
private:
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;
  std::string query;
  int lastInsertRow = 0;
  
  static int insertCallback(void *rank, int argc, char **argv, char **azColName){
    int *c = (int*) rank;
    *c = atoi(argv[0]);
    return 0;
  }
};

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
    
    boost::asio::async_write(socket_, boost::asio::buffer(message_),
        boost::bind(&tcp_connection::handle_write, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
  }

  void start() {
    message_ = "Welcome to TouchBarDinoXServer\n";
    
    database_manager *dbm = new database_manager();
    
    /*
    if (!dbm->start()) {
      std::cout << timestamp_.substr(0, timestamp_.size()-1) << " " << "Database error, please try again later" << std::endl;
      return;
    }*/

    boost::asio::async_write(socket_, boost::asio::buffer(message_),
        boost::bind(&tcp_connection::handle_write, shared_from_this(),
          boost::asio::placeholders::error,
          boost::asio::placeholders::bytes_transferred));
    
    for(;;) {
      boost::array<char, 1024> buf;
      boost::system::error_code error;

      size_t len = socket_.read_some(boost::asio::buffer(buf), error);

      if (error == boost::asio::error::eof)
        break; // Connection closed cleanly by peer.
      else if (error)
        throw boost::system::system_error(error); // Some other error.

      //std::cout.write(buf.data(), len);

      std::string json_string(buf.begin(), buf.begin()+len);
      const char *json_string_c = json_string.c_str();
      JSONValue *data = JSON::Parse(json_string_c);
      
      //std::cout << "json_string:" << json_string << std::endl; 
      
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
        std::cout << "1" << std::endl;
        print_data_error();
        return;
      }
      
      if (root.find(L"player_name")!= root.end() && root[L"player_name"]->IsString()) {
        temp = root[L"player_name"]->AsString();
        player_name = std::string(temp.begin(), temp.end());
      } else {
        std::cout << "2" << std::endl;
        print_data_error();
        return;
      }
      
      if (root.find(L"player_score")!= root.end() && root[L"player_score"]->IsNumber()) {
        player_score = root[L"player_score"]->AsNumber();
      } else {
        std::cout << "3" << std::endl;
        print_data_error();
        return;
      }
      
      if (root.find(L"player_ip")!= root.end() && root[L"player_ip"]->IsString()) {
        temp = root[L"player_ip"]->AsString();
        player_ip = std::string(temp.begin(), temp.end());
      } else {
        std::cout << "4" << std::endl;
        print_data_error();
        return;
      }
      
      if (root.find(L"player_system")!= root.end() && root[L"player_system"]->IsString()) {
        temp = root[L"player_system"]->AsString();
        player_system = std::string(temp.begin(), temp.end());
      } else {
        std::cout << "5" << std::endl;
        print_data_error();
        return;
      }
      
      std::time_t seconds = std::time(nullptr);
      std::stringstream ss;
      ss << seconds;
      timestamp = ss.str();
      
      int rank = -1;
      
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