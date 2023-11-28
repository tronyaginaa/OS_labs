#include "host.hpp"
#include <csignal>
#include <iostream>
#include <sys/syslog.h>
#include <semaphore.h>
#include <cstring>
#include <future>
#include <fcntl.h>
#include <string>
#include <thread>
#include <string>

int main(int argc, char* argv[]){
    auto& host = Host::getInstance();
    std::cout << getpid()<<std::endl;
    return host.start();
}


Host::Host(){
    struct sigaction sig{};
    memset(&sig, 0, sizeof(sig));
    sig.sa_sigaction = _signal_handler;
    sig.sa_flags = SA_SIGINFO;
    sigaction(SIGTERM, &sig, nullptr);
    sigaction(SIGUSR1, &sig, nullptr);
    sigaction(SIGUSR2, &sig, nullptr);
    openlog(NULL, LOG_PID, 0);
}


Host::~Host(){
    for (auto iter  = _clients.begin(); iter  != _clients.end(); ++iter ) {
          sem_t* host_sem = sem_open(( "/host_" + std::to_string(iter ->first)).c_str(), 0);
          sem_t* client_sem = sem_open(("/client_" + std::to_string(iter ->first)).c_str(), 0);
          if (sem_close(host_sem) == -1)
              syslog(LOG_ERR, "Host semaphore close error");
          if (sem_close(client_sem) == -1)
              syslog(LOG_ERR, "Client %i semaphore close error", iter ->first);
      }
      for (auto& conn : connections)
          delete conn;
      closelog();
}


Host& Host::getInstance(){
    static Host instance;
    return instance;
}


void Host::_signal_handler(int signum, siginfo_t *info, void *ptr){
    auto& host = getInstance();
    pid_t pid = info->si_pid;
    switch(signum){
        case SIGUSR1: {
            std::cout << "Connect from client:"<< pid <<std::endl;
            sem_t* host_sem = sem_open(("/host_" + std::to_string(pid)).c_str(), O_CREAT | O_EXCL, 0666, 0);
            if (host_sem == SEM_FAILED) {
                syslog(LOG_ERR, "Host semaphore error");
                break;
            }
            sem_t* client_sem = sem_open(("/client_" + std::to_string(pid)).c_str(), O_CREAT | O_EXCL, 0666, 0);
            if (client_sem == SEM_FAILED) {
                syslog(LOG_ERR, "Client semaphore error");
                break;
            }
            syslog(LOG_INFO, "Client pid %i connected", pid);
            host._clients.insert({pid ,0});
            host.connections.push_back(Connection::createConnection(pid, true));
            kill(pid, SIGUSR1);
            break;
        }
        case SIGUSR2:{
            host._clients.erase(pid);
            break;
        }
        case SIGTERM:{
            for (auto iter  = host._clients.begin(); iter  != host._clients.end(); ++iter )
                kill(iter ->first, SIGTERM);
            std::this_thread::sleep_for(std::chrono::microseconds(20));
            host._is_running = false;
            syslog(LOG_INFO, "Host killed");
            exit(EXIT_SUCCESS);
            break;
        }
    }
}


Message Host::_get_output_message(){
    Message msg;
    std::cin.getline(msg.m_message, STRING_MAX_SIZE);
    msg.is_init = true;
    return msg;
}


void Host::_end(){
    _is_running = false;
}

void* Host::_run(void*  argv){
    auto& host = Host::getInstance();
    pid_t pid = *(pid_t*)argv;
    std::string pid_as_string = std::to_string(pid);
    sem_t* host_sem = sem_open(("/host_" + pid_as_string).c_str(), 0);
    if (host_sem == SEM_FAILED) {
        syslog(LOG_ERR, "Host semaphore error");
        return nullptr;
    }
    sem_t* client_sem = sem_open(("/client_" + pid_as_string).c_str(), 0);
    if (client_sem == SEM_FAILED) {
        syslog(LOG_ERR, "Client semaphore error");
        return nullptr;
    }
    Connection* conn = Connection::createConnection(pid, false);
    
    {
        timespec t;
        clock_gettime(CLOCK_REALTIME, &t);
        t.tv_sec += 5;
        int s = sem_timedwait(client_sem, &t);
        if (s == -1){
            syslog(LOG_ERR, "Read semaphore timeout");
            host._end();
            return nullptr;
        }
    }
    
    Message msg;
    if (!host._input_messages.get_from_connection(pid, conn)) {
        syslog(LOG_ERR, "Host can not read");
        return nullptr;
    }else if (host._input_messages.get_and_remove(pid, &msg)){
        for (auto iter  = host._clients.begin(); iter  != host._clients.end(); ++iter ){
            if(iter ->first != pid){
                Message other_clients_msg;
                other_clients_msg.is_init = true;
                std::string id = "Client[" + std::to_string(pid) + "]:";
                strcpy(other_clients_msg.m_message,strcat(const_cast<char*>(id.c_str()), msg.m_message));
                host._output_messages.push(iter ->first, other_clients_msg);
            }
        }
        std::cout<<"["<<pid<<"]: "<<msg.m_message<<std::endl;
    }
    
    
    if(host._output.is_init)
        host._output_messages.push(pid, host._output);
    if (!host._output_messages.send_to_connection(pid, conn)) {
            syslog(LOG_ERR, "Host can not send messages");
            return nullptr;
    }
    if (sem_post(host_sem) == -1) {
           syslog(LOG_ERR, "Host semaphore can not post");
           return nullptr;
    }
    
    delete conn;
    return 0;
}

int Host::start(){
    _is_running = true;
    std::future<Message> future =  std::async(_get_output_message);
    while(_is_running) {
        
        Message msg;
        std::chrono::seconds timeout(1);
        if (future.wait_for(timeout) == std::future_status::ready){
        msg = future.get();
        future = std::async(_get_output_message);
        }
        _output = msg;
        
        
        for (auto iter  = _clients.begin(); iter  != _clients.end(); ++iter ) {
            if (pthread_create(&(iter ->second), nullptr, _run, (void*)(&(iter ->first))) != 0) {
            syslog(LOG_ERR, "Can not create thread");
            return -1;
            }
        }
        
        
        for (auto iter  = _clients.begin(); iter  != _clients.end(); ++iter ) {
        void* res;
        if (pthread_join(iter ->second, &res) != 0) {
            syslog(LOG_ERR, "Error in thread");
            return -1;
            }
        }
        
        
        for (auto iter  = _clients.begin(); iter  != _clients.end(); ++iter ) {
            sem_t* host_sem = sem_open(("/host_" + std::to_string(iter ->first)).c_str(), 0);
            if (sem_post(host_sem) == -1) {
                syslog(LOG_ERR, "Can not post host semaphore");
                }
        }
    }
    return 0;
}



