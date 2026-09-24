// ============================================================================
// HashTable.h  -  Phase 6
// String-keyed hash table using SEPARATE CHAINING.
//
//   bucket[0] -> NULL
//   bucket[1] -> (LOOP,2) -> (END,7) -> NULL      <- collision: 2 keys, 1 bucket
//   bucket[2] -> (START,0) -> NULL
//
// hash(key) = djb2(key) % bucketCount. Each bucket is the head of a small
// singly linked chain. Average search/insert/delete is O(1 + load factor);
// worst case (everything in one bucket) is O(n).
// When size > bucketCount (load factor > 1) we REHASH into ~2x buckets.
// Used for: the Symbol Table  (label -> instruction address).
// ============================================================================
#pragma once
#include <cstddef>
#include <iostream>
#include <string>

template <typename V>
class HashTable {
public:
    struct Node {
        std::string key;
        V value;
        Node* next;
        Node(const std::string& k, const V& v, Node* n) : key(k), value(v), next(n) {}
    };

private:
    Node** buckets_;
    std::size_t bucketCount_;
    std::size_t size_ = 0;
    std::size_t collisions_ = 0;   // inserts that landed in a non-empty bucket

    static std::size_t djb2(const std::string& s) {
        std::size_t h = 5381;
        for (unsigned char c : s) h = h * 33 + c;
        return h;
    }
    void allocate(std::size_t n) {
        bucketCount_ = n;
        buckets_ = new Node*[n];
        for (std::size_t i = 0; i < n; ++i) buckets_[i] = nullptr;
    }
    void freeAll() {
        for (std::size_t i = 0; i < bucketCount_; ++i) {
            Node* n = buckets_[i];
            while (n) { Node* nx = n->next; delete n; n = nx; }
        }
        delete[] buckets_;
    }
    void rehash() {
        Node** old = buckets_;
        std::size_t oldCount = bucketCount_;
        std::size_t savedCollisions = collisions_;   // re-inserting is not a new collision
        allocate(oldCount * 2 + 1);
        size_ = 0;
        for (std::size_t i = 0; i < oldCount; ++i) {
            Node* n = old[i];
            while (n) { Node* nx = n->next; insertNoGrow(n->key, n->value); delete n; n = nx; }
        }
        delete[] old;
        collisions_ = savedCollisions;
    }
    bool insertNoGrow(const std::string& key, const V& value) {
        std::size_t b = hash(key);
        Node* tail = nullptr;
        for (Node* n = buckets_[b]; n; n = n->next) {
            if (n->key == key) return false;   // duplicate key
            tail = n;
        }
        if (buckets_[b]) ++collisions_;
        Node* node = new Node(key, value, nullptr);
        if (tail) tail->next = node; else buckets_[b] = node;   // append at chain end
        ++size_;
        return true;
    }

public:
    explicit HashTable(std::size_t buckets = 11) { allocate(buckets < 1 ? 1 : buckets); }
    HashTable(const HashTable& o) {
        allocate(o.bucketCount_);
        for (std::size_t i = 0; i < o.bucketCount_; ++i)
            for (Node* n = o.buckets_[i]; n; n = n->next) insertNoGrow(n->key, n->value);
        collisions_ = o.collisions_;
    }
    HashTable& operator=(const HashTable& o) {
        if (this != &o) {
            freeAll();
            allocate(o.bucketCount_);
            size_ = 0;
            for (std::size_t i = 0; i < o.bucketCount_; ++i)
                for (Node* n = o.buckets_[i]; n; n = n->next) insertNoGrow(n->key, n->value);
            collisions_ = o.collisions_;
        }
        return *this;
    }
    ~HashTable() { freeAll(); }

    std::size_t hash(const std::string& key) const { return djb2(key) % bucketCount_; }

    // Returns false (and changes nothing) if the key already exists.
    bool insert(const std::string& key, const V& value) {
        if (size_ + 1 > bucketCount_) rehash();
        return insertNoGrow(key, value);
    }
    bool search(const std::string& key, V& out) const {
        for (Node* n = buckets_[hash(key)]; n; n = n->next)
            if (n->key == key) { out = n->value; return true; }
        return false;
    }
    bool contains(const std::string& key) const { V tmp{}; return search(key, tmp); }
    bool remove(const std::string& key) {
        std::size_t b = hash(key);
        Node* prev = nullptr;
        for (Node* n = buckets_[b]; n; prev = n, n = n->next) {
            if (n->key == key) {
                if (prev) prev->next = n->next; else buckets_[b] = n->next;
                delete n;
                --size_;
                return true;
            }
        }
        return false;
    }
    void clear() {
        freeAll();
        allocate(bucketCount_);
        size_ = 0;
        collisions_ = 0;
    }

    std::size_t size() const { return size_; }
    std::size_t bucketCount() const { return bucketCount_; }
    std::size_t collisions() const { return collisions_; }
    double loadFactor() const { return double(size_) / double(bucketCount_); }
    Node* bucket(std::size_t i) const { return buckets_[i]; }

    template <typename Fn>
    void forEach(Fn fn) const {
        for (std::size_t i = 0; i < bucketCount_; ++i)
            for (Node* n = buckets_[i]; n; n = n->next) fn(n->key, n->value);
    }

    void display(std::ostream& os = std::cout) const {
        os << "HashTable  entries=" << size_ << "  buckets=" << bucketCount_
           << "  load=" << loadFactor() << "  collisions=" << collisions_ << "\n";
        for (std::size_t i = 0; i < bucketCount_; ++i) {
            os << "  Bucket " << i << (i < 10 ? " " : "") << " -> ";
            for (Node* n = buckets_[i]; n; n = n->next) os << n->key << "(" << n->value << ") -> ";
            os << "NULL\n";
        }
    }
};
