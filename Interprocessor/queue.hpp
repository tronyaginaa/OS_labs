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
    mutable std::mutex _mutex;
public:
    void push(pid_t pid, const T &val){
        _mutex.lock();
        _storage[pid].push(val);
        _mutex.unlock();
    }
    
    
    bool get_and_remove(pid_t pid,T *msg){
        _mutex.lock();
        if (_storage[pid].empty()){
            _mutex.unlock();
            return false;
        }
        *msg = _storage[pid].front();
        _storage[pid].pop();
        _mutex.unlock();
        return true;
    }
    
    
    bool get_from_connection(pid_t pid,Connection *conn){
        _mutex.lock();
        uint32_t amount = 0;
        conn->Get(&amount, sizeof(uint32_t));
        for (uint32_t i = 0; i < amount; i++){
            T msg = {0};
            conn->Get(&msg, sizeof(T));
            _storage[pid].push(msg);
        }
        _mutex.unlock();
        return true;
    }
    
    
    bool send_to_connection(pid_t pid, Connection *conn){
        _mutex.lock();
        uint32_t amount = _storage[pid].size();
        if (amount > 0)
        conn->Send(&amount, sizeof(uint32_t));
        while (!_storage[pid].empty()){
            conn->Send(&_storage[pid].front(), sizeof(T));
            _storage[pid].pop();
        }
        _mutex.unlock();
        return true;
    }
    
    
    bool isEmpty(pid_t pid){
        return _storage[pid].empty();
    }
};

