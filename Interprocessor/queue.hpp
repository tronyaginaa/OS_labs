#pragma once

#include <queue>
#include <map>
#include <sys/syslog.h>
#include <mutex>
#include "./Connections/connection.hpp"

enum{
    STRING_MAX_SIZE = 300
};

struct Message{
    bool is_init = false;
    char m_message[STRING_MAX_SIZE];
};

template <typename T>
class queue {
private:
    std::map<pid_t, std::queue<T>> _storage;
    mutable pthread_mutex_t _mutex;
public:
    queue(){
        pthread_mutex_init(&_mutex, NULL);
    }
    ~queue(){
        pthread_mutex_destroy(&_mutex);
    }
    void push(pid_t pid, const T &val){
        pthread_mutex_lock(&_mutex);
        _storage[pid].push(val);
        pthread_mutex_unlock(&_mutex);
    }
    
    
    bool get_and_remove(pid_t pid,T *msg){
        pthread_mutex_lock(&_mutex);
        if (_storage[pid].empty()){
            pthread_mutex_unlock(&_mutex);
            return false;
        }
        *msg = _storage[pid].front();
        _storage[pid].pop();
        pthread_mutex_unlock(&_mutex);
        return true;
    }
    
    
    bool get_from_connection(pid_t pid,Connection *conn){
        pthread_mutex_lock(&_mutex);
        uint32_t amount = 0;
        conn->Get(&amount, sizeof(uint32_t));
        for (uint32_t i = 0; i < amount; i++){
            T msg = {0};
            conn->Get(&msg, sizeof(T));
            _storage[pid].push(msg);
        }
        pthread_mutex_unlock(&_mutex);
        return true;
    }
    
    
    bool send_to_connection(pid_t pid, Connection *conn){
        pthread_mutex_lock(&_mutex);
        uint32_t amount = _storage[pid].size();
        if (amount > 0)
        conn->Send(&amount, sizeof(uint32_t));
        while (!_storage[pid].empty()){
            conn->Send(&_storage[pid].front(), sizeof(T));
            _storage[pid].pop();
        }
        pthread_mutex_unlock(&_mutex);
        return true;
    }
    
    
   
};

