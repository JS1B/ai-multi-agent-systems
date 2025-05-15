#pragma once

#include <cstdint>

#include "helpers.hpp"

class Point2D {
   public:
    Point2D() : x_(0), y_(0) {}
    Point2D(int16_t x, int16_t y) : x_(x), y_(y) {}
    Point2D(const Point2D &) = default;
    Point2D &operator=(const Point2D &) = default;
    ~Point2D() = default;

    bool operator==(const Point2D &other) const { return x_ == other.x_ && y_ == other.y_; }
    bool operator!=(const Point2D &other) const { return !(*this == other); }

    // Add operator< for std::map compatibility
    bool operator<(const Point2D &other) const {
        if (y_ < other.y_) return true;
        if (y_ > other.y_) return false;
        return x_ < other.x_;
    }

    Point2D &operator+=(const Point2D &other) {
        x_ += other.x_;
        y_ += other.y_;
        return *this;
    }
    Point2D &operator-=(const Point2D &other) {
        x_ -= other.x_;
        y_ -= other.y_;
        return *this;
    }

    Point2D operator+(const Point2D &other) const { return Point2D(x_ + other.x_, y_ + other.y_); }
    Point2D operator-(const Point2D &other) const { return Point2D(x_ - other.x_, y_ - other.y_); }

    inline int16_t x() const { return x_; }
    inline int16_t y() const { return y_; }

   private:
    int16_t x_;
    int16_t y_;
};

namespace std {
template <>
struct hash<Point2D> {
    size_t operator()(const Point2D &p) const {
        size_t seed = 0;
        // Combine hashes of x_ and y_
        utils::hashCombine(seed, p.x());
        utils::hashCombine(seed, p.y());
        return seed;
    }
};
}  // namespace std