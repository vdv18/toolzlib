#ifndef TOOLZ_ALGORITHM_H
#define TOOLZ_ALGORITHM_H

#include <algorithm>

// Function like binary_search, but returns iterator, not bool
template <class ForwardIterator, class T>
ForwardIterator binary_find(ForwardIterator first,
    ForwardIterator last, const T& value)
{
    first = std::lower_bound(first, last, value);
    if (first!=last && !(value < *first))
        return first; //found
    return last;
};

// Function like binary_search, but returns iterator, not bool
template <class ForwardIterator, class T, class Compare>
ForwardIterator binary_find(ForwardIterator first,
    ForwardIterator last, const T& value, Compare comp)
{
    first = std::lower_bound(first, last, value, comp);
    if (first!=last && !(value < *first))
        return first; //found
    return last;
};

#endif

