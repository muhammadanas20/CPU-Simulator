// ============================================================================
// Stack.h  -  Phase 4
// Fixed-capacity, array-based LIFO stack.
//
//   index:  0     1     2    ...  top_
//         [10]  [20]  [30]        ^ last pushed = first popped
//
// A fixed capacity is deliberate: a real CPU stack region has a limited size,
// so we can demonstrate STACK OVERFLOW as well as STACK UNDERFLOW.
// Used for: CPU PUSH/POP, and the Undo / Redo stacks of the editor.
// All operations are O(1).
// ============================================================================
#pragma once
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

class StackOverflowError : public std::runtime_error {
public: explicit StackOverflowError(const std::string& m) : std::runtime_error(m) {}
};
class StackUnderflowError : public std::runtime_error {
public: explicit StackUnderflowError(const std::string& m) : std::runtime_error(m) {}
};

template <typename T>
class Stack {
private:
    T* data_;
    std::size_t capacity_;
    std::size_t count_ = 0;          // top element lives at data_[count_-1]
    std::size_t operations_ = 0;     // push/pop counter for statistics

public:
    explicit Stack(std::size_t capacity = 256)
        : data_(new T[capacity < 1 ? 1 : capacity]), capacity_(capacity < 1 ? 1 : capacity) {}
    Stack(const Stack& o) : data_(new T[o.capacity_]), capacity_(o.capacity_), count_(o.count_) {
        for (std::size_t i = 0; i < count_; ++i) data_[i] = o.data_[i];
    }
    Stack& operator=(Stack o) {
        std::swap(data_, o.data_);
        std::swap(capacity_, o.capacity_);
        std::swap(count_, o.count_);
        return *this;
    }
    ~Stack() { delete[] data_; }

    void push(const T& value) {
        if (isFull())
            throw StackOverflowError("Stack overflow: capacity " + std::to_string(capacity_) +
                                     " reached");
        data_[count_++] = value;
        ++operations_;
    }
    T pop() {
        if (isEmpty()) throw StackUnderflowError("Stack underflow: pop from empty stack");
        ++operations_;
        return std::move(data_[--count_]);
    }
    const T& peek() const {
        if (isEmpty()) throw StackUnderflowError("Stack underflow: peek on empty stack");
        return data_[count_ - 1];
    }
    bool isEmpty() const { return count_ == 0; }
    bool isFull() const { return count_ == capacity_; }
    std::size_t size() const { return count_; }
    std::size_t capacity() const { return capacity_; }
    std::size_t operations() const { return operations_; }
    void clear() { count_ = 0; }

    // Element `depth` positions below the top (0 = top). For visualisation.
    const T& fromTop(std::size_t depth) const {
        if (depth >= count_) throw std::out_of_range("Stack::fromTop: depth out of range");
        return data_[count_ - 1 - depth];
    }

    void display(std::ostream& os = std::cout) const {
        os << "Stack  size=" << count_ << "/" << capacity_ << "\n";
        if (isEmpty()) { os << "  TOP -> (empty)\n"; return; }
        os << "  TOP\n   |\n   v\n";
        for (std::size_t d = 0; d < count_; ++d) os << "  [ " << fromTop(d) << " ]\n";
        os << "  BOTTOM\n";
    }
};
