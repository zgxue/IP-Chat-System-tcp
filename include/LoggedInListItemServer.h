#ifndef LOGGEDINLISTITEMSERVER_H_
#define LOGGEDINLISTITEMSERVER_H_

//This is the Server version of LoggedInList

#include <string>
#include <vector>
using namespace std;

class LoggedInListItemServer{
public:

    string hostname;
    string ip_addr;
    int port_num;
    //below is the Server excluded
    int num_msg_sent;
    int num_msg_rcv;
    string status;  //"logged-in" or "logged-out"
    int socketNumber; // -1: logged-out
    vector<string> blockedList;
    vector<pair<string,string> > bufferdMessages; // <from ip> <message>


  LoggedInListItemServer(string _hostname, string _ip, int _port)
    :hostname(_hostname),
    ip_addr(_ip),
    port_num(_port),
    num_msg_sent(0),
    num_msg_rcv(0),
    status("NeedInit"),
    socketNumber(-1)
    {}

  bool operator < (const LoggedInListItemServer &m)const{
    return port_num < m.port_num;
  }
};

#endif
