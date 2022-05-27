#pragma once

#include <vector>
#include <iostream>
#include <cassert>

template<typename Iterator>
class IteratorRange {
public:
    explicit IteratorRange(Iterator begin, Iterator end, size_t size) : begin_(begin), end_(end), size_(size) {
    }

    Iterator begin() const {
        return begin_;
    }

    size_t size() const {
        return size_;
    }

    Iterator end() const {
        return end_;
    }

private:
    Iterator begin_;
    Iterator end_;
    size_t size_;
};

template<typename Iterator>
class Paginator {
public:
    explicit Paginator(Iterator begin, Iterator end, size_t size) {
        assert(end >= begin && size > 0);
        for (auto page_begin_it = begin; page_begin_it != end;) {
            if (distance(page_begin_it, end) >= size) {
                Iterator page_end_it = next(page_begin_it, size);
                pages_.push_back(IteratorRange(page_begin_it, page_end_it, size));
                page_begin_it = page_end_it;
            } else {
                Iterator page_end_it = next(page_begin_it, distance(page_begin_it, end));
                pages_.push_back(IteratorRange(page_begin_it, page_end_it, distance(page_begin_it, end)));
                break;
            }
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template<typename Iterator>
std::ostream& operator<<(std::ostream& out, const IteratorRange<Iterator> page) {
    for (Iterator it = page.begin(); it != page.end(); ++it) {
        out << *it;
    }
    return out;
}

template<typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(c.begin(), c.end(), page_size);
}
