#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <vector>

#define bufflen 512

using namespace std;

struct portndir{
    int portno;
    string dir;
};

struct filenpos{
    string file;
    int pos;
};

string ipadd;
int trackerport;
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
    if(templen==0) return "0";
    while(templen>0){
        digit = templen%10;
        lengths+=(char)(digit+'0');
        templen =templen/10;
    }
    return lengths;
}

void* server(void * arg){ // the order number is sent after the filename; everything related to posi is in intocc
    //send the file chunks according to the order number given by the client
    int sockfd, newsockfd, portno;
    socklen_t clilen;
    char buffer[bufflen];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket");
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    portno = *((int*)(&arg));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    
    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        error("Error on binding");
    }

    listen(sockfd, 5);
    clilen = sizeof(cli_addr);

    while(true){
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
        int file;
        string filename = buffer;
        bzero(buffer, bufflen);
        n = read(newsockfd, buffer, bufflen);
        if (n < 0) {
            error("Error reading from socket");
        }
        int orderno = cctoint(buffer);
        bzero(buffer, bufflen);
        n = read(newsockfd, buffer, bufflen);
        if (n < 0) {
            error("Error reading from socket");
        }
        int totalseeders = cctoint(buffer);
        string filepath = to_string(portno)+"/"+filename;
        file = open(filepath.c_str(), O_RDONLY);
        if(file == -1){
            cerr<<"ERROR: File not found."<<endl;
            exit(1);
        }
        int length = lseek(file, 0, SEEK_END);
        int partlength = length/totalseeders;
        int rem = length%partlength;
        if(rem!=0){
            partlength++;
            rem = length%partlength;
        }
        else rem = partlength;
        int myposition= orderno*partlength;        
        lseek(file, 0 ,SEEK_SET );
        long long i =0;
        if(orderno<totalseeders-1){
            n = write(newsockfd, inttocc(partlength).c_str(), bufflen);
            if (n < 0) {
                error("Error writing to socket");
            }
        }
        else if(orderno==totalseeders-1){
            n = write(newsockfd, inttocc(rem).c_str(), bufflen);
            if (n < 0) {
                error("Error writing to socket");
            }
        }

        n = write(newsockfd, inttocc(myposition).c_str(), bufflen);
        if (n < 0) {
            error("Error writing to socket");
        }

        if(orderno<totalseeders-1){
            while(i<partlength-bufflen){
                lseek(file, 0 , SEEK_SET);
                lseek(file, i+myposition ,SEEK_SET);
                n=read(file, buffer, bufflen);
                i+=bufflen;
                write(newsockfd, buffer, sizeof(buffer));
            }
            lseek(file, 0 ,SEEK_SET);
            lseek(file, i+myposition,SEEK_SET);
            char buff1[partlength-i];
            n=read(file, buff1, partlength-i);
            write(newsockfd, buff1, bufflen);
        }
        else if(orderno==totalseeders-1){
            while(i<rem-bufflen){
                lseek(file, 0 , SEEK_SET);
                lseek(file, i+myposition ,SEEK_SET);
                n=read(file, buffer, bufflen);
                i+=bufflen;
                write(newsockfd, buffer, sizeof(buffer));
            }
            lseek(file, 0 ,SEEK_SET);
            lseek(file, i+myposition,SEEK_SET);
            char buff1[rem-i];
            n=read(file, buff1, rem-i);
            write(newsockfd, buff1, bufflen);
        }
    }
    

    // Close the sockets
    close(newsockfd);
    close(sockfd);
    return NULL;
}

void* cclient(void* arg){
    struct filenpos *args = (struct filenpos *) arg;
    string filedir = args->file;
    string filerq = strtok((char *) filedir.c_str(), " ");
    string dir = strtok(NULL, " ");
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    char buffer[bufflen];
    int n;


    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket");
    }

    bzero((char*)&serv_addr, sizeof(serv_addr));
    portno = ports[args->pos];
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);

    if (inet_pton(AF_INET, ipadd.c_str(), &serv_addr.sin_addr) <= 0) {
        error("Invalid address");
    }

    while (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        // error("Error connecting to server");
    }
    n = write(sockfd, filerq.c_str(), bufflen);
    if (n < 0) {
        error("Error writing to socket");
    }


    n = write(sockfd, inttocc(args->pos).c_str(), bufflen);
    if (n < 0) {
        error("Error writing to socket");
    }

    n = write(sockfd, inttocc(ports.size()).c_str(), bufflen);
    if (n < 0) {
        error("Error writing to socket");
    }
    int file;
	string ofilepath = dir+"/" + filerq;
	struct stat statbuffer;
    if(stat(dir.c_str(), &statbuffer)!=0){
		int err = mkdir(dir.c_str(),0700);
		if(err==-1){
			cerr<<"ERROR: Directory could not be created."<<endl;
			exit(1);
		}
	}
	file = open(ofilepath.c_str(), O_WRONLY| O_CREAT, 0700);
	if(file == -1){
		cerr<<"ERROR: File could not be created."<<endl;
		exit(1);
	}
	truncate(ofilepath.c_str(), 0);
    bzero(buffer, bufflen);
    int length=0;
    n = read(sockfd, buffer, bufflen);
    if (n < 0) {
        error("Error reading from socket");
    }
    length = cctoint(buffer);
    bzero(buffer, bufflen);
    int posi;
    n = read(sockfd, buffer, bufflen);
    if (n < 0) {
        error("Error reading from socket");
    }
    posi = cctoint(buffer);

    long long i =0;
    lseek(file, 0 ,SEEK_SET );
	while(i<length-bufflen){
        lseek(file, 0 ,SEEK_SET );
		bzero(buffer, bufflen);
        n = read(sockfd, buffer, bufflen);
        lseek(file, i+posi ,SEEK_SET );
		write(file, buffer, bufflen);
        i+=bufflen;
    }
    bzero(buffer, bufflen);
    n = read(sockfd, buffer, bufflen);
    if (n < 0) {
        error("Error reading from socket ");
    }
    lseek(file, 0 ,SEEK_SET );
    lseek(file, i+posi ,SEEK_SET );
    write(file, buffer, length-i);
    close(sockfd);
    return NULL;
}

void* client(void * arg){ //give position of file as arugment to pthread
    struct filenpos *args = (struct filenpos *) arg;
    pthread_t threads[ports.size()];
    struct filenpos argsarr[ports.size()];
    // pthread_t athread;
    for(int i=0; i<ports.size(); i++){
        argsarr[i].file = args->file;
        argsarr[i].pos = i;
        
        // threads.push_back(athread);
        if(pthread_create(&threads[i], NULL, cclient, (void *)&argsarr[i])!=0){
            perror("pthread_create");
            exit(1);
        }
    }
    for(int i=0; i<ports.size(); i++){
        if(pthread_join(threads[i], NULL)!=0){
            perror("pthread_join");
            exit(1);
        }
    }
    return NULL;
}

void tclient(string gid, string filerq, int myport){
    string message = "tc "+ inttocc(myport) +" "+gid+ " " + filerq;
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    char buffer[bufflen];
    int n;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket");
    bzero((char*)&serv_addr, sizeof(serv_addr));
    portno = trackerport;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    
    if (inet_pton(AF_INET, ipadd.c_str(), &serv_addr.sin_addr) <= 0) {
        error("Invalid address");
    }
    while (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        // error("Error connecting to server");
    }

    n = write(sockfd, message.c_str(), bufflen);
    if (n < 0) {
        error("Error writing to socket");
    }
    
    // receiving a message
    bzero(buffer, bufflen);
    n = read(sockfd, buffer, bufflen);
    if (n < 0) {
        error("Error reading from socket in tclient");
    }
    char* part;
    part = strtok(buffer," ");
    ports.push_back(cctoint(part));
    while((part = strtok(NULL, " "))!=NULL){
        ports.push_back(cctoint(part));
    }
    close(sockfd);
}

void tserver(int myport, string sdir, string gid){
    string temps;
    string message = "ts ";
    temps = inttocc(myport);
    message+= temps+ " ";
    message+= gid+ " ";
    message+= sdir;
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    char buffer[bufflen];
    int n;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket");
    bzero((char*)&serv_addr, sizeof(serv_addr));
    portno = trackerport;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    
    if (inet_pton(AF_INET, ipadd.c_str(), &serv_addr.sin_addr) <= 0) {
        error("Invalid address");
    }
    while (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        // error("Error connecting to server");
    }
    n = write(sockfd, message.c_str(), bufflen);
    if (n < 0) {
        error("Error writing to socket");
    }
    
    close(sockfd);
}

void sendtotracker(string message){
    int sockfd, portno;
    struct sockaddr_in serv_addr;
    char buffer[bufflen];
    int n;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) error("Error opening socket");
    bzero((char*)&serv_addr, sizeof(serv_addr));
    portno = trackerport;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portno);
    
    if (inet_pton(AF_INET, ipadd.c_str(), &serv_addr.sin_addr) <= 0) {
        error("Invalid address");
    }
    while (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        // error("Error connecting to server");
    }

    n = write(sockfd, message.c_str(), bufflen);
    if (n < 0) {
        error("Error writing to socket");
    }
    
    // receiving a message
    bzero(buffer, bufflen);
    n = read(sockfd, buffer, bufflen);
    if (n < 0) {
        error("Error reading from socket in tclient");
    }
    cout<<buffer<<endl;
        // message = buffer;
        // char* part;
        // part = strtok(buffer," ");
        // ports.push_back(cctoint(part));
        // while((part = strtok(NULL, " "))!=NULL){
        //     ports.push_back(cctoint(part));
        // }
    close(sockfd);
}

int main(int argc, char* argv[]) {
    //todo list:
    //have to generalize peerfile for all peers
    //test chunking
    //updated code is only here
    pthread_t serverthread, clientthread, trackerclient, trackerserver;
    //generate port number;
    char* ipnport = argv[1];
    ipadd = strtok(ipnport, ":");
    string portstr = strtok(NULL, ":");
    string dir = portstr;
    struct stat statbuffer;
    if(stat(dir.c_str(), &statbuffer)!=0){
		int err = mkdir(dir.c_str(),0700);
		if(err==-1){
			cerr<<"ERROR: Directory could not be created."<<endl;
			exit(1);
		}
	}
    int portno = atoi(portstr.c_str());

    int tfile = open(argv[2], O_RDONLY);
    if(tfile == -1){
        cerr<<"ERROR: File not found"<<endl;
        exit(1);
    }
    int tl = lseek(tfile, 0, SEEK_END);
    lseek(tfile, 0 ,SEEK_SET );
    char temp[tl];
    read(tfile, temp, tl);
    portstr = strtok(temp, ":");
    portstr = strtok(NULL, ":");
    trackerport = atoi(portstr.c_str());

    
    struct portndir *args = (portndir*)malloc(sizeof(struct portndir));
    struct filenpos *argo = (filenpos *)malloc(sizeof(struct filenpos));
    args->portno= portno;
    args->dir = dir;
    char input[256];
    char* inp;
    pthread_create(&serverthread, NULL, server , (void *) portno);
    while(true){
        cout<<portno<<">";
        cin.getline(input, 256);
        inp = strtok(input, " ");
        if(strcmp(inp, "create_user")==0){
            string username = strtok(NULL, " ");
            string password = strtok(NULL, " ");
            string tosend = "cu "+username+" "+password;
            sendtotracker(tosend);
        }
        else if(strcmp(inp, "login")==0){
            string username = strtok(NULL, " ");
            string password = strtok(NULL, " ");
            string tosend = "l "+ inttocc(portno)+" " +username+ " "+password;
            sendtotracker(tosend);
        }
        else if(strcmp(inp, "create_group")==0){
            string groupid = strtok(NULL, " ");
            string tosend = "cg "+ inttocc(portno)+" " + groupid;
            sendtotracker(tosend);
        }
        else if(strcmp(inp, "join_group")==0){
            string groupid = strtok(NULL, " ");
            string tosend = "jg "+ inttocc(portno) +" "+ groupid;
            sendtotracker(tosend);
        }
        else if(strcmp(inp, "leave_group")==0){
            string groupid = strtok(NULL, " ");
            string tosend = "lg "+ inttocc(portno) +" "+groupid;
            sendtotracker(tosend);
        }
        else if(strcmp(inp, "list_requests")==0){
            string groupid = strtok(NULL, " ");
            string tosend = "listr "+groupid;
            sendtotracker(tosend);
        }
        else if(strcmp(inp, "accept_request")==0){
            string groupid = strtok(NULL, " ");
            string userid = strtok(NULL, " ");
            string tosend = "ar "+groupid+" "+userid;
            sendtotracker(tosend);
        }
        else if(strcmp(inp, "list_groups")==0){
            string tosend = "listg";
            sendtotracker(tosend);
        }
        else if(strcmp(inp, "list_files")==0){
            string groupid = strtok(NULL, " ");
            string tosend = "lf "+ groupid;
            sendtotracker(tosend);
        }
        else if(strcmp(inp, "upload_file")==0){
            string filepath = strtok(NULL, " ");
            string groupid = strtok(NULL, " ");
            tserver(portno, filepath, groupid);
        }
        else if(strcmp(inp, "download_file")==0){
            string groupid = strtok(NULL, " ");
            string filepath = strtok(NULL, " ");
            string destpath = strtok(NULL, " ");
            tclient(groupid, filepath, portno);
            string filedir = filepath+" "+destpath;
            argo->file = filedir;
            pthread_create(&clientthread, NULL, client, argo);
        }
        else if(strcmp(inp, "logout")==0){
            string tosend = "lo "+ inttocc(portno);
            sendtotracker(tosend);
        }
        else if(strcmp(inp, "show_downloads")==0){
            string tosend = "sd";
            sendtotracker(tosend);
        }
        else if(strcmp(inp, "stop_share")==0){
            string groupid = strtok(NULL, " ");
            string filepath = strtok(NULL, " ");
            string tosend = "ss "+groupid+ " "+ filepath;
            sendtotracker(tosend);
        }
    }
    
    pthread_join(clientthread, NULL);
    pthread_join(serverthread, NULL);
    return 0;
}
