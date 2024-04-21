PIPES

// Make read end of the pipe non-blocking
fcntl(pipeFD[0], F_SETFL, O_NONBLOCK);

// Make write end of the pipe non-blocking
fcntl(pipeFD[1], F_SETFL, O_NONBLOCK);

If your server operates in a single-threaded manner, blocking on pipe read operations could be a significant issue. Even though each client has its dedicated CGI process and pipe, if the server needs to handle multiple clients simultaneously, blocking on one client's CGI output means that other clients will have to wait.

FD:

You'll add the file descriptors (FDs) of the pipes to your array of FDs monitored by poll(). This way, your server can react to data being available for reading from the CGI process or the pipe being ready for writing without blocking.

Example Flow
Request arrives: Router determines it's a CGI request.
Setup pipes: Create pipes for stdin and stdout of the CGI process. Set them to non-blocking mode.
Fork and exec: Fork a new process. In the child, redirect stdin and stdout to the respective pipes and exec the CGI script. In the parent, close the ends of the pipes not used by the parent.
Polling: Add the relevant FDs to the poll() call to monitor for readiness to read/write.
Data handling: Read from/write to the CGI process as data becomes available or as needed by your application.
Process termination: Periodically attempt a non-blocking waitpid() on the CGI process to check if it has exited.
