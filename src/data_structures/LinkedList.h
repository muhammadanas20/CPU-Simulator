// ============================================================================
// LinkedList.h  -  Phase 3
// A singly linked list with head AND tail pointers.
//
//   head -> [data|next] -> [data|next] -> [data|next] -> NULL
//                                          ^ tail
//
// Keeping a tail pointer makes pushBack O(1) instead of O(n).
// Used for: the Program Repository (ProgramRecord nodes) and the
//           Execution History (HistoryEntry nodes).
// Why a list there? Both grow one item at a time, items are removed from the
// middle (repository delete), and we mostly traverse front-to-back.
// ============================================================================
#pragma once
#include <cstddef>
#include <stdexcept>
#include <string>
#include "DynamicArray.h"

template <typename T>
struct ListNode {
    T data;
    ListNode* next;
    explicit ListNode(const T& d) : data(d), next(nullptr) {}
};

template <typename T>
class LinkedList {
private:
    ListNode<T>* head_ = nullptr;
    ListNode<T>* tail_ = nullptr;
    std::size_t size_ = 0;

    ListNode<T>* nodeAt(std::size_t index) const {   // O(n) walk
        if (index >= size_)
            throw std::out_of_range("LinkedList: index " + std::to_string(index) +
                                    " out of range (size " + std::to_string(size_) + ")");
        ListNode<T>* cur = head_;
        for (std::size_t i = 0; i < index; ++i) cur = cur->next;
        return cur;
    }

public:
    LinkedList() = default;
    LinkedList(const LinkedList& other) {
        for (auto* n = other.head_; n; n = n->next) pushBack(n->data);
    }
    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            clear();
            for (auto* n = other.head_; n; n = n->next) pushBack(n->data);
        }
        return *this;
    }
    ~LinkedList() { clear(); }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    ListNode<T>* head() const { return head_; }   // read-only walking for visualisation

    // --------------------------- insert --------------------------------------
    void pushFront(const T& value) {                 // O(1)
        auto* node = new ListNode<T>(value);
        node->next = head_;
        head_ = node;
        if (!tail_) tail_ = node;
        ++size_;
    }
    void pushBack(const T& value) {                  // O(1) thanks to tail_
        auto* node = new ListNode<T>(value);
        if (!head_) head_ = tail_ = node;
        else { tail_->next = node; tail_ = node; }
        ++size_;
    }
    // Insert so that the new node ends up at position `index` (0..size). O(n)
    void insertAt(std::size_t index, const T& value) {
        if (index > size_) throw std::out_of_range("LinkedList::insertAt: bad index");
        if (index == 0) { pushFront(value); return; }
        if (index == size_) { pushBack(value); return; }
        ListNode<T>* prev = nodeAt(index - 1);
        auto* node = new ListNode<T>(value);
        node->next = prev->next;
        prev->next = node;
        ++size_;
    }

    // --------------------------- delete --------------------------------------
    void removeAt(std::size_t index) {               // O(n)
        if (index >= size_) throw std::out_of_range("LinkedList::removeAt: bad index");
        ListNode<T>* victim;
        if (index == 0) {
            victim = head_;
            head_ = head_->next;
            if (!head_) tail_ = nullptr;
        } else {
            ListNode<T>* prev = nodeAt(index - 1);
            victim = prev->next;
            prev->next = victim->next;
            if (victim == tail_) tail_ = prev;
        }
        delete victim;
        --size_;
    }
    // Delete the first node that satisfies pred. O(n)
    template <typename Pred>
    bool removeIf(Pred pred) {
        ListNode<T>* prev = nullptr;
        for (ListNode<T>* cur = head_; cur; prev = cur, cur = cur->next) {
            if (pred(cur->data)) {
                if (prev) prev->next = cur->next; else head_ = cur->next;
                if (cur == tail_) tail_ = prev;
                delete cur;
                --size_;
                return true;
            }
        }
        return false;
    }
    void clear() {
        while (head_) { auto* n = head_->next; delete head_; head_ = n; }
        tail_ = nullptr;
        size_ = 0;
    }

    // --------------------------- search / update -----------------------------
    // Linear search: pointer to the first matching element or nullptr. O(n)
    template <typename Pred>
    T* find(Pred pred, long* comparisons = nullptr) {
        for (auto* n = head_; n; n = n->next) {
            if (comparisons) ++*comparisons;
            if (pred(n->data)) return &n->data;
        }
        return nullptr;
    }
    template <typename Pred>
    long indexOf(Pred pred) const {
        long i = 0;
        for (auto* n = head_; n; n = n->next, ++i) if (pred(n->data)) return i;
        return -1;
    }
    T& get(std::size_t index) { return nodeAt(index)->data; }
    const T& get(std::size_t index) const { return nodeAt(index)->data; }
    void update(std::size_t index, const T& value) { nodeAt(index)->data = value; }

    // --------------------------- traversal ------------------------------------
    template <typename Fn>
    void forEach(Fn fn) const { for (auto* n = head_; n; n = n->next) fn(n->data); }

    // Copy into / rebuild from a DynamicArray (used by sorting: sort the array
    // with Bubble/Selection/Insertion, then relink the list in that order).
    void toArray(DynamicArray<T>& out) const {
        out.clear();
        for (auto* n = head_; n; n = n->next) out.pushBack(n->data);
    }
    void fromArray(const DynamicArray<T>& in) {
        clear();
        for (std::size_t i = 0; i < in.size(); ++i) pushBack(in[i]);
    }
};
