#pragma once
#include <iostream>

class Connection
{
protected:
    int _desc;
    bool _host;
public:
    static Connection* createConnection(size_t id, bool isHost, size_t msg_size = 0);
    virtual void Get(void* buf, size_t count) = 0;
    virtual void Send(void* buf, size_t count) = 0;
    virtual ~Connection() = 0;
};



