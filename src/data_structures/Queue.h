// ============================================================================
// Queue.h  -  Phase 5
// Linked-list based FIFO queue with front and rear pointers.
//
//   FRONT -> [A] -> [B] -> [C] <- REAR
//   dequeue removes A (front), enqueue attaches after C (rear).
//
// Both ends are O(1) and the queue never becomes "full".
// Used for: the CPU's Instruction Queue (a prefetch queue). The CPU always
// DEQUEUES the next instruction to execute. When a jump is taken, the queue
// is flushed and refilled from the jump target - the same idea as a real
// processor's prefetch queue / pipeline flush.
// ============================================================================
#pragma once
#include <cstddef>
#include <stdexcept>
#include <string>

class QueueUnderflowError : public std::runtime_error {
public: explicit QueueUnderflowError(const std::string& m) : std::runtime_error(m) {}
};

template <typename T>
class Queue {
private:
    struct Node { T data; Node* next; explicit Node(const T& d) : data(d), next(nullptr) {} };
    Node* front_ = nullptr;
    Node* rear_ = nullptr;
    std::size_t size_ = 0;
    std::size_t operations_ = 0;

public:
    Queue() = default;
    Queue(const Queue& o) { for (Node* n = o.front_; n; n = n->next) enqueue(n->data); operations_ = o.operations_; }
    Queue& operator=(const Queue& o) {
        if (this != &o) { clear(); for (Node* n = o.front_; n; n = n->next) enqueue(n->data); }
        return *this;
    }
    ~Queue() { clear(); }

    void enqueue(const T& value) {                   // O(1)
        Node* n = new Node(value);
        if (!rear_) front_ = rear_ = n;
        else { rear_->next = n; rear_ = n; }
        ++size_;
        ++operations_;
    }
    T dequeue() {                                    // O(1)
        if (isEmpty()) throw QueueUnderflowError("Queue underflow: dequeue from empty queue");
        Node* n = front_;
        T value = n->data;
        front_ = front_->next;
        if (!front_) rear_ = nullptr;
        delete n;
        --size_;
        ++operations_;
        return value;
    }
    const T& peek() const {                          // O(1)
        if (isEmpty()) throw QueueUnderflowError("Queue underflow: peek on empty queue");
        return front_->data;
    }
    bool isEmpty() const { return size_ == 0; }
    std::size_t size() const { return size_; }
    std::size_t operations() const { return operations_; }
    void clear() {
        while (front_) { Node* n = front_->next; delete front_; front_ = n; }
        rear_ = nullptr;
        size_ = 0;
    }
    template <typename Fn>
    void forEach(Fn fn) const { for (Node* n = front_; n; n = n->next) fn(n->data); }
};
