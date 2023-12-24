#pragma once

#include <pthread.h>
#include <vector>
#include "set.hpp"

template <typename T>
class OptimisticSet : public Set<T> {
private:
    class Node{
    private:
        pthread_mutex_t _mutex;
    public:
        T item;
        size_t key;
        Node *next;
        Node(const T &i, size_t k, Node *n)  : _mutex(PTHREAD_MUTEX_INITIALIZER), item(i), key(k), next(n) {};
        void lock() {pthread_mutex_lock(&_mutex);}
        void unlock() {pthread_mutex_unlock(&_mutex);}
        ~Node() {pthread_mutex_destroy(&_mutex); }
    };
    Node *_head;
    std::vector<Node*> _nodes;
    pthread_mutex_t _mutex;
    
    bool _validate(Node *pred, Node *curr) {
        Node *node = _head;
        while (node->key <= pred->key) {
            if (node == pred)
                return pred->next == curr;
            node = node->next;
        }
        return false;
    }
    
    void _find(Node* &pred, Node* &curr, size_t key) {
        while (curr->key < key){
            pred = curr;
            curr = curr->next;
        }
    }
public:
    OptimisticSet()   : _mutex(PTHREAD_MUTEX_INITIALIZER) {
        Node *next = new Node(T(), SIZE_MAX, nullptr);
        _head = new Node(T(), 0, next);
        _nodes.push_back(_head);
        _nodes.push_back(next);
    }

    bool add(const T &item) override {
        size_t key = std::hash<T>()(item);
        while (true){
            Node *pred = _head, *curr = pred->next;
            _find(pred, curr, key);
            pred->lock();
            curr->lock();
            if (_validate(pred, curr)){
                if (curr->key == key){
                    pred->unlock();
                    curr->unlock();
                    return false;
                } else {
                    Node *node = new Node(item, key, curr);
                    pred->next = node;
                    pthread_mutex_lock(&_mutex);
                    _nodes.push_back(node);
                    pthread_mutex_unlock(&_mutex);
                    pred->unlock();
                    curr->unlock();
                    return true;
                }
            } else {
                pred->unlock();
                curr->unlock();
            }
        }
    }
    
    bool remove(const T &item) override {
        size_t key = std::hash<T>()(item);
        while (true){
            Node *pred = _head, *curr = pred->next;
            _find(pred, curr, key);
            pred->lock();
            curr->lock();
            if (_validate(pred, curr)){
                if (curr->key == key){
                    pred->next = curr->next;
                    pred->unlock();
                    curr->unlock();
                    return true;
                } else {
                    pred->unlock();
                    curr->unlock();
                    return false;
                }
            } else {
                pred->unlock();
                curr->unlock();
            }
        }
    }

    bool contains(const T &item) override {
        size_t key = std::hash<T>()(item);
        while (true){
            Node *pred = _head, *curr = pred->next;
            _find(pred, curr, key);
            pred->lock();
            curr->lock();
            if (_validate(pred, curr)){
                pred->unlock();
                curr->unlock();
                return curr->key == key;
            } else {
                pred->unlock();
                curr->unlock();
            }
        }
    }
    
    bool empty() override {
        return _head->key == 0 && _head->next->key == SIZE_MAX;
    }

    ~OptimisticSet() {
        for (Node *node : _nodes)
            delete node;
        pthread_mutex_destroy(&_mutex);
    }
};

