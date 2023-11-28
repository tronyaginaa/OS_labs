#include "connection.hpp"

#include <cstring>
#include <fcntl.h>
#include <mqueue.h>
#include <syslog.h>

namespace {
    class ConnectionMq : public Connection{
    private:
        static std::string const pathname;
        std::string name;
        const int MAX_MSG_SIZE = 1024;
    public:
        ConnectionMq(size_t id, bool create);
        ~ConnectionMq();
        void Get(void* buffer, size_t count) override;
        void Send(void* buffer, size_t count) override;

    };
    std::string const ConnectionMq::pathname = "/tmp/mymq";
}

Connection* Connection::createConnection(size_t id, bool create) {
    return new ConnectionMq(id, create);
}

Connection::~Connection() {}


ConnectionMq::ConnectionMq(size_t id, bool isHost) {
    _host = isHost;
    name = pathname + std::to_string(id);
    if (isHost) {
        struct mq_attr attr;
        attr.mq_maxmsg = 10;
        attr.mq_msgsize = MAX_MSG_SIZE;
        _desc = mq_open(name.c_str(), O_RDWR | O_CREAT, 0666, &attr);
    }
    else
        _desc = mq_open(name.c_str(), O_RDWR);
    if (_desc == -1)
        syslog(LOG_ERR, "Can not open queue");
}

ConnectionMq::~ConnectionMq() {
    if (mq_close(_desc) == -1)
        std::cout << "Bad close" << std::endl;
    if (_host)
        mq_unlink(name.c_str());
}

void ConnectionMq::Get(void* buffer, size_t count) {
    struct mq_attr attr;
    if (mq_getattr(_desc, &attr) == -1)
        syslog(LOG_ERR, "Can not get attr");
    if(mq_receive(_desc, (char*)buffer, count, NULL) == -1)
        syslog(LOG_ERR, "Can not read");
}

void ConnectionMq::Send(void* buffer, size_t count) {
    if (mq_send(_desc, (char*)buffer, count, 0) == -1)
        syslog(LOG_ERR, "Can not write");
}
