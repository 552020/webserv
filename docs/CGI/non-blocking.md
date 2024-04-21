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

---

STEPS

- Pipe Creation: Creates two pipes using T::pipe:
- server_to_cgi_pipe: For sending data from the server to the CGI process (such as HTTP request body).
- cgi_to_server_pipe: For receiving data from the CGI process back to the server (such as the output of the CGI script).
- Process Forking: Forks the current process using The return value forked_pid distinguishes between the parent process (the server) and the child process (which will execute the CGI script).

- Child Process (CGI Script Execution):
- Sets up the file descriptors:
- Closes the unused ends of the pipes.
- Duplicates the read end of the server_to_cgi_pipe to STDIN, so the CGI script reads input from this pipe.
- Duplicates the write end of the cgi_to_server_pipe to STDOUT, so the output of the CGI script is sent back to the server through this pipe.
- Executes the CGI script

Parent Process (Server):

- Closes the ends of the pipes that are now exclusively used by the child process.
- Stores the PID (Process ID) of the forked child process in the client object. This allows the server to manage and track the CGI process associated with each client.
- Maps the CGI process's PID to the client's file descriptor, enabling a correspondence between the CGI process and the client it's serving.

- Manages the server's end of the pipes for communication with the CGI process:
- If there's no data to write to the CGI, closes the write end immediately.
- Otherwise, registers the write end of the server_to_cgi_pipe for monitoring write readiness (POLLOUT), enabling the server to write request body data to the CGI script.
- Registers the read end of the cgi_to_server_pipe for monitoring read readiness (POLLIN), enabling the server to read the CGI script's output.

File Descriptor for Reading: After setting up the pipes and forking the child process to run the CGI script, your server will need to read the output produced by the CGI. This output is sent back through the CGIToServer pipe, specifically through the pipe's read end. By adding this read end FD to your multiplexer, your server can be notified when there's output ready to be read (e.g., using select(), poll(), or epoll() on Linux). This way, you can asynchronously read the CGI output as soon as it becomes available, without blocking your server.

if (isFDReady(cgiOutputFD)){
std::string cgiOutput = readCGIOutput(cgiOutputFD);
// Process CGI output...
}
