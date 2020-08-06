Basic HTTP Server in C
======================
This server is not (yet) suitable for production use.

At the moment, this is an educational project. Building a server from scratch is a good way to learn about HTTP. It's stable enough to serve local files - I'm using it to serve notes written in markdown and converted to HTML.

Overview
--------
This server is designed to serve static content using the HTTP protocol.

Programme flow:

* Load settings.
* Wait for connection.
* When a connection is received, create a forked process and transfer the connection to it.

In the connection process:

* Analyse the request
* Load requested file and prepare a response
* Send response
* Log the client server interaction
* Close socket

Build & Usage
-------------
This is a Linux project.

### Build
* Clone this repo
* `cd` into the repo
* Object files & binaries are by default added to the `bin` directory
* In the project root run `make`

### Usage
* Run `./bin/http-server` to serve the test files - default port is 8001.
* Access the site at `http://localhost:8001`
* If you want to specify a port, add it as the first command line argument
* `cd` into a directory of static HTML files and run the `http-server` script from there.

You could install the script into your `$PATH`, or create a suitable alias. I don't recommend this. In most cases you're better off running `python3 -m http.server` from your HTML directory.

Notes on Accepting Connections
------------------------------
The `accept()` system call extracts the first connection request on a queue of pending connections for the listening socket. It creates a new connected socket - a client socket - returning a file descriptor for that socket.

```c
#include <sys/types.h>
#include <sys/socket.h>

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

The `addr` argument to `accept()` is a pointer to a `struct sockaddr`. In the [`acceptTCPConnection()` function][6], a pointer to `struct sockaddr_storage` is typecast to the required `struct sockaddr`:

```c
struct sockaddr_storage genericClientAddr;
socklen_t clientAddrLen = sizeof(genericClientAddr);
memset(&genericClientAddr, 0, clientAddrLen);

int clientSocket = accept(serverSocket, (struct sockaddr *)&genericClientAddr, &clientAddrLen);
if (clientSocket < 0) {
	fprintf(stderr, "accept() failed");
	return -1;
}
```
Note that the generic `sockaddr_storage` struct is used, typecast to a generic `struct sockaddr` as required. The rationale for this decision is that the `sockaddr_storage` structure can hold either an IPv4 or IPv6 address.

Depending on the IP version being handled, you would create either a `struct sockaddr_in` for IPv4 or a `struct sockaddr_in6` for IPv6, typecasting one of these to the `struct sockaddr *` required by `accept()`. The `sockaddr_storage` struct can hold _either_ address format.

The act of casting to `struct sockadrr` for `accept()` and other functions is an idiosyncracy of socket programming.

Basic Multi-Client Support
--------------------------
The project uses a multi-process approach to handle multiple clients.

Multiple clients aren't really part of the use case at the moment. If this becomes important, I'd consider changing to a multi-threaded approach.

Forking Child Processes & Preventing Zombies
--------------------------------------------
In this project the infinite loop that accepts and handles client connections forks when the `accept()` system call is successful.

A zombie process is a defunct (finished, dead) process that still has an entry in the process table.

When a child process completes, it sends a `SIGCHLD` signal to the process that created it. The parent process can send a `wait()` system call, allowing access to the child's exit status. After the `wait()` call, the child's entry is removed from the process table.

If the child process entry is not removed from the process table, it is known as a zombie - the process is dead, but it hasn't moved on to silicon heaven. It has an entry in the process table available for the parent process to use. Zombies don't take up much resources - the memory used by the child process has been freed, and the zombie is simply an entry in the process table.

If zombies accumulate, they may prevent new processes being started, since their process ID number has not been freed, and in severe cases this may limit issuance of new process IDs. Because they are already dead (defunct), getting rid of zombies is known as "reaping".

Generally, after a parent process forks a child process, it must use `wait()` or `waitpid()` to access status information on the child process, after which time the entry is removed from the process table. 

>The `wait()` and `waitpid()` functions shall obtain status information pertaining to one of the caller's child processes.
>Various options permit status information to be obtained for child processes that have terminated or stopped.
>If status information is available for two or more child processes, the order in which their status is reported is unspecified.
>
>Man page: enter `man 3 wait` for more.


Because all errors are handled in the child process, the parent does not need to know the exit status of the child process. One option hten is to instruct the parent process to ignore the `SIGCHLD` signal, so it doesn't need to `wait()`. By using `signal(SIGCHLD, SIG_IGN);` in the parent process, the children don't become defunct - they die completely.

Another option is to keep track of the number of child processes created, and repeatedly calling `waitpid()` on this number of processes to clean up zombies:

```c
// Clean up zombies.
// At the end of the iteration of the infinite loop, remove any children that
// have completed.
// Loops through and `wait()`s on children, in the order in which they exit.
while (childProcessCount) {
	pid = waitpid((pid_t) -1, NULL, WNOHANG);
	if (pid < 0) {
		dieWithSystemMessage("waitpid() failed.");
	} else if (pid == 0) {
		break;
	} else {
		childProcessCount--;
	}
}
```
Note that this method leaves a single zombie around until the server process shuts down, unless you pass `0` as an option to `waitpid()` in place of `WNOHANG`.

To check for zombies:

```bash
# Display a process family tree, with zombies annotated `<devunct>`:
ps ef

# Alternatively:
ps aux | grep Z

# Similarly:
ps -el | grep Z
```

See [TCP/IP Sockets in C][4](book) section 6.4.

Setting the HTTP Return Code
----------------------------
The `setStatusString()` function sets a dynamically allocated string representing the HTTP return code.

The status codes are defined as an enum - because of their values, they can't be effectively used to dereference an array of character strings, hence this function.

This is not good for maintenance - codes are defined separately from the return strings - this is a compromise.

An alternative would be a hash map/associative array of integers indexed by `char*` value, but this is a small project and doesn't warrant this.

The approach taken by Apache httpd is to `#define` define codes (e.g. `#define HTTP_CONTINUE 100`), and using these values to dereference a large array of `char*` strings, using an `index_of_response()` function to compute the correct offsets from the x00 code of each level, such that a supplied index of 100 (the lowest possible return code value) returns the first element of the array (the correct string denoting a 100 return code). In this way, a supplied index of 200 returns the element at index 3, because there are only 3 1xx codes.

For more info see:

* [https://github.com/apache/httpd/blob/trunk/modules/http/http_protocol.c#L73][7]
* [https://github.com/apache/httpd/blob/trunk/modules/http/http_protocol.c#L795][8]
* [https://github.com/apache/httpd/blob/trunk/include/httpd.h#L479][9]

Resources
---------
* [Good overview of programme flow][1]
* [nweb][2]: tiny, safe web server - very useful article, few problems in the code (e.g. incorrectly determines file validity/extension)
* [Notes on forking the process][3] when a client connection has been established
* [Beej's notes on forking][5]


[1]: https://stackoverflow.com/a/2338837/3590673
[2]: https://www.ibm.com/developerworks/systems/library/es-nweb/index.html
[3]: https://stackoverflow.com/a/13669947/3590673
[4]: https://www.amazon.co.uk/TCP-IP-Sockets-Practical-Programmers/dp/0123745403
[5]: http://beej.us/guide/bgipc/html/multi/fork.html
[6]: https://github.com/csknk/http-server-static/blob/master/server.c#L84
[7]: https://github.com/apache/httpd/blob/trunk/modules/http/http_protocol.c#L73
[8]: https://github.com/apache/httpd/blob/trunk/modules/http/http_protocol.c#L795
[9]: https://github.com/apache/httpd/blob/trunk/include/httpd.h#L479
