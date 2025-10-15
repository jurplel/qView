#ifndef QVRANDOM_H
#define QVRANDOM_H

#include <QtCore/QVector>

class QVRandom
{
public:
    void ensureParamsUpToDate(int n);
    int nextIndex(int currentIndex) const;
    int previousIndex(int currentIndex) const;

private:
    int modN{ 0 };
    QVector<int> permutation;
    QVector<int> positions;
};

#endif // QVRANDOM_H
