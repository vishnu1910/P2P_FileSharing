# Assignment 4 
## Distributed File Sharing System 

A peer-to-peer file transfer system made to simulate the working of Bit-torrent.

## Working

- The system works on a ONE tracker system only.
- The peers sends their port info during login and their respective files that they have during upload.
- The tracker keeps track of all the ports and their files using an unordered map.
- Whenever a download request arrives the tracker sends all the port numbers which possess the requested file to the client that requested the download.
- The client then requests the other clients for the same file in a multithreaded fashion.
- While sending the requests the requesting client also sends an index and the total number of seeders to each seeder.
- Using this each seeder can then calculate which part of the file they need to send.
- Subsequently using the same index that was given to the seeders the client can order the incoming packets and write it into a new file.

## Assumptions made
- Each client has its own folder which is named after its port number and can only upload files in that folder.

## Getting Started

- To execute the program, first open atleast two terminals, one for tracker and one for client.
You may open any number of terminals for the client depending on the number of peer you want in you system.
- Make sure you are inside the tracker folder to run the tracker.cpp file and the client folder to run the client.cpp file.
- To run the tracker:
``` 
g++ tracker.cpp -o tracker
./tracker 
```
- To run the client:
```
g++ client.cpp -o client
./client <ip_address>:<port> ../tracker/tracker_info.txt
```
- After executing client file, something like
```
12345>
```
will be printed where you need to execute your commands.(where 12345 will be you client's port number)


## Features: 
### Create User Account: 
```
create_user <user_id> <passwd>
```
- Creates a new user account.

### Login:
```
login <user_id> <passwd>
```
- Logs in with given credentials.

### Create Group:
```
create_group <group_id>
```
- Creates a new group in which files can be shared
### Join Group:
```
join_group <group_id>
```
- Joins a pre-existing group
### Leave Group:
```
leaves_group <group_id>
```
- Leaves a pre-existing group

### List pending join:
Not implemented
### Accept Group Joining Request:
Not implemented
### List All Group In Network:
```
list_groups
```
- Lists all groups created during a session
### List All sharable Files In Group: 
```
list_files <group_id>
```
- Lists all files in a given group.
### Upload File:
```
upload_file <file_name> <group_id>
```
- Relays information about the possession of a file by the client to the tracker which primes it for seeding when a request arises.

### Download File:
```
download_file <group_id> <file_name> <destination_path>
```
- Sends request to download a particular file
### Logout:
```
logout
```
- Logs out
### Show downloads:
Not implemented
### Stop sharing:
Not implemented