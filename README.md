# TouchBarDinoX-Server
Written in C++, TouchBarDinoX-Server extends the original [TouchBarDino](https://github.com/yuhuili/TouchBarDino) with a new leaderboard system.

It communicates with [TouchBarDinoX](https://github.com/yuhuili-lab/TouchBarDinoX) using socket, provided by [Boost.Asio](http://www.boost.org/doc/libs/1_63_0/doc/html/boost_asio.html). The server handles request of new high score and returns the ranking based on the data from the sqlite3 database.

## Dependencies
- [Boost.Asio](http://www.boost.org/doc/libs/1_63_0/doc/html/boost_asio.html)
  - On Mac, using Homebrew
  `brew install boost`

- [SimpleJSON](https://github.com/MJPA/SimpleJSON)
  - Already included in this package

## Note
- The server currently trusts all incoming data and will update the database if it sees a valid json. Because of this, no test server is provided to prevent abuse.
- The compiled .out version is for MacOS Sierra.
- Player ID in database is based on the Mac serial number.

## TouchBarDinoX
Please see [TouchBarDinoX repo](https://github.com/yuhuili-lab/TouchBarDinoX) for more information on TouchBarDinoX.
