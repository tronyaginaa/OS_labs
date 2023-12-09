#include "connection.hpp"

#include <fcntl.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace
{
    class ConnectionFifo : public Connection
    {
    private:
        static std::string const _path;
        std::string _fileName;
    public:
        ConnectionFifo(size_t id, bool isHost);
        ~ConnectionFifo();
        void Get(void* buf, size_t count) override;
        void Send(void* buf, size_t count) override;

    };
    std::string const ConnectionFifo::_path = "/tmp/myfifo";
}


Connection* Connection::createConnection(size_t id, bool isHost, size_t msg_size){
    return new ConnectionFifo(id, isHost);
}


Connection::~Connection(){}


ConnectionFifo::ConnectionFifo(size_t id, bool isHost){
    _fileName = _path + std::to_string(id);
    _host = isHost;
    if (isHost && mkfifo(_fileName.c_str(), 0777) < 0)
        std::cout << "Make fifo error" << std::endl;
    _desc = open(_fileName.c_str(), O_RDWR | O_NONBLOCK);
    if (_desc == -1)
        std::cout << "Open error" << std::endl;
}


ConnectionFifo::~ConnectionFifo(){
    close(_desc);
    if (_host) {
        remove(_fileName.c_str());
    }
}


void ConnectionFifo::Get(void* buf, size_t count){
    read(_desc, buf, count);

}


void ConnectionFifo::Send(void* buf, size_t count){
    write(_desc, buf, count);
}
