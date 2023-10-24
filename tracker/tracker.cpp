#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unordered_map>
#include <vector>
#include <unordered_set>

#define bufflen 512

using namespace std;

unordered_map<string, unordered_map<string, vector<int>>> mapper;
unordered_map<string, string> usermap;
unordered_map<int, string> currusers;
unordered_map<int, unordered_set<string>> curtogrp;
vector<int> ports;

void error(const char* msg) {
    perror(msg);
    exit(1);
}

int cctoint(const char* buffer){
    int mult = 1;
    int length =0;
    for(int i=0; i<strlen(buffer); i++){
        length += (buffer[i]-'0')*mult;
        mult*=10;
    }
    return length;
}

string inttocc(int length){
    string lengths = "";
    int templen = length;
    int digit;
    while(templen>0){
        digit = templen%10;
        lengths+=(char)(digit+'0');
        templen =templen/10;
    }
    return lengths;
}

void* server(void * arg){
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[bufflen];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    // Create a socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket");
    }

    // Initialize socket structure
    bzero((char*)&serv_addr, sizeof(serv_addr));
    portno = *((int*)(&arg));  // Choose your desired port number

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    
    // Bind the socket to a specific address and port
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        error("Error on binding");
    }
    
    // Listen for incoming connections
    listen(sockfd, 10);
    clilen = sizeof(cli_addr);

    while(true){
        // Accept a connection from a client
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if (newsockfd < 0) {
            error("Error on accept");
        }


        // receiving a message
        bzero(buffer, bufflen);

        n = read(newsockfd, buffer, bufflen);
        if (n < 0) {
            error("Error reading from socket");
        }
        
        char* part;
        part = strtok(buffer," ");
        int recportno;
        string gid;
        string s = part;
        if(s == "ts"){
            part = strtok(NULL, " ");
            s = part;
            recportno = cctoint(part);
            part = strtok(NULL, " ");
            gid = part;
            vector<int> temp;
            if(currusers.find(recportno)==currusers.end()){
            }
            else{
                if(curtogrp[recportno].find(gid)==curtogrp[recportno].end() && mapper.find(gid)==mapper.end()){
                    cout<<"Group does not exist"<<endl;
                }
                else if(curtogrp[recportno].find(gid)!=curtogrp[recportno].end() && mapper.find(gid)==mapper.end()){
                    cout<<"Group does not exist"<<endl;
                }
                else if(curtogrp[recportno].find(gid)==curtogrp[recportno].end() && mapper.find(gid)!=mapper.end()){
                    cout<<"Peer not in group"<<endl;
                }
                else if(curtogrp[recportno].find(gid)!=curtogrp[recportno].end() && mapper.find(gid)!=mapper.end()){
                    part = strtok(NULL, " ");
                    s=part;
                    if(mapper[gid].find(s)==mapper[gid].end()){
                        temp = {};
                        temp.push_back(recportno);
                        mapper[gid][s]= temp;
                    }
                    else{
                        mapper[gid][s].push_back(recportno);
                    }
                    cout<<"Successfully Uploaded"<<endl;
                }
            }
            
        }
        else if(s == "tc"){
            part = strtok(NULL, " ");
            int recportno = cctoint(part);
            part = strtok(NULL, " ");
            gid = part;
            part = strtok(NULL, " ");
            s = part;
            if(currusers.find(recportno)==currusers.end()){
                n = write(newsockfd, "Not logged in", bufflen);
                if (n < 0) {
                    error("Error writing to socket");
                }
            }
            else{
                if(curtogrp[recportno].find(gid)!=curtogrp[recportno].end() && mapper.find(gid)==mapper.end()){
                    cout<<"Group does not exist"<<endl;
                }
                else if(curtogrp[recportno].find(gid)==curtogrp[recportno].end() && mapper.find(gid)!=mapper.end()){
                    cout<<"Peer not in group"<<endl;
                }
                else if(curtogrp[recportno].find(gid)!=curtogrp[recportno].end() && mapper.find(gid)!=mapper.end()){
                    if(mapper[gid].find(s)==mapper[gid].end()){
                        cout<<"File not found"<<endl;
                    }
                    else{
                        string temps = "";
                        for(int i=0; i<mapper[gid][s].size(); i++){
                            temps+= inttocc(mapper[gid][s][i])+" ";
                        }
                        n = write(newsockfd, temps.c_str(), bufflen);
                        if (n < 0) {
                            error("Error writing to socket");
                        }
                    }
                }
            }
        }
        else if(s == "cu"){
            part = strtok(NULL, " ");
            string username = part;
            if(usermap.find(username)==usermap.end()){
                usermap[username] = strtok(NULL, " ");
                n = write(newsockfd, "User created Successfully", bufflen);
                if (n < 0) {
                    error("Error writing to socket");
                }
            }
            else{
                n = write(newsockfd, "Username already exists", bufflen);
                if (n < 0) {
                    error("Error writing to socket");
                }
            }
        }
        else if(s == "l"){
            part  = strtok(NULL, " ");
            int usrport = cctoint(part);
            part = strtok(NULL, " ");
            string username = part;
            if(usermap.find(username)==usermap.end()){
                n = write(newsockfd, "Username doesnt exist", bufflen);
                if (n < 0) {
                    error("Error writing to socket");
                }
            }
            else{
                part = strtok(NULL, " ");
                string password = part;
                if(password == usermap[username]){
                    if(currusers.find(usrport)==currusers.end()){
                        currusers[usrport] = username;
                        if(currusers.find(usrport)==currusers.end()) cout<<"hello"<<endl;
                        else cout<<"hi"<<endl;
                        n = write(newsockfd, "Successfully Authenticated", bufflen);
                        if (n < 0) {
                            error("Error writing to socket");
                        }
                    }
                    else{
                        n = write(newsockfd, "User already logged in", bufflen);
                        if (n < 0) {
                            error("Error writing to socket");
                        }
                    }
                }
                else{
                    n = write(newsockfd, "Incorrect password", bufflen);
                    if (n < 0) {
                        error("Error writing to socket");
                    }
                }
            }
        }
        else if(s == "cg"){
            part = strtok(NULL, " ");
            int usrport = cctoint(part);
            vector<int> temp;
            unordered_map<string, vector<int>> submap;
            part = strtok(NULL, " ");
            s = part;
            if(currusers.find(usrport)==currusers.end()){
                n = write(newsockfd, "Not logged in", bufflen);
                if (n < 0) {
                    error("Error writing to socket");
                }
            }
            else{
                if(mapper.find(s)==mapper.end()){
                    submap.clear();
                    mapper[s]=submap;
                    n = write(newsockfd, "Group created successfully", bufflen);
                    if (n < 0) {
                        error("Error writing to socket");
                    }
                }
                else{
                    n = write(newsockfd, "Group already exists", bufflen);
                    if (n < 0) {
                        error("Error writing to socket");
                    }
                }
            }
        }
        else if(s == "jg"){
            part = strtok(NULL, " ");
            int usrport = cctoint(part);
            part = strtok(NULL, " ");
            string grid  = part;
            if(currusers.find(usrport)==currusers.end()){
                n = write(newsockfd, "Not logged in", bufflen);
                if (n < 0) {
                    error("Error writing to socket");
                }
                continue;
            }
            else{
                    if(mapper.find(grid)==mapper.end()){
                    n = write(newsockfd, "Group does not exist", bufflen);
                    if (n < 0) {
                        error("Error writing to socket");
                    }
                }
                else if(curtogrp.find(usrport)==curtogrp.end()){
                    unordered_set<string> sset;
                    sset.insert(grid);
                    curtogrp[usrport]=sset;
                    n = write(newsockfd, "Successfully joined group", bufflen);
                    if (n < 0) {
                        error("Error writing to socket");
                    }
                }
                else{
                    if(curtogrp[usrport].find(grid)==curtogrp[usrport].end()){
                        curtogrp[usrport].insert(grid);
                        n = write(newsockfd, "Successfully joined group", bufflen);
                        if (n < 0) {
                            error("Error writing to socket");
                        }
                    }
                    else{
                        n = write(newsockfd, "Already joined group", bufflen);
                        if (n < 0) {
                            error("Error writing to socket");
                        }
                    }
                }
            }
        }
        else if(s == "lg"){
            part = strtok(NULL, " ");
            int usrport = cctoint(part);
            part = strtok(NULL, " ");
            string grid  = part;
            if(currusers.find(usrport)==currusers.end()){
                n = write(newsockfd, "Not logged in", bufflen);
                if (n < 0) {
                    error("Error writing to socket");
                }
            }
            else{
                    if(mapper.find(grid)==mapper.end()){
                    n = write(newsockfd, "Group does not exist", bufflen);
                    if (n < 0) {
                        error("Error writing to socket");
                    }
                }
                else if(curtogrp.find(usrport)==curtogrp.end()){
                    n = write(newsockfd, "Not in group", bufflen);
                    if (n < 0) {
                        error("Error writing to socket");
                    }
                }
                else{
                    if(curtogrp[usrport].find(grid)==curtogrp[usrport].end()){
                        n = write(newsockfd, "Not in group", bufflen);
                        if (n < 0) {
                            error("Error writing to socket");
                        }
                    }
                    else{
                        curtogrp[usrport].erase(grid);
                        n = write(newsockfd, "Left group successfully", bufflen);
                        if (n < 0) {
                            error("Error writing to socket");
                        }
                    }
                }
            }
        }
        else if(s == "listr"){
            
        }
        else if(s == "ar"){
            
        }
        else if(s == "listg"){
            string s = "";
            for(auto it: mapper){
                s+=it.first+"\n";
            }
            n = write(newsockfd, s.c_str(), bufflen);
            if (n < 0) {
                error("Error writing to socket");
            }
        }
        else if(s == "lf"){
            part = strtok(NULL, " ");
            string grid  = part;
            string s = "";
            for(auto it: mapper[grid]){
                s+=it.first+"\n";
            }
            n = write(newsockfd, s.c_str(), bufflen);
            if (n < 0) {
                error("Error writing to socket");
            }
        }
        else if(s == "lo"){
            part = strtok(NULL, " ");
            int usrport = cctoint(part);
            if(currusers.find(usrport)==currusers.end()){
                n = write(newsockfd, "Not logged in", bufflen);
                if (n < 0) {
                    error("Error writing to socket");
                }
            }
            else{
                currusers.erase(usrport);
                n = write(newsockfd, "Successfully logged out", bufflen);
                if (n < 0) {
                    error("Error writing to socket");
                }
            }
        }
        else if(s == "sd"){
            
        }
        else if(s == "ss"){
            
        }
    }
    // Close the sockets
    close(newsockfd);
    close(sockfd);
    return NULL;
}

int main() {
    pthread_t serverthread;
    int port1=13000;
    pthread_create(&serverthread, NULL, server , (void *)port1);
    pthread_join(serverthread, NULL);
    return 0;
}
