// ============================================================================
// Sorting.h  -  Phase 8
// Three classic O(n^2) comparison sorts, each counting comparisons and swaps.
// `less(a,b)` returns true when a must come before b.
//
//  Algorithm   Best     Average  Worst   Extra space  Stable
//  Bubble      O(n)*    O(n^2)   O(n^2)  O(1)         yes   (*with early exit)
//  Selection   O(n^2)   O(n^2)   O(n^2)  O(1)         no
//  Insertion   O(n)     O(n^2)   O(n^2)  O(1)         yes
// ============================================================================
#pragma once
#include <string>
#include "../data_structures/DynamicArray.h"

struct SortStats {
    std::string algorithm;
    std::string complexity;
    long comparisons = 0;
    long swaps = 0;       // for insertion sort: element shifts
};

namespace Sorting {

// Repeatedly swap adjacent out-of-order pairs; largest "bubbles" to the end.
template <typename T, typename Less>
SortStats bubbleSort(DynamicArray<T>& a, Less less) {
    SortStats s{"Bubble Sort", "Best O(n), Average/Worst O(n^2), Space O(1)"};
    std::size_t n = a.size();
    for (std::size_t pass = 0; pass + 1 < n; ++pass) {
        bool swapped = false;
        for (std::size_t j = 0; j + 1 < n - pass; ++j) {
            ++s.comparisons;
            if (less(a[j + 1], a[j])) { a.swapElements(j, j + 1); ++s.swaps; swapped = true; }
        }
        if (!swapped) break;   // already sorted -> stop early
    }
    return s;
}

// Find the minimum of the unsorted part and swap it into place.
template <typename T, typename Less>
SortStats selectionSort(DynamicArray<T>& a, Less less) {
    SortStats s{"Selection Sort", "Best/Average/Worst O(n^2), Space O(1)"};
    std::size_t n = a.size();
    for (std::size_t i = 0; i + 1 < n; ++i) {
        std::size_t minIdx = i;
        for (std::size_t j = i + 1; j < n; ++j) {
            ++s.comparisons;
            if (less(a[j], a[minIdx])) minIdx = j;
        }
        if (minIdx != i) { a.swapElements(i, minIdx); ++s.swaps; }
    }
    return s;
}

// Take the next element and shift larger ones right until its spot is found.
template <typename T, typename Less>
SortStats insertionSort(DynamicArray<T>& a, Less less) {
    SortStats s{"Insertion Sort", "Best O(n), Average/Worst O(n^2), Space O(1)"};
    for (std::size_t i = 1; i < a.size(); ++i) {
        T key = a[i];
        std::size_t j = i;
        while (j > 0) {
            ++s.comparisons;
            if (!less(key, a[j - 1])) break;
            a[j] = a[j - 1];     // shift right
            ++s.swaps;
            --j;
        }
        a[j] = key;
    }
    return s;
}

}  // namespace Sorting
