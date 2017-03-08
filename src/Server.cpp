#include "../include/Server.h"
#include "../include/global.h"
#include "../include/logger.h"

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <unistd.h>
#include <cstring>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <sstream>
#include <assert.h>
#include <ifaddrs.h>
#include <algorithm>
#include <iomanip>

using namespace std;

//test functions NEED deleted
void Server::testSortVector(){

    LoggedInListItemServer itemTemp = LoggedInListItemServer("euston", "34", 5701);
    loggedInList.push_back(itemTemp);
    itemTemp = LoggedInListItemServer("embankment", "35", 5000);
    loggedInList.push_back(itemTemp);
    itemTemp = LoggedInListItemServer("stones", "46", 4545);
    loggedInList.push_back(itemTemp);
    itemTemp = LoggedInListItemServer("highgate", "33", 5499);
    loggedInList.push_back(itemTemp);

    printLoggedInList();

    sort(loggedInList.begin(), loggedInList.end());
    cout << "After Sorted:>>" << endl;
    printLoggedInList();

    rmLoggedInList("35");
    cout << "After remove 35:>>" << endl;
    printLoggedInList();

}

int Server::addLoggedInList(LoggedInListItemServer item){
  //need to deal with the client which is not new but logged-out
  // return 1 : add
  // return 2 : update

  int listSize = loggedInList.size();
  for (int i = 0; i < listSize; i++) {
    if (loggedInList.at(i).hostname == item.hostname) {
      loggedInList.at(i).status = "logged-in";
      return 2;
    }
  }

  loggedInList.push_back(item);
  sort(loggedInList.begin(), loggedInList.end());
  return 1;
}
int Server::rmLoggedInList(string ip){
  //之后实现的时候需要注意，一个 ip 可能有多个端口的客户端，所以参数需要接受两个：ip 和 port
  for (size_t i = 0; i < loggedInList.size(); i++) {
    LoggedInListItemServer t = loggedInList.at(i);
    if (t.ip_addr == ip) {
      loggedInList.erase(loggedInList.begin() + i);
      return 1;
    }
  }
  return 0;
}
int Server::addBlockList(string blockingIP, string blockedIP){
    size_t i = 0;
    for (i = 0; i < loggedInList.size(); i++) {
        LoggedInListItemServer lilis = loggedInList.at(i);
        if (lilis.ip_addr == blockingIP) {
            vector<string> blist = lilis.blockedList;
            int j = 0;
            for (j = 0; j < blist.size(); ++j) {
                if(blist.at(j) == blockedIP){
                    break;
                }
            }
            if (j == blist.size()){ //没有再次 block，添加新的
                loggedInList.at(i).blockedList.push_back(blockedIP);
            }
            break;
        }
    }
    if (i == loggedInList.size()){
        return -1; //means that blockingIP is not existed in loggedInList
    }
    return 1;
}
int Server::rmBlockList(string unblockingIP, string unblockedIP){
    size_t i = 0;
    for (i = 0; i < loggedInList.size(); i++) {
        LoggedInListItemServer lilis = loggedInList.at(i);
        if (lilis.ip_addr == unblockingIP) {
            vector<string> blist = lilis.blockedList;
            int j = 0;
            for (j = 0; j < blist.size(); ++j) {
                if(blist.at(j) == unblockedIP){
                    loggedInList.at(i).blockedList.erase(loggedInList.at(i).blockedList.begin() + i);
                    break;
                }
            }
            break;
        }
    }
    if (i == loggedInList.size()){
        return -1; //means that blockingIP is not existed in loggedInList
    }
    return 1;
}
int Server::isClientBeenBlocked(string blockedBy, string beBlocked){
    // -1 means no such blockedBy IP in the loggedInList
    // 0 means not be blocked
    // 1 means be blocked
    int ret = -100;
    int i = 0;
    for (i = 0; i < loggedInList.size(); ++i) {
        if(loggedInList.at(i).ip_addr == blockedBy){
            int j = 0;
            for (j = 0; j < loggedInList.at(i).blockedList.size(); ++j) {
                if(loggedInList.at(i).blockedList.at(j) == beBlocked){
                    return 1;
                }
            }
            return 0;
        }
    }
    return -1;
}
int Server::isClientLoggedIn(string clientIP){
    // -1 means clientIP doesn't exist
    // 0 means logged-out
    // 1 means logged-in
    int ret = -100;
    int i =0;
    for (i = 0; i < loggedInList.size(); ++i) {
        if(loggedInList.at(i).ip_addr == clientIP){
            if(loggedInList.at(i).status == "logged-in"){
                ret = 1;
            }else if(loggedInList.at(i).status == "logged-out"){
                ret = 0;
            }
            break;
        }
    }
    if(i == loggedInList.size()){
        ret = -1;
    }

    return ret;
}
int Server::findIndexOfIpInLIST(string clientIP){
    // -1 means no such IP exists in LIST
    // >= 0 the index
    int ret = -1;
    for (int i = 0; i < loggedInList.size(); ++i) {
        if(loggedInList.at(i).ip_addr == clientIP){
            ret = i;
            break;
        }
    }
    return ret;

}
string Server::findClientIPfromSocket(int clientSocket){
    struct sockaddr_in c;
    socklen_t cLen = sizeof(c);
    getpeername(clientSocket, (struct sockaddr*) &c, &cLen);
    string result =  inet_ntoa(c.sin_addr);
    return result;
}




int Server::start(){
	string _portStr = selfPort;

	cout << "This is Server process with port : " << _portStr << endl;
  cout << "test begin***************************************"<<endl;
  testSortVector();
  cout << "test end*****************************************"<<endl;


	int port,
			server_socket,
			head_socket,
			selret,
			sock_index,
			fdaccept = 0,
			caddr_len;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;

	fd_set master_list;
	fd_set watch_list;

	//socket
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0) {
		std::cerr << "XueError: Cannot create socket" << '\n';
	}

	//fill up sockaddr_in struct
	port = atoi(_portStr.c_str());
	bzero(&server_addr,sizeof(server_addr));

	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

	//bind

	if(bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
		std::cerr << "XueError: Bind failed" << '\n';
	}

	//listen
	if (listen(server_socket, BACKLOG) < 0) {
		std::cerr << "XueError: Unable to listen on port" << '\n';
	}

	/**************************************************************************/
	// Zero select FD sets
	FD_ZERO(&master_list);
	FD_ZERO(&watch_list);

	//Register the listening socket
	FD_SET(server_socket, &master_list);
	//Register STDIN
	FD_SET(STDIN, &master_list);

	head_socket = server_socket;

	while (TRUE) {
		memcpy(&watch_list, &master_list, sizeof(master_list));

		//select system call, BLOCK
		selret = select(head_socket+1, &watch_list, NULL, NULL, NULL);
		if (selret < 0) {
			std::cerr << "XueError: Select() failed." << '\n';
		}
		if (selret > 0) {
			//loop socket descriptors to check which one is ready
			for (sock_index=0; sock_index <= head_socket; sock_index++) {
				if (FD_ISSET(sock_index, &watch_list)) {
					//check if new command on STDIN
					if (sock_index == STDIN) {
						char *cmd = (char*) malloc(sizeof(char) * CMD_SIZE);
						memset(cmd, '\0', CMD_SIZE);
						if (fgets(cmd, CMD_SIZE-1, stdin) == NULL) {
							exit(-1);
						}
						std::cout << "I got command:" << cmd << endl;

						//todo: process PA1 commands here
						string tempCmd = string(cmd);
						parseCmd(tempCmd);

						free(cmd);
					}

					//check if new client is requesting connections, 新 client 连接
					else if (sock_index == server_socket) {
						caddr_len = sizeof(client_addr);
						fdaccept = accept(server_socket, (struct sockaddr *)&client_addr, (socklen_t*)&caddr_len);
						if (fdaccept < 0) {
							std::cerr << "XueError: Accept failed." << '\n';
						}
						std::cout << "Remote Host Connected." << '\n';

						//add to watched socket list
						FD_SET(fdaccept, &master_list);
						if (fdaccept > head_socket) {
							head_socket = fdaccept;
						}
					}

					//read from exiting Clients
					else{
						//initialize buffer to receive response
						char *buffer = (char*) malloc(sizeof(char) * BUFFER_SIZE);
						memset(buffer, '\0', BUFFER_SIZE);
						if (recv(sock_index, buffer, BUFFER_SIZE, 0) <= 0) {
							close(sock_index);
							std::cout << "Remote Host's connection is terminated." << '\n';

							//remove from watched list
							FD_CLR(sock_index, &master_list);
						}
						else{
							//process incomming data from exiting clients
							std::cout << "Client sent me: " << buffer << endl;
                            //process request sent by existing sockets
                            string requestStr = string(buffer);
//                            parseRequest(fdaccept, requestStr);
                            parseRequest(sock_index, requestStr);

						}
						free(buffer);
					}
				}
			}
		}
	}
	return 1;
}

int Server::connect_to_host(string server_ip, int server_port){
    int fdsocket;
    struct sockaddr_in remoteServerAddr;
    fdsocket = socket(AF_INET, SOCK_STREAM, 0);
    if (fdsocket < 0) {
        std::cerr << "XueError: Failed to create socket.[Client::connect_to_host]" << std::endl;
    }
    bzero(&remoteServerAddr, sizeof(remoteServerAddr));
    remoteServerAddr.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip.c_str(), &remoteServerAddr.sin_addr);
    remoteServerAddr.sin_port = htons(server_port);

    if (connect(fdsocket, (struct sockaddr *)&remoteServerAddr, (socklen_t)sizeof(remoteServerAddr))<0) {
        std::cerr << "XueError: Conect failed.[Client::connect_to_host]" << std::endl;
    }
    return fdsocket;
}

int Server::parseCmd(string cmd){
  stringstream ss(cmd);
  vector<string> tokens;
  string tkn;
  while (ss >> tkn) {
    tokens.push_back(tkn);
  }
  if (tokens.size() == 0) {
    exit(-1);
  }

	//Process cmds
	string cmder = tokens.at(0);
  int nToken = tokens.size();
	if (cmder == "AUTHOR") {
		assert(nToken == 1);
		string myUBIT = onAUTHOR();
		assert(myUBIT != "");
		printf("[%s:SUCCESS]\n",cmder.c_str());
		printf("I, %s, have read and understood the course academic integrity policy.\n", myUBIT.c_str());
		printf("[%s:END]\n", cmder.c_str());
	}
	else if (cmder == "IP") {
		assert(nToken == 1);
		string ip_addr = onIP();
		if(ip_addr != ""){
			printf("[%s:SUCCESS]\n",cmder.c_str());
			printf("IP:%s\n",ip_addr.c_str());
			printf("[%s:END]\n", cmder.c_str());
		}else{
			printf("[%s:ERROR]\n",cmder.c_str());
			printf("[%s:END]\n", cmder.c_str());
		}
	}
	else if (cmder == "PORT") {
		assert(nToken == 1);
		string port_num = onPORT();
		if(port_num != ""){
			printf("[%s:SUCCESS]\n",cmder.c_str());
			printf("IP:%s\n",port_num.c_str());
			printf("[%s:END]\n", cmder.c_str());
		}else{
			printf("[%s:ERROR]\n",cmder.c_str());
			printf("[%s:END]\n", cmder.c_str());
		}
	}

	else if (cmder == "LIST") {assert(nToken == 1); onLIST();}
	else if (cmder == "STATISTICS") {assert(nToken == 1);onSTATISTICS();}
	else if (cmder == "BLOCKED") {
        assert(nToken == 2);
        onBLOCKED(tokens.at(1));
    }
	else{std::cerr << "XueError: "<< cmder <<" | NO such commander!" << std::endl;}
}

int Server::parseRequest(int fdaccept, string requestStr){
  stringstream ss(requestStr);
  vector<string> tokens;
  string tkn;
  while (ss >> tkn) {
    tokens.push_back(tkn);
  }
  if (tokens.size() == 0) {
    exit(-1);
  }

  string rqstCmder = tokens.at(0); //the command of total request string, the first word
  int nTokens = tokens.size();

    //Process requestStr
    //***********************************************************************************
    if (rqstCmder == "LOGIN") {
        cout << "[Server::parseRequest] : Request is LOGIN, Processing..." << endl;
        //add to list
        assert(nTokens == 4);

        LoggedInListItemServer item = LoggedInListItemServer(tokens.at(1), tokens.at(2), atoi(tokens.at(3).c_str()));
        //需要查询一下是否已经有对应的项，re-login会出现的情况
        item.status = "logged-in";
        item.socketNumber = fdaccept;

        addLoggedInList(item);
        printLoggedInList();

        vector<LoggedInListItemClient> tlist = getListForClient();


        string strsend;
        for (int i = 0; i < tlist.size(); ++i) {
            stringstream ss;
            ss << tlist.at(i).port_num;
            string strport_num = ss.str();

            strsend = strsend
                      + " " + tlist.at(i).hostname
                      + " " + tlist.at(i).ip_addr
                      + " " + strport_num;
        }
        sendMsgtoSocket(fdaccept,strsend);
        cout << "just sent : "<<strsend<<endl;

        cout << "just receive: " << recvMsgfromSocket(fdaccept) <<endl; // == "ACK");


        string strRcv;

        int ind = findIndexOfIpInLIST(findClientIPfromSocket(fdaccept));
        if(ind >= 0){
            for (int j = 0; j < loggedInList.at(ind).bufferdMessages.size(); ++j) {
                string t = "from:";
                t = t + loggedInList.at(ind).bufferdMessages.at(j).first
                        +" " + loggedInList.at(ind).bufferdMessages.at(j).second;
                sendMsgtoSocket(fdaccept, t);
                recvMsgfromSocket(fdaccept);  //wait "ACK"

            }
            sendMsgtoSocket(fdaccept,"END");
        }


/*        for (int j = 0; j < 2; ++j) {
            string t = "This is the message that I send to test if being recognized well!";
            sendMsgtoSocket(fdaccept,t);
            cout << "just sent : "<< t <<endl;

            cout << "just receive: " << recvMsgfromSocket(fdaccept) <<endl; // == "ACK");
        }*/

      //***********************************************************************************
    }else if(rqstCmder == "LOGOUT"){
        cout << "start parsing request and compare it to LOGOUT, nTokens is(should be 2): " << nTokens<< endl;
        assert(nTokens == 2);

        //set list of current client to LOGOUT
        string clntIP = tokens.at(1);
        for (size_t i = 0; i < loggedInList.size(); i++) {
        if (loggedInList.at(i).ip_addr == clntIP) {
            loggedInList.at(i).status = "logged-out";
            loggedInList.at(i).socketNumber = -1;
        }
        }
        printLoggedInList();

        //***********************************************************************************
    }else if(rqstCmder == "REFRESH"){
        assert(nTokens == 1);
        cout << "[Server::parseRequest] : Request is REFRESH, Processing..." << endl;
        vector<LoggedInListItemClient> tlist = getListForClient();
        cout << "[rqstCmder == REGRESH] getlistforClient's return size is:"<<tlist.size()<<endl;
        string strsend="";

        for (int i = 0; i < tlist.size(); ++i) {
            stringstream ss;
            ss << tlist.at(i).port_num;
            string strport_num = ss.str();

            strsend = strsend
                      + " " + tlist.at(i).hostname
                      + " " + tlist.at(i).ip_addr
                      + " " + strport_num;
        }
        sendMsgtoSocket(fdaccept,strsend);
        cout << "just sent : "<<strsend<<endl;

        cout << "just receive: " << recvMsgfromSocket(fdaccept) <<endl; // == "ACK");


        //***********************************************************************************
    }else if(rqstCmder == "SEND"){
        assert(nTokens >= 3);
        cout << "[Server::parseRequest] : Request is SEND, Processing..." << endl;
        string toClientIP = tokens.at(1);
        int toClientPort = 0;
        string fromClientIP;

        int listSize = loggedInList.size();
        int toClientSocket = -100;
        int indexOftoClientInList = -1;
        int indexOffromClientInList = -1;

        string strMsg = tokens.at(2);
        for (int i = 3; i < nTokens; ++i) {
            strMsg = strMsg + " " + tokens.at(i);
        }

        //getpeerip, get fromClientIP
        struct sockaddr_in c;
        socklen_t cLen = sizeof(c);
        getpeername(fdaccept, (struct sockaddr*) &c, &cLen);
        printf("[Send] fromClient: %s\n", inet_ntoa(c.sin_addr));
        fromClientIP =  inet_ntoa(c.sin_addr);

        //modify the fromClient's msg_send
        for (int i = 0; i < listSize; i++) {
            if (loggedInList.at(i).ip_addr == fromClientIP) {
                indexOffromClientInList = i;
            }
        }
        assert(indexOffromClientInList != -1);
        loggedInList.at(indexOffromClientInList).num_msg_sent += 1;


        //find socketNumber from list
        for (int i = 0; i < listSize; i++) {
            if (loggedInList.at(i).ip_addr == toClientIP) {
                toClientSocket = loggedInList.at(i).socketNumber;
                toClientPort = loggedInList.at(i).port_num;
                indexOftoClientInList = i;
                break;
            }
        }

        //根据 socket 来决定 action
        if(toClientSocket > 0){
            if (isClientBeenBlocked(toClientIP, fromClientIP) == 0){
                //cout << "Here should send strMsg to toClientIP" << endl;
                //sendMsgtoSocket(toClientSocket, strMsg);
                assert(toClientPort != 0);
                int newSocketToClient = connect_to_host(toClientIP, toClientPort);
                sendMsgtoSocket(newSocketToClient, strMsg);
                loggedInList.at(indexOftoClientInList).num_msg_rcv += 1;
                close(newSocketToClient);
            }

        }
        if(toClientSocket == -1){

            cout << "Here should buffer this strMsg for toClientIP, and store it in the loggedInList" <<endl;
            assert(indexOftoClientInList != -1);
            pair<string, string> tbuf = make_pair(fromClientIP, strMsg);
            loggedInList.at(indexOftoClientInList).bufferdMessages.push_back(tbuf);
        }

        printLoggedInList();

        //**************************************************************************************************
    }else if(rqstCmder == "BLOCK"){

        cout << "start parsing request and compare it to BLOCK, nTokens is(should be 2): " << nTokens<< endl;
        assert(nTokens == 2);

        string blockingIP;
        string blockedIP;

        //get blocking IP an blockedIP
        struct sockaddr_in c;
        socklen_t cLen = sizeof(c);
        getpeername(fdaccept, (struct sockaddr*) &c, &cLen);
        printf("[Send] fromClient: %s\n", inet_ntoa(c.sin_addr));
        blockingIP =  inet_ntoa(c.sin_addr);
        blockedIP = tokens.at(1);
        addBlockList(blockingIP, blockedIP);

        //**************************************************************************************************
    }else if(rqstCmder == "UNBLOCK"){
        cout << "start parsing request and compare it to UNBLOCK, nTokens is(should be 2): " << nTokens<< endl;
        assert(nTokens == 2);

        string unblockingIP;
        string unblockedIP;

        //get blocking IP an blockedIP
        struct sockaddr_in c;
        socklen_t cLen = sizeof(c);
        getpeername(fdaccept, (struct sockaddr*) &c, &cLen);
        printf("[Send] fromClient: %s\n", inet_ntoa(c.sin_addr));
        unblockingIP =  inet_ntoa(c.sin_addr);
        unblockedIP = tokens.at(1);
        rmBlockList(unblockingIP, unblockedIP);

        //**************************************************************************************************
    }else if (rqstCmder == "BROADCAST"){
        assert(nTokens >= 2);
        string strBroadcast = tokens.at(1);
        for (int i = 2; i < nTokens; ++i) {
            strBroadcast = strBroadcast + " " + tokens.at(i);
        }
        inBROADCAST(fdaccept,strBroadcast);

        //**************************************************************************************************
    } else if(rqstCmder == "EXIT"){
        assert(nTokens == 1);
        inEXIT(fdaccept);
    }
    else{
        cout << "[Server::parseRequest] : Request is not LOGIN or LOGOUT or REFRESH or SEND or BLOCK or Broadcast or exit" << endl;
    }
    return 1;

}

int Server::printLoggedInList(){
  for (size_t i = 0; i < loggedInList.size(); i++) {
    LoggedInListItemServer t = loggedInList.at(i);
    cout << setw(2) <<i
        << " : " <<setw(30)<< t.hostname
        << " : " <<setw(15)<< t.ip_addr
        << " : " <<setw(5)<< t.port_num
        << " : " <<setw(3)<< t.num_msg_sent
        << " : " <<setw(3)<< t.num_msg_rcv
        << " : " <<setw(3)<< t.status
        << " : " <<setw(5)<< t.socketNumber
        <<endl;
  }
}

vector<LoggedInListItemClient> Server::getListForClient(){
    vector<LoggedInListItemClient> resultList;

    for (size_t i = 0; i < loggedInList.size(); i++) {
        if(loggedInList.at(i).status != "logged-in"){
            continue;
        }

        LoggedInListItemServer ts = loggedInList.at(i);
        LoggedInListItemClient tc = LoggedInListItemClient(ts.hostname, ts.ip_addr, ts.port_num);
        resultList.push_back(tc);
    }
    return resultList;
}


string Server::getMyHostName(){
	char *msg = (char*)malloc(MSG_SIZE);
	gethostname(msg, MSG_SIZE);
  string ret = string(msg);
  free(msg);
	return ret;
}


int Server::sendMsgtoSocket(int _socket, string msg){
	std::cout << "SENDing it to the remote server...";
	if (send(_socket, msg.c_str(), msg.length(), 0) == msg.length()) {
		std::cout << "Done!" << std::endl;
	}
	return 1;
}
string Server::recvMsgfromSocket(int _socket){
  //initialize buffer to receive response
	char *buffer = (char*) malloc(sizeof(char) * BUFFER_SIZE);
	memset(buffer, '\0', BUFFER_SIZE);
	string ret = "";
	if (recv(_socket, buffer, BUFFER_SIZE, 0) >= 0) {
    ret = string(buffer);
	}
  free(buffer);
  return ret;
}

string Server::onAUTHOR(){
	string myUBIT = "zhenggan";
	return myUBIT;
	// printf("I, %s, have read and understood the course academic integrity policy.\n", myUBIT.c_str());
}
string Server::onIP(){
	struct ifaddrs *ifaddr, *ifa;
	struct sockaddr_in *sa;
	char *ip_addr;

	if(getifaddrs (&ifaddr) == -1){
		std::cerr << "XueError: getifaddrs == -1 [Client:onIP]" << std::endl;
		return "";
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL){
			continue;
		}
		if (ifa->ifa_addr->sa_family==AF_INET && strncmp(ifa->ifa_name,"eth0",BUFFER_SIZE)==0) {
			sa = (struct sockaddr_in *) ifa->ifa_addr;
			ip_addr = inet_ntoa(sa->sin_addr);
		}
	}

    freeifaddrs(ifaddr);
    return string(ip_addr);
}
string Server::onPORT(){
	return selfPort;
}

string Server::onLIST(){

    vector<LoggedInListItemClient> resultList = getListForClient();

    printf("[LIST:SUCCESS]\n");
    for (int j = 0; j < resultList.size(); ++j) {
        LoggedInListItemClient t = resultList.at(j);
        cout << setw(2) <<j+1
             << " : " <<setw(30)<< t.hostname
             << " : " <<setw(15)<< t.ip_addr
             << " : " <<setw(5)<< t.port_num
             <<endl;
    }
    printf("[LIST:END]\n");
    return "list";
}
string Server::onSTATISTICS(){
    vector<LoggedInListItemServer> resultList = loggedInList;

    printf("[LIST:SUCCESS]\n");
    for (int j = 0; j < resultList.size(); ++j) {
        LoggedInListItemServer t = resultList.at(j);
        cout << setw(2) <<j+1
             << " : " <<setw(30)<< t.hostname
             << " : " <<setw(5)<< t.num_msg_sent
             << " : " <<setw(5)<< t.num_msg_rcv
             << " : " <<setw(10)<< t.status
             <<endl;
    }
    printf("[LIST:END]\n");
    return "statistics";

}
string Server::onBLOCKED(string _clientIP){

    vector<string> blockedlist;
    int i = 0;
    for (i = 0; i < loggedInList.size(); ++i) {
        LoggedInListItemServer lllis = loggedInList.at(i);
        if(lllis.ip_addr == _clientIP){
            blockedlist = lllis.blockedList;
            break;
        }
    }

    if(i == loggedInList.size()){
        cerr << "[blocked] there is no match record given the clientIP." <<endl;
        return "NoMatch";
    }

    printf("[BLOCKED:SUCCESS]\n");
    for (int j = 0; j < blockedlist.size(); ++j) {
        cout << blockedlist.at(j) << endl;
    }
    printf("[BLOCKED:END]\n");

    return "blocked";
}

string Server::inBROADCAST(int fromClientSocket, string msgBroadcast){

    string fromClientIP;
    string toClientIP;

    //get fromClientIP
    fromClientIP = findClientIPfromSocket(fromClientSocket);
    cout << "[inBROADCAST] fromClientIP is:"<<fromClientIP <<"fromClientSocket:"<<fromClientSocket<<endl;

    //给 fromClient 的 msg sent number +1
    int indexFrom = findIndexOfIpInLIST(fromClientIP);
    cout << "[inBROADCAST] index of fromClientIP is:"<<indexFrom<<endl;


    assert(indexFrom >= 0);
    loggedInList.at(indexFrom).num_msg_sent += 1;


    //对 loggedInList 里面每一个client 遍历处理, 不包括自己
    for (int i = 0; i < loggedInList.size(); ++i) {
        toClientIP = loggedInList.at(i).ip_addr;

        //跳过自己
        if(toClientIP == fromClientIP){
            continue;
        }

        if(loggedInList.at(i).status == "logged-in"){
            if (isClientBeenBlocked(toClientIP, fromClientIP) == 0){
                int newSocketToClient = connect_to_host(toClientIP, loggedInList.at(i).port_num);
                sendMsgtoSocket(newSocketToClient, msgBroadcast);
                loggedInList.at(i).num_msg_rcv += 1;
                close(newSocketToClient);
            }else{
                cout << "[broadcast] the client has been blocked!";  //ignored if blocked
            }

        }else{ //if logged-out, buffered the messages
            pair<string, string> tbuf = make_pair(fromClientIP, msgBroadcast);
            loggedInList.at(i).bufferdMessages.push_back(tbuf);
        }
    }
    return "broadcast";
}

string Server::inEXIT(int fromClientSocket){
    string fromClientIP = findClientIPfromSocket(fromClientSocket);
    int index = findIndexOfIpInLIST(fromClientIP);
    if(index >= 0){
        loggedInList.erase(loggedInList.begin()+index);
    }
    return "exit";
}
