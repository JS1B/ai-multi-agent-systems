#pragma once

#include "cell2d.hpp"
#include "color.hpp"

class Entity {
   public:
    Entity(const Cell2D &position, const Color &color, char symbol) : position_(position), color_(color), symbol_(symbol) {}
    const Cell2D &getPosition() const { return position_; }
    const Color &getColor() const { return color_; }
    char getSymbol() const { return symbol_; }

    void moveBy(const Cell2D &delta) { position_ += delta; }

   private:
    Cell2D position_;
    Color color_;
    char symbol_;
};