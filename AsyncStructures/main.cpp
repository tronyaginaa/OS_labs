#include <iostream>
#include <filesystem>
#include <fstream>
#include <unistd.h>

#include "test.hpp"

int main(int argc, char *argv[]) {
    int writers_num ;
    int readers_num;
    int records_num;
    int readings_num;
    int tests_num;
    if (argc!= 2){
        std::cout<<"Expected 1 argument - config file path"<<std::endl;
        return EXIT_FAILURE;
    }
    std::ifstream file(argv[1]);
    if(file.is_open()){
        if(file >> writers_num >> readers_num >> records_num >> readings_num >> tests_num){
            std::cout << "--------Fine set---------" << std::endl;
            run_write_test(SetType::FINE_LIST, writers_num, records_num, tests_num);
            run_read_test(SetType::FINE_LIST, readers_num, readings_num, tests_num);
            std::cout << "\n-----Optimistic set-----" << std::endl;
            run_write_test(SetType::OPTIMISTIC_LIST, writers_num, records_num, tests_num);
            run_read_test(SetType::OPTIMISTIC_LIST, readers_num, readings_num, tests_num);
            if (writers_num + readers_num > sysconf(_SC_NPROCESSORS_ONLN) ){
                std::cout << " " << std::endl;
                return 0;
            }
            std::cout << "\n--------Fine set---------" << std::endl;
            run_general_test(SetType::FINE_LIST, writers_num, readers_num, records_num, readings_num, tests_num);
            std::cout << "\n-----Optimistic set-----" << std::endl;
            run_general_test(SetType::OPTIMISTIC_LIST, writers_num, readers_num, records_num, readings_num, tests_num);
        }
        return 0;
    }
    std::cout<<"Config cannot be opened"<<std::endl;
    return EXIT_FAILURE;
  
}
