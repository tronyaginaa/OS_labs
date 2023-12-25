//
//  Daemon.cpp
//  Daemon
//
//  Created by Alexandra on 26.10.2023.
//

#include "Daemon.hpp"
#include <signal.h>
#include <syslog.h>
#include <fstream>
#include <thread>
#include <unistd.h>
#include <vector>
#include <sys/stat.h>

 std::string Daemon::_config_path;

Daemon::Daemon(){
    _config_path = std::filesystem::absolute("Config.conf");
    _read_config_file();
}

Daemon& Daemon::getInstance(){
    static Daemon instance;
    return instance;
}

void Daemon::start(){
    _close_existing_process();
    _fork();
    while(true){
        _folders_chek();
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

void Daemon::_fork(){
    pid_t pid = fork();
    if (pid != 0)
        exit(EXIT_FAILURE);
    if (setsid() == -1)
        exit(EXIT_FAILURE);
    signal(SIGHUP,_sighup_handler);
    signal(SIGTERM,_sigterm_handler);
    umask(0);
    openlog("Daemon", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "Daemon started");
    std::ofstream pid_file(_pid_path);
    if (pid_file.is_open()){
        syslog(LOG_INFO, ".pid overwritten");
        pid_file << getpid();
    }else {
        syslog(LOG_INFO, ".pid connot be overwritten. The started Daemon was deleted.");
        exit(EXIT_FAILURE);
    }
}

void Daemon::_sighup_handler(int signal){
    _read_config_file();
}

void Daemon::_sigterm_handler(int signal){
    syslog(LOG_INFO, "Daemon killed");
    exit(EXIT_SUCCESS);
}

void Daemon::_close_existing_process(){
    std::ifstream pid_file(_pid_path);
    if(pid_file.is_open()){
        pid_t pid;
        pid_file >> pid;
        std::string path = "/proc/" + std::to_string(pid);
        std::ifstream dir(path);
        if (dir) {
            kill(pid, SIGTERM);
            syslog(LOG_INFO, "Killing another instance");
        }
    } else {
        syslog(LOG_INFO, ".pid file cannot be read. Daemon start stopped.");
        exit(EXIT_FAILURE);
    }
}


void Daemon::_folders_chek(){
    for(auto& folder : _proccesed_folders)
        if (std::filesystem::exists(folder.first)) 
            _delete_files(folder);
}


uintmax_t Daemon::_get_size(std::filesystem::path file_path){
    return std::filesystem::file_size(file_path);
}


void Daemon::_delete_files(std::pair<std::filesystem::path, uintmax_t> folder){
    uintmax_t folder_content_size = 0;
    for (const auto& file : std::filesystem::directory_iterator(folder.first))
        if (std::filesystem::is_regular_file(file.path()))
            folder_content_size += _get_size(file.path());
    if (folder_content_size > folder.second){
        for (const auto& file : std::filesystem::directory_iterator(folder.first))
            std::filesystem::remove(file.path());
        syslog(LOG_INFO, "Files have been deleted");
    }
}


void Daemon::_read_config_file(){
    _proccesed_folders.clear();
    std::ifstream file(_config_path);
    if(file.is_open()){
        std::string path;
        uintmax_t size;
        while (file >> path >> size)
            _proccesed_folders.push_back(std::make_pair(std::filesystem::path(path), size));
        syslog(LOG_INFO, "Config file read");
    }else
        syslog(LOG_INFO, "Сonfig file cannot be read");
}
