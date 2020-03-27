Basic HTTP Server in C
======================
Programme flow for serving static content:

* Load settings.
* Wait for connection.
* When a connection is received, create a new thread and transfer the connection to it.

In the connection thread:

* Analyse the request.
* Load requested file.
* Stream the file content.

* Send response.
* Close socket.

Basic Multi-Client Support
--------------------------
fork() the process HERE, after accepting a clientSocket.
see: https://stackoverflow.com/a/13669947/3590673


Resources
---------
* [Good overview of programme flow][1]
* [nweb][2]: tiny, safe web server - very useful article
* [Notes on forking the process][3] when a client connection has been established


[1]: https://stackoverflow.com/a/2338837/3590673
[2]: https://www.ibm.com/developerworks/systems/library/es-nweb/index.html
[3]: https://stackoverflow.com/a/13669947/3590673
