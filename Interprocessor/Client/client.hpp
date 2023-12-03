#include "../queue.hpp"
#include "../Connections/connection.hpp"
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <bits/types/siginfo_t.h>

class Client{
private:
    queue<Message> _input_messages;
    queue<Message> _output_messages;
    pid_t _host_pid;
    bool _is_connected = false;
    bool _is_running = false;
    static Client _instance;
    Client();
    ~Client();
    struct timespec _send_request_to_connect_time;
    static void _signal_handler(int signum, siginfo_t* info, void* ptr);
    int _end();
public:
    static Client& get_instance();
    int start();
    bool connect_to_host(pid_t pid);
    
};
