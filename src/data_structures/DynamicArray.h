// ============================================================================
// DynamicArray.h  -  Phase 2
// A hand-written growable array (our own replacement for std::vector).
//
// Idea: keep a raw heap block `data_` of `capacity_` slots, of which the first
// `size_` are in use. When the block is full, allocate a block TWICE as big,
// copy the elements across and free the old one ("doubling strategy").
// Doubling makes pushBack amortized O(1): n pushes cause at most
// 1 + 2 + 4 + ... + n < 2n element copies in total.
//
// Used for: program source lines, the assembled instruction list,
//           simulated memory, and temporary arrays for sorting/searching.
// ============================================================================
#pragma once
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

template <typename T>
class DynamicArray {
private:
    T* data_;
    std::size_t size_;
    std::size_t capacity_;
    std::size_t resizeCount_ = 0;   // how many times we re-allocated (for demos)

    // Allocate a new block of newCapacity slots and move elements into it. O(n)
    void resize(std::size_t newCapacity) {
        if (newCapacity < 1) newCapacity = 1;
        T* newData = new T[newCapacity];
        for (std::size_t i = 0; i < size_; ++i) newData[i] = std::move(data_[i]);
        delete[] data_;
        data_ = newData;
        capacity_ = newCapacity;
        ++resizeCount_;
    }

    void checkIndex(std::size_t index, const char* op) const {
        if (index >= size_)
            throw std::out_of_range(std::string("DynamicArray::") + op + ": index " +
                                    std::to_string(index) + " out of range (size " +
                                    std::to_string(size_) + ")");
    }

public:
    DynamicArray() : DynamicArray(4) {}
    explicit DynamicArray(std::size_t initialCapacity)
        : data_(new T[initialCapacity < 1 ? 1 : initialCapacity]), size_(0),
          capacity_(initialCapacity < 1 ? 1 : initialCapacity) {}

    // --- Rule of five: deep copy, so two arrays never share one heap block ---
    DynamicArray(const DynamicArray& other)
        : data_(new T[other.capacity_]), size_(other.size_), capacity_(other.capacity_) {
        for (std::size_t i = 0; i < size_; ++i) data_[i] = other.data_[i];
    }
    DynamicArray(DynamicArray&& other) noexcept
        : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
        other.data_ = new T[1];
        other.size_ = 0;
        other.capacity_ = 1;
    }
    DynamicArray& operator=(DynamicArray other) {   // copy-and-swap idiom
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
        return *this;
    }
    ~DynamicArray() { delete[] data_; }

    // ---------------------------- basic info ---------------------------------
    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    std::size_t resizeCount() const { return resizeCount_; }
    bool empty() const { return size_ == 0; }

    // ------------------------------ insert -----------------------------------
    // Append at the end. Amortized O(1).
    void pushBack(const T& value) {
        if (size_ == capacity_) resize(capacity_ * 2);
        data_[size_++] = value;
    }

    // Insert at position `index` (0..size), shifting the tail right. O(n)
    void insert(std::size_t index, const T& value) {
        if (index > size_)
            throw std::out_of_range("DynamicArray::insert: index " + std::to_string(index) +
                                    " out of range (size " + std::to_string(size_) + ")");
        if (size_ == capacity_) resize(capacity_ * 2);
        for (std::size_t i = size_; i > index; --i) data_[i] = std::move(data_[i - 1]);
        data_[index] = value;
        ++size_;
    }

    // ------------------------------ remove -----------------------------------
    // Remove element at `index`, shifting the tail left. O(n)
    // Shrinks to half capacity when only a quarter is used (avoids thrashing).
    void removeAt(std::size_t index) {
        checkIndex(index, "removeAt");
        for (std::size_t i = index; i + 1 < size_; ++i) data_[i] = std::move(data_[i + 1]);
        --size_;
        if (capacity_ > 8 && size_ <= capacity_ / 4) resize(capacity_ / 2);
    }

    void popBack() {
        if (size_ == 0) throw std::out_of_range("DynamicArray::popBack: array is empty");
        removeAt(size_ - 1);
    }

    // --------------------------- update / get --------------------------------
    void update(std::size_t index, const T& value) {   // O(1)
        checkIndex(index, "update");
        data_[index] = value;
    }
    T& get(std::size_t index) {                          // O(1)
        checkIndex(index, "get");
        return data_[index];
    }
    const T& get(std::size_t index) const {
        checkIndex(index, "get");
        return data_[index];
    }
    T& operator[](std::size_t index) { return get(index); }
    const T& operator[](std::size_t index) const { return get(index); }

    void swapElements(std::size_t i, std::size_t j) {
        checkIndex(i, "swap");
        checkIndex(j, "swap");
        std::swap(data_[i], data_[j]);
    }

    // Remove everything (keeps the current block).
    void clear() { size_ = 0; }

    // Pre-fill with n copies of value (used by Memory).
    void assign(std::size_t n, const T& value) {
        clear();
        if (n > capacity_) resize(n);
        for (std::size_t i = 0; i < n; ++i) data_[size_++] = value;
    }

    // ------------------------------ search -----------------------------------
    // Linear search for an equal value. Returns index or -1. O(n)
    long search(const T& value) const {
        for (std::size_t i = 0; i < size_; ++i)
            if (data_[i] == value) return static_cast<long>(i);
        return -1;
    }

    // ------------------------------ display ----------------------------------
    // Prints   index | value   plus size/capacity. Requires operator<< for T.
    void display(std::ostream& os = std::cout) const {
        os << "DynamicArray  size=" << size_ << "  capacity=" << capacity_
           << "  (resized " << resizeCount_ << " times)\n";
        for (std::size_t i = 0; i < capacity_; ++i) {
            os << "  [" << i << "] ";
            if (i < size_) os << data_[i] << "\n";
            else os << "<unused>\n";
            if (i >= size_ && i >= size_ + 3 && i + 1 < capacity_) {
                os << "  ... " << (capacity_ - i - 1) << " more unused slots\n";
                break;
            }
        }
    }
};
