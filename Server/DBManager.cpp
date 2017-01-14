//
// DBManager.cpp
//
//

#include <sqlite3.h>
#include <iostream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>

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