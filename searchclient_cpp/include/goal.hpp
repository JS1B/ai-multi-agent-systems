#pragma once

#include "point2d.hpp"

class Goal {
   public:
    Goal(char id, Point2D position) : id_(id), position_(position) {}
    Goal(char id, uint16_t row, uint16_t col) : id_(id), position_(row, col) {}

    char id() const { return id_; }
    Point2D position() const { return position_; }

    bool operator==(const Goal &other) const { return id_ == other.id_; }
    bool operator!=(const Goal &other) const { return !(*this == other); }

    bool at(Point2D position) const { return position_ == position; }

   private:
    const char id_;
    const Point2D position_;
};