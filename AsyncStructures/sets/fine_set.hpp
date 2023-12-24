#pragma once

#include <pthread.h>
#include "set.hpp"

template <typename T>
class FineSet : public Set<T>{
private:
    class Node {
    private:
        pthread_mutex_t _mutex;
    public:
        T item;
        size_t key;
        Node *next;
        Node(const T &i, size_t k, Node *n): _mutex(PTHREAD_MUTEX_INITIALIZER), item(i), key(k), next(n){};
        void lock() {pthread_mutex_lock(&_mutex);}
        void unlock() {pthread_mutex_unlock(&_mutex);}
        ~Node() {pthread_mutex_destroy(&_mutex);}
    };
    Node *_head;
    
    void _find(Node* &pred, Node* &curr, size_t key) {
        while (curr->key < key){
            pred->unlock();
            pred = curr;
            curr = curr->next;
            curr->lock();
        }
    }
    
public:
    FineSet() : _head(new Node(T(), 0, new Node(T(), SIZE_MAX, nullptr))) {};
    
    bool add(const T &item) override {
        size_t key = std::hash<T>()(item);
        _head->lock();
        Node *pred = _head, *curr = pred->next;
        curr->lock();
        _find(pred, curr, key);
        if (curr->key == key) {
            curr->unlock();
            pred->unlock();
            return false;
        }
        Node *newNode = new Node(item, key, curr);
        pred->next = newNode;
        curr->unlock();
        pred->unlock();
        return true;
    }
    
    bool remove(const T &item) override {
        size_t key = std::hash<T>()(item);
        _head->lock();
        Node *pred = _head, *curr = pred->next;
        curr->lock();
        _find(pred, curr, key);
        if (curr->key == key) {
            pred->next = curr->next;
            curr->unlock();
            delete curr;
            pred->unlock();
            return true;
        }
        curr->unlock();
        pred->unlock();
        return false;
    }
    
    bool contains(const T &item) override {
        size_t key = std::hash<T>()(item);
        _head->lock();
        Node *pred = _head, *curr = pred->next;
        curr->lock();
        _find(pred, curr, key);
        if (curr->key == key){
            curr->unlock();
            pred->unlock();
            return true;
        }
        curr->unlock();
        pred->unlock();
        return false;
    }

    bool empty() override {
        return _head->key == 0 && _head->next->key == SIZE_MAX;
    }

    ~FineSet() {
        Node *curr = _head, *next;
        while (curr){
            next = curr->next;
            delete curr;
            curr = next;
        }
    }
};
