# proxyServer
The proxy server receives an HTTP request from the client and sends the response back to the client.


threadpool.c
The pool is implemented by a queue. When the server gets a connection (getting back from
accept()), it should put the connection in the queue. When there will be available thread (can be
immediately), it will handle this connection (read request and write response).


input: proxyServer <port> <pool-size> <max-number-of-request> <filter>
output:
server side:(if valid request)
HTTP request =
<http request>
LEN = N
File is given from origin server/local filesystem
Total response bytes: N
client side will recieve the response from the server of his request

compile: gcc proxyServer.c threadpool.c -o combind -lpthread
run:server:./combind <port> <pool-size> <max-number-of-request> <filter>
   client:telnet localhost <port>

remarks:
request must end with \r\n\r\n
