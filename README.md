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

404 Not Found
-------------
Indicates that the client can communicate with the server, but the requested resource could not be found.



Preventing Zombies
------------------
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

In this project the infinite loop that accepts and handles client connections forks when the `accept()` system call is successful.

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

See [TCP/IP Sockets in C][4] section 6.4.


Resources
---------
* [Good overview of programme flow][1]
* [nweb][2]: tiny, safe web server - very useful article
* [Notes on forking the process][3] when a client connection has been established
* [Beej's notes on forking][5]


[1]: https://stackoverflow.com/a/2338837/3590673
[2]: https://www.ibm.com/developerworks/systems/library/es-nweb/index.html
[3]: https://stackoverflow.com/a/13669947/3590673
[4]: https://www.amazon.co.uk/TCP-IP-Sockets-Practical-Programmers/dp/0123745403
[5]: http://beej.us/guide/bgipc/html/multi/fork.html
