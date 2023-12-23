#pragma once

#include <pthread.h>
#include <chrono>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>

#include "sets/fine_set.hpp"
#include "sets/opt_set.hpp"

enum class SetType {
    FINE_LIST,
    OPTIMISTIC_LIST
};

class ThreadInfo{
public:
    Set<int> *set;
    std::vector<int> *data, *array;
    int writers_num, readers_num, records_num, readings_num, index;
    ThreadInfo() {};
    ThreadInfo(Set<int> *s, std::vector<int> *d, std::vector<int> *a, int nWriters, int nReaders, int nRecords, int nReadings, int i)
        : set(s), data(d), array(a), writers_num(nWriters), readers_num(nReaders), records_num(nRecords), readings_num(nReadings), index(i) {};
};

std::vector<ThreadInfo> create_threads_info(int threads_num, Set<int> *set, std::vector<int> *data, std::vector<int> *array, int writers_num, int readers_num, int records_num, int readings_num) {
    std::vector<ThreadInfo> threads_inf(threads_num);
    for (int i = 0; i < threads_num; i++)
        threads_inf[i] = ThreadInfo(set, data, array, writers_num, readers_num, records_num, readings_num, i);
    return threads_inf;
}

std::vector<pthread_t> create_threads(int threads_num, void *(*start_routine) (void *), std::vector<ThreadInfo> &threads_inf){
    std::vector<pthread_t> threads(threads_num);
    for (int i = 0; i < threads_num; i++)
        pthread_create(&threads[i], nullptr, start_routine, &threads_inf[i]);
    return threads;
}

void join_threads(const std::vector<pthread_t> &threads){
    for (auto thread : threads)
        pthread_join(thread, nullptr);
}

void* write(void *arg){
    ThreadInfo *thread_inf = (ThreadInfo*)arg;
    for (int i = 0; i < thread_inf->records_num; i++)
        thread_inf->set->add(thread_inf->data->at(thread_inf->index + i * thread_inf->writers_num));
    return nullptr;
}

void* read(void *arg){
    ThreadInfo *thread_inf = (ThreadInfo*)arg;
    for (int i = 0; i < thread_inf->readings_num; i++){
        while (!thread_inf->set->remove(thread_inf->data->at(thread_inf->index + i * thread_inf->readers_num)))
            sched_yield();
        thread_inf->array->at(thread_inf->data->at(thread_inf->index + i * thread_inf->readers_num)) = 1;
    }
    return nullptr;
}

bool check_writers(Set<int> &set, std::vector<int> &array){
    for (auto i : array)
        if (!set.contains(i))
            return false;
    return true;
}

bool check_readers(Set<int> &set, std::vector<int> &array){
    for (auto i : array)
        if (i != 1)
            return false;
    return set.empty();
}

Set<int>* create_set(SetType set_type){
    if (set_type == SetType::FINE_LIST)
        return new FineSet<int>();
    return new OptimisticSet<int>();
}

std::vector<int> create_data(int n){
    std::vector<int> data(n);
    iota(data.begin(), data.end(), 0);
    return data;
}

void run_write_test(SetType set_type, int writers_num, int records_num, int tests_num){
    bool result = true;
    double timeTotal = 0;
    for (int i = 0; i < tests_num; i++){
        Set<int> *set = create_set(set_type);
        std::vector<int> data = create_data(writers_num * records_num);
        std::vector<ThreadInfo> threads_inf = create_threads_info(writers_num, set, &data, nullptr, writers_num, 0, records_num, 0);
        std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
        std::vector<pthread_t> threads = create_threads(writers_num, write, threads_inf);
        join_threads(threads);
        std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
        result &= check_writers(*set, data);
        timeTotal += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;
        delete set;
    }
    std::cout << "Writers test\n result: " <<(result ? "SUCCESS " : "FAIL ") << " time: " << timeTotal / tests_num << std::endl;
}

void run_read_test(SetType set_type,  int readers_num, int readings_num, int tests_num){
    bool result = true;
    double timeTotal = 0;
    for (int i = 0; i < tests_num; i++){
        Set<int> *set = create_set(set_type);
        std::vector<int> data = create_data( readers_num * readings_num);
        std::vector<int> array = std::vector<int>(readers_num * readings_num, 0);
        std::vector<ThreadInfo> threads_inf = create_threads_info(readers_num, set, &data, &array, 0, readers_num, 0, readings_num);
        for (auto d : data)
            set->add(d);
        std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
        std::vector<pthread_t> threads = create_threads(readers_num, read, threads_inf);
        join_threads(threads);
        std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
        result &= check_readers(*set, array);
        timeTotal += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;
        delete set;
    }
    std::cout << "Readers test\n result: " <<(result ? "SUCCESS " : "FAIL ") << " time: " << timeTotal / tests_num << std::endl;
}

void run_general_test(SetType set_type, int writers_num, int readers_num, int records_num, int readings_num, int tests_num){
    bool result = true;
    double timeTotal = 0;
    for (int i = 0; i < tests_num; i++){
        Set<int> *set = create_set(set_type);
        std::vector<int> data = create_data(writers_num * records_num);
        std::vector<int> array = std::vector<int>(writers_num * records_num, 0);
        std::vector<ThreadInfo> writers_inf = create_threads_info(writers_num, set, &data, nullptr, writers_num, 0, records_num, 0);
        std::vector<ThreadInfo> readers_inf = create_threads_info(readers_num, set, &data, &array, 0, readers_num, 0, readings_num);
        std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();
        std::vector<pthread_t> writers = create_threads(writers_num, write, writers_inf);
        std::vector<pthread_t> readers = create_threads(readers_num, read, readers_inf);
        join_threads(writers);
        join_threads(readers);
        std::chrono::time_point<std::chrono::system_clock> end = std::chrono::system_clock::now();
        result &= check_readers(*set, array);
        timeTotal += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() / 1000.0;
        delete set;
    }
    std::cout << "General test\n result: " <<(result ? "SUCCESS " : "FAIL ") << " time: " << timeTotal / tests_num << std::endl;
}
