#pragma once

#include <cstdint>
#include <vector>

#include "color.hpp"
#include "point2d.hpp"

class Box {
   public:
    Box(char id, int x_col, int y_row, Color color) : id_(id), position_(x_col, y_row), color_(color) {}
    Box(char id, Point2D position, Color color) : id_(id), position_(position), color_(color) {}

    Box(const Box &) = default;
    Box &operator=(const Box &) = delete;
    ~Box() = default;

    bool operator==(const Box &other) const { return id_ == other.id_ && position_ == other.position_ && color_ == other.color_; }
    bool operator!=(const Box &other) const { return !(*this == other); }

    Point2D position() const { return position_; }
    Color color() const { return color_; }

    void moveBy(const Point2D &delta) { position_ += delta; }
    void moveBy(uint16_t dx, uint16_t dy) { position_ += Point2D(dx, dy); }

    bool at(const Point2D &position) const { return position_ == position; }

    char getId() const { return id_; }

   private:
    const char id_;
    Point2D position_;
    const Color color_;
};