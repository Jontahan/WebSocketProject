#include "websocket.h"

int main(int argc, char *argv[]){
    // hosting a very basic simple test client
    // tip: use the development tools in the browser to see the javascript in a better format

    std::string serverip = "10.20.224.59";
    std::string webpage =
        "HTTP/1.1 200 OK\r\n"
        "\n<script>"
        "var ws = new WebSocket('ws://" + serverip + ":8080');\n"
        "ws.addEventListener('open',function(event){"
        "\n\tconsole.log('open!!');\n\t"
        "ws.send('yo bro!');\n});\n"
        "var sendmessage = function(){"
        "ws.send(document.getElementById(\"tf\").value);};"
        "ws.onmessage = function(event){console.log(event.data);};"
        "ws.onclose = function(){console.log('closed succesfully!')};"
        "var closews = function(){ws.close()};"
        "</script>"
        "<input id=\"tf\" type=\"text\">"
        "<button onclick=\"sendmessage()\">send</button>\r\n"
        "<button onclick=\"closews()\">close</button>\r\n";

    // ws::websocket is the server hosting websocket
    // below you can see examples of all public functions
    
    ws::websocket ws;
    ws.start(8080, webpage);
    while(1){
        std::string input = "";
        std::cout << "\n1: check message buffer\n2: send message\n3: list client ids\n4: broadcast message to all clients\n5: host echo-server\n6: host chat-server" << std::endl;
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
	if(option == 4){
            std::cout << "\nmessage:" << std::endl;
            getline(std::cin, input);

            int retcode = ws.broadcast_message(input);
            if(retcode == -1){
                std::cout << "something went wrong" << std::endl;
            }
            if(retcode == 1){
                std::cout << "message succesfully broadcasted" << std::endl;
            }
	}
	if(option == 5){
	    while(true){
		std::vector<ws::wsmsg> messages = ws.get_msg_buffer();
		for(int i = 0; i < messages.size(); ++i){
		    ws.send_message(messages[i].sock_id, messages[i].message);
		}
       		usleep(1000);
	    }
	}
	if(option == 6){
	    while(true){
		std::vector<ws::wsmsg> messages = ws.get_msg_buffer();
		for(int i = 0; i < messages.size(); ++i){
		    ws.broadcast_message(messages[i].message);
		}
       		usleep(1000);
	    }
	}
    };
}
