#pragma once

#include <cstdint>

#include "color.hpp"
#include "point2d.hpp"

class Agent {
   public:
    Agent(const char id, Point2D position, const Color color) : id_(id), position_(position), color_(color) {}
    Agent(const char id, uint16_t x, uint16_t y, const Color color) : id_(id), position_(x, y), color_(color) {}

    Agent(const Agent &) = default;
    Agent &operator=(const Agent &) = default;
    ~Agent() = default;

    bool operator==(const Agent &other) const { return id_ == other.id_ && position_ == other.position_ && color_ == other.color_; }
    bool operator!=(const Agent &other) const { return !(*this == other); }

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