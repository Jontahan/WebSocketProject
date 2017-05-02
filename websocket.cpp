#include "websocket.h"
/*#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <string>
#include <iostream>
#include <openssl/sha.h>
#include "include/base64.h"
#include <bitset>
#include <thread>
#include <mutex>
#include <vector>
*/


    
std::string ws::websocket::generate_accept(std::string key){
    key.append("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    unsigned char const* hash = SHA1(reinterpret_cast<const unsigned char*>(key.c_str()), key.length(), nullptr);
    return base64_encode(hash, 20);
}

std::string ws::websocket::mask_message(std::string message, char* mask){
    std::string out = "";
    for(int i = 0 ; i < message.size(); ++i)
	out += char(message[i] ^ mask[i % 4]);
    return out;
}

void ws::websocket::new_websocket(int connfd){
    std::thread thr([this](int fdcp) {
	    std::vector<char*> segments;
	    bool open = true;
	    while(open){
		char wsbuff[1400];
		int fsize = read(fdcp, wsbuff, 1400);
		if(fsize > 0){
		    int opcode = wsbuff[0] & 0x0f;
		    if(opcode == 0x01){
			uint64_t size = char(wsbuff[1] ^ 0x80);
			int offset = 6; 
			if(size == 126){
			    size = (((unsigned int)wsbuff[2] << 8) | (unsigned char)wsbuff[3]);
			    offset = 8;
				}else if(size == 127){
			    size = 0;
			    for(int i = 0; i < 8; ++i)
				size |= (uint64_t(wsbuff[2 + i]) << 8 * (7 - i));
			    offset = 14;
			}
			
			char mask[4] = {wsbuff[offset - 4], wsbuff[offset - 3],
						wsbuff[offset - 2], wsbuff[offset - 1]};
			char* msg_data = (char*)malloc(sizeof(char) * size);
			memcpy(msg_data, wsbuff + offset, size);
			std::string message = mask_message(std::string(msg_data), mask);
			std::lock_guard<std::mutex> lock(messages_mx);
			messages.push_back({fdcp, message});
			char* response = (char*)malloc(sizeof(char) * (fsize - 4));
				
			memcpy(response, wsbuff, offset - 4);
			memcpy(response + (offset - 4), message.c_str(), size);
			response[1] ^= 0x80;
			
			send(fdcp, response, fsize - 4, 0);
			free(response);
				free(msg_data);
		    }else if(opcode == 0x08){
			send(fdcp, wsbuff, 1, 0);
			close(fdcp);
			open = false;
			int index = -1;
			for(int i = 0; i < clients.size(); ++i)
			    if(clients[i] == fdcp)
				index = i;
			if(index >= 0){
			    std::lock_guard<std::mutex> lock(clients_mx);
			    clients.erase(clients.begin() + index);
			}
		    }
		}
		usleep(1000);
	    }
	}, connfd);
    thr.detach();
}

void ws::websocket::run(){
    bool ws = false;
    while(1)
    {
	connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
	socklen_t ll = 14;
	getpeername(connfd, &caddr, &ll);
	std::string address(caddr.sa_data);
	
	char buffer[1400];
	
	read(connfd, buffer, 1400);
	
	std::string buf(buffer);
	
	
	std::string reply;
	
	ws = strstr(buffer, "Upgrade: websocket");
	
	if(ws){
	    std::string key = buf.substr(buf.find("Sec-WebSocket-Key") + 19,
					 buf.substr(buf.find("Sec-WebSocket-Key")).find("\n") - 20);
	    std::string key_accept = generate_accept(key);
	    
	    reply =
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: " + key_accept + "\r\n\r\n";
	    
	}else{
	    reply = webpage;
	}
	send(connfd, reply.c_str(), reply.size(), 0);
	
	
	if(ws){
	    new_websocket(connfd);
	    clients.push_back(connfd);
	    ws = false;
	}
	usleep(1000);
    }
}

void ws::websocket::start(int port, std::string wp){
    webpage = wp;
    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port); 
    
    bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(listenfd, port);
    std::thread thr(&websocket::run, this);
    thr.detach();
}

int ws::websocket::send_message(int id, std::string message){
    bool open = false;
    for(int i = 0; i < clients.size(); ++i){
	if(clients[i] == id)
	    open = true;
    }
    if(open){
	int length = message.size();
	char* frame = (char*)malloc(sizeof(char)*(length + 2));
	frame[0] = 0x81;
	frame[1] = length ^ (length & 0x80 ? 0x80 : 0x00);
	memcpy(frame + 2, message.c_str(), length);
	send(id, frame, length + 2, 0);
	return 1;
    }else{
	return -1;
    }
}

std::vector<ws::wsmsg> ws::websocket::get_msg_buffer(){
    std::vector<wsmsg> cpy;
    std::lock_guard<std::mutex> lock(messages_mx);
    for(int i = 0; i < messages.size(); ++i)
	cpy.push_back(messages[i]);
    return cpy;
}

std::vector<int> ws::websocket::get_clients(){
    std::vector<int> cpy;
    std::lock_guard<std::mutex> lock(clients_mx);
    for(int i = 0; i < clients.size(); ++i)
	cpy.push_back(clients[i]);
    return cpy;
}

/*
int main(int argc, char *argv[])
{
    std::string serverip = "10.20.224.59";
    std::string webpage =
	"HTTP/1.1 200 OK\r\n"
	"\n<script>"
	"var ws = new WebSocket('ws://" + serverip + ":8080');\n"
	"ws.addEventListener('open',function(event){"
	"\n\tconsole.log('open!!');\n\t"
	"ws.send('yo bro!');\n});\n"
	"var sendmessage = function(){"
	"console.log('click');"
	"ws.send('test123567890"
	"A1234567890B1234567890C1234567890D1234567890E1234567890F1234567890"
	"F1234567890G1234567890H1234567890I1234567890J1234567890K1234567890"
	"L1234567890M1234567890N1234567890O1234567890P1234567890Q1234567890');};"
	"ws.onmessage = function(event){console.log(event.data);};"
	"ws.onclose = function(){console.log('closed succesfully!')};"
	"var closews = function(){ws.close()};"
	"</script>"
	"<input type=\"text\">"
	"<button onclick=\"sendmessage()\">send</button>\r\n"
	"<button onclick=\"closews()\">close</button>\r\n";
    
    ws::websocket ws;
    ws.start(8080, webpage);
    while(1){
	std::string input = "";
	std::cout << "\n1: check message buffer\n2: send message\n3: list client ids" << std::endl;
	getline(std::cin, input);
	int option = stoi(input);
	if(option == 1){
	    std::vector<ws::wsmsg> messages = ws.get_msg_buffer();
	    for(int i = 0; i < messages.size(); ++i)
		std::cout << "client" << messages[i].sock_id <<  ": " << messages[i].message << std::endl;
	}
	if(option == 2){
	    std::cout << "sent msg to socket number: ";
	    getline(std::cin, input);
	    int id = stoi(input);
	    std::cout << "\nmessage:" << std::endl;
	    getline(std::cin, input);
	    
	    int retcode = ws.send_message(id, input);
	    if(retcode == -1){
		std::cout << "client does not exist or is closed" << std::endl;
	    }
	    if(retcode == 1){
		std::cout << "message sent succesfully" << std::endl;
	    }
	}
	if(option == 3){
	    std::vector<int> clients = ws.get_clients();
	    std::cout << "active clients:" << std::endl;
	    for(int i = 0; i < clients.size(); ++i)
		std::cout << clients[i] << std::endl;
	}
    };
}
*/
