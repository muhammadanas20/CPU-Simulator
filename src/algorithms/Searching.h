// ============================================================================
// Searching.h  -  Phase 7
// Linear search  : works on ANY order.            O(n)
// Binary search  : requires data SORTED by the key being searched.  O(log n)
//   Each comparison with the middle element discards half the range; that
//   decision is only valid if everything left of mid is <= and everything
//   right of mid is >= the middle key - i.e. the data is sorted.
// Both return the index (or -1) plus the number of comparisons performed so
// the UI can show the performance difference.
// ============================================================================
#pragma once
#include "../data_structures/DynamicArray.h"

struct SearchResult {
    long index = -1;
    long comparisons = 0;
};

namespace Searching {

template <typename T, typename Pred>
SearchResult linearSearch(const DynamicArray<T>& arr, Pred matches) {
    SearchResult r;
    for (std::size_t i = 0; i < arr.size(); ++i) {
        ++r.comparisons;
        if (matches(arr[i])) { r.index = static_cast<long>(i); return r; }
    }
    return r;
}

// keyOf(element) must return something comparable with `target` via < and ==.
// PRECONDITION: arr is sorted ascending by keyOf.
template <typename T, typename K, typename KeyFn>
SearchResult binarySearch(const DynamicArray<T>& arr, const K& target, KeyFn keyOf) {
    SearchResult r;
    long low = 0, high = static_cast<long>(arr.size()) - 1;
    while (low <= high) {
        long mid = low + (high - low) / 2;      // avoids (low+high) overflow
        const auto& key = keyOf(arr[mid]);
        ++r.comparisons;
        if (key == target) { r.index = mid; return r; }
        if (key < target) low = mid + 1;         // target is in the right half
        else high = mid - 1;                     // target is in the left half
    }
    return r;
}

template <typename T, typename KeyFn>
bool isSorted(const DynamicArray<T>& arr, KeyFn keyOf) {
    for (std::size_t i = 1; i < arr.size(); ++i)
        if (keyOf(arr[i]) < keyOf(arr[i - 1])) return false;
    return true;
}

}  // namespace Searching
