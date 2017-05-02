This is a very simple websocket implementation in C/C++<br>

Functionality:
- Handshake
- Support for multiple clients
- Supports large messages
- Can broadcast a message to all active clients
- Can send a message to a specific client
- Can list all live clients
- Can give you a buffer of all messages revieved

Check example.cpp for a very basic client using all the functionalities<br>
How to compile the example:<br>
g++ example.cpp websocket.cpp include/base64.cpp -lcrypto -pthread<br>


Confirmed to run well on Manjaro Linux<br>
For some reason it takes a while to connect a client to the server, but it works.<br>


Sources of information:<br>
https://developer.mozilla.org/en-US/docs/Web/API/WebSocket<br>
Incredibly well written source for websocket implementation<br>
https://tools.ietf.org/html/rfc6455<br>
Detailed descriptions of everything<br>
