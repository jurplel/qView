#include "qvrandom.h"

#include <random>

// Simple reversible random generator
// Build a random permutation R of [0..N-1] and store the actual position of
// each value in positions P. To get the next/previous value of i, get its
// position, increment/decrement it and get its permutation value.
// Next/previous = R[(P[i] ± 1) mod N]

void QVRandom::ensureParamsUpToDate(int n)
{
    if (n <= 0) {
        modN = 0;
        permutation.clear();
        positions.clear();
        return;
    }
    if (n != modN) {
        modN = n;
        permutation.resize(modN);
        positions.resize(modN);
        for (int i = 0; i < modN; i++)
            permutation[i] = i;

        static std::mt19937 rng(std::random_device{}());

        // shuffle permutation array using Fisher-Yates algorithm
        for (int i = modN - 1; i > 0; i--) {
            std::uniform_int_distribution<int> distribute(0, i);
            int j = distribute(rng);
            std::swap(permutation[i], permutation[j]);
        }
        for (int i = 0; i < modN; i++)
            positions[permutation[i]] = i;
    }
}

int QVRandom::nextIndex(int currentIndex) const
{
    if (modN <= 0)
        return currentIndex;
    int idx = currentIndex;
    if (idx < 0)
        idx = 0;
    else if (idx >= modN)
        idx %= modN;
    int k = positions[idx];
    // k' = (k + 1) mod N
    k++;
    if (k == modN) k = 0;
    return permutation[k];
}

int QVRandom::previousIndex(int currentIndex) const
{
    if (modN <= 0)
        return currentIndex;
    int idx = currentIndex;
    if (idx < 0)
        idx = 0;
    else if (idx >= modN)
        idx %= modN;
    int k = positions[idx];
    k--;
    if (k < 0) k = modN - 1;
    return permutation[k];
}
