#include "../queue.hpp"
#include "../Connections/connection.hpp"
#include <map>
#include <vector>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <future>
#include <bits/types/siginfo_t.h>


class Host{
private:
    queue<Message> _input_messages;
    queue<Message> _output_messages;
    std::map<pid_t, pthread_t> _clients;
    static Host _instance;
    bool _is_running = false;
    Message _output;
    std::vector<Connection*> connections;
    Host();
    ~Host();
    static void _signal_handler(int signum, siginfo_t* info, void *ptr);
    static void* _run(void*  argv);
    void _end();
public:
    static Host& getInstance();
    int start();
};
