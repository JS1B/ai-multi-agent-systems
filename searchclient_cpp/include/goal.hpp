#pragma once

#include "point2d.hpp"

class Goal {
   public:
    Goal(char id, int x_col, int y_row) : id_(id), position_(x_col, y_row) {}
    Goal(char id, Point2D position) : id_(id), position_(position) {}

    char id() const { return id_; }
    Point2D position() const { return position_; }

    bool operator==(const Goal &other) const { return id_ == other.id_ && position_ == other.position_; }
    bool operator!=(const Goal &other) const { return !(*this == other); }

    bool at(Point2D position) const { return position_ == position; }

   private:
    const char id_;
    const Point2D position_;
};