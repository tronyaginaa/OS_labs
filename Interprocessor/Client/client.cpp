#include "client.hpp"
#include <iostream>
#include <csignal>
#include <sys/syslog.h>
#include <semaphore.h>
#include <cstring>
#include <future>
#include <fcntl.h>

int main(int argc, char* argv[]){
    if (argc != 2){
        std::cout << "Invalid arguments. Must be one arg - host pid" << std::endl;
        return -1;
    }
    int host_pid = std::stoi(argv[1]);
    Client& client = Client::get_instance();
    if (!client.connect_to_host(host_pid)){
        syslog(LOG_ERR, "Client cannot conect");
        return -1;
    }
    std::cout << getpid()<<std::endl;
    return client.start();
}

bool inputAvailable(){
  struct timeval tv;
  fd_set fds;
  tv.tv_sec = 1;
  tv.tv_usec = 0;
  FD_ZERO(&fds);
  FD_SET(STDIN_FILENO, &fds);
  int ret = select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
  if(ret == 0 || ret == -1)
    return false;
  return (FD_ISSET(0, &fds));
}


Client::Client(void){
    struct sigaction sig{};
    memset(&sig, 0, sizeof(sig));
    sig.sa_sigaction = _signal_handler;
    sig.sa_flags = SA_SIGINFO;
    sigaction(SIGTERM, &sig, nullptr);
    sigaction(SIGUSR1, &sig, nullptr);
    openlog(NULL, LOG_PID, 0);
}


Client::~Client(){
    closelog();
}


Client& Client::get_instance(){
    static Client instance;
    return instance;
}


void Client::_signal_handler(int signum, siginfo_t* info, void* ptr){
    Client& client = get_instance();
    switch (signum) {
    case SIGUSR1:
        client._is_connected = true;
        syslog(LOG_INFO, "Host accepted connect");
        break;
    case SIGTERM:
        syslog(LOG_INFO, "Client killed");
        client._is_running = false;
        kill(client._host_pid, SIGUSR2);
        exit(EXIT_SUCCESS);
        break;
    }
    return;
}


int Client::start() {
    _is_running = true;
    while(!_is_connected) {
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
            syslog(LOG_ERR, "Can not get current time");
            return _end();
        }
    }
    pid_t pid = getpid();
    std::string pid_as_string = std::to_string(pid);
    sem_t* host_sem = sem_open(("/host_" + pid_as_string).c_str(), 0);
    if (host_sem == SEM_FAILED) {
        syslog(LOG_ERR, "Host semaphore error");
        return _end();
    }
    sem_t* client_sem = sem_open(("/client_" + pid_as_string).c_str(), 0);
    if (client_sem == SEM_FAILED) {
        syslog(LOG_ERR, "Client semaphore error");
        return _end();
    }
    while (_is_running){
        Message clientMsg;
         if(inputAvailable()){
            std::cout<<"got it"<<std::endl;
            std::cin.getline(clientMsg.m_message, STRING_MAX_SIZE);
            clientMsg.is_init = true;
        }
        Connection* conn = Connection::createConnection(pid, false,sizeof(Message));
        if (sem_post(client_sem) == -1) {
            syslog(LOG_ERR, "Client semaphore error");
            return _end();
        }
        
        
        {
            timespec t;
            clock_gettime(CLOCK_REALTIME, &t);
            t.tv_sec += 5;
            int s = sem_timedwait(client_sem, &t);
            if (s == -1){
                syslog(LOG_ERR, "Read semaphore timeout");
                return _end();;
            }
        }
        
        
        Message msg;
        if (!_input_messages.get_from_connection(pid, conn)) {
            syslog(LOG_ERR, "Client can not read");
            return _end();
        } else if(_input_messages.get_and_remove(pid, &msg))
            std::cout << "Host:" << msg.m_message<<std::endl;
        
        
        if(clientMsg.is_init){
            _output_messages.push(pid, clientMsg);
            if (!_output_messages.send_to_connection(pid, conn)) {
                syslog(LOG_ERR, "Client can not send messages");
                return _end();
            }
        }
        if (sem_post(client_sem) == -1) {
            syslog(LOG_ERR, "Client semaphore can not post");
            return _end();
        }
        delete conn;
    }
    return 0;
}

int Client::_end() {
    _is_running = false;
    kill(_host_pid, SIGUSR2);
    return -1;
}


bool Client::connect_to_host(pid_t pid) {
    _host_pid = pid;
    kill(_host_pid, SIGUSR1);
    if (clock_gettime(CLOCK_REALTIME, &_send_request_to_connect_time) == -1) {
        syslog(LOG_ERR, "Can not get current time");
        return false;
    }
    return true;
}

