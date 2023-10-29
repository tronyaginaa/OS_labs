//
//  Daemon.hpp
//  Daemon
//
//  Created by Alexandra on 26.10.2023.
//

#include <iostream>
#include <filesystem>
#include <vector>

class Daemon {
public:
    static Daemon& getInstance();
    void _start();
private:
    Daemon();
    Daemon(const Daemon&) = delete;
    Daemon& operator=(Daemon&) = delete;
    
    const std::string _pid_path = "/home/sss/Desktop/Daemon/Deamon.pid";
    static std::string _config_path;
    static inline std::vector<std::pair<std::filesystem::path, uintmax_t>> _proccesed_folders = {};
    
    
    void _fork();
    void _close_existing_process();
    static void _read_config_file();
    static void _sighup_handler(int signal);
    static void _sigterm_handler(int signal);
    
    void _folders_chek();
    void _delete_files(std::pair<std::filesystem::path, uintmax_t> folder);
    uintmax_t _get_size(std::filesystem::path file_path);
    
    
};

