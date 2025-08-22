#pragma once

#include <cassert>
#include <vector>

#include "cell2d.hpp"
#include "chargrid.hpp"
#include "color.hpp"

class BoxBulk {
   private:
    std::vector<Cell2D> positions_;
    std::vector<Cell2D> goals_positions_;  // if any
    const Color color_;
    const char symbol_;

   public:
    BoxBulk() = delete;
    BoxBulk(const std::vector<Cell2D> &positions, const std::vector<Cell2D> &goals, const Color &color, char symbol)
        : positions_(positions), goals_positions_(goals), color_(color), symbol_(symbol) {
        positions_.shrink_to_fit();
        goals_positions_.shrink_to_fit();
    }
    BoxBulk(const BoxBulk &) = default;
    BoxBulk &operator=(const BoxBulk &) = delete;
    ~BoxBulk() = default;

    std::vector<BoxBulk> split() const {
        std::vector<BoxBulk> splitted_bulks;
        for (const auto &position : positions_) {
            splitted_bulks.push_back(BoxBulk(std::vector<Cell2D>{position}, goals_positions_, color_, symbol_));
        }
        return splitted_bulks;
    }

    BoxBulk merge(const BoxBulk &other) const {
        assert(color_ == other.color_);
        assert(symbol_ == other.symbol_);

        std::vector<Cell2D> merged_positions = positions_;
        merged_positions.insert(merged_positions.end(), other.positions_.begin(), other.positions_.end());
        return BoxBulk(merged_positions, goals_positions_, color_, symbol_);
    }

    bool operator==(const BoxBulk &other) const {
        for (const auto &goal : goals_positions_) {
            if (std::find(other.goals_positions_.begin(), other.goals_positions_.end(), goal) == other.goals_positions_.end()) {
                return false;
            }
        }
        for (const auto &position : positions_) {
            if (std::find(other.positions_.begin(), other.positions_.end(), position) == other.positions_.end()) {
                return false;
            }
        }
        return true;
    }

    size_t size() const { return positions_.size(); }
    Cell2D &position(size_t i) { return positions_[i]; }                 // For read-write access
    const Cell2D &getPosition(size_t i) const { return positions_[i]; }  // For read-only access
    const std::vector<Cell2D> &getPositions() const { return positions_; }

    const Cell2D &getGoal(size_t i) const { return goals_positions_[i]; }

    const Color &getColor() const { return color_; }
    char getSymbol() const { return symbol_; }

    void addPosition(const Cell2D &position) { positions_.push_back(position); }
    void addGoal(const Cell2D &goal) { goals_positions_.push_back(goal); }

    bool reachedGoal(void) const {
        bool reached = true;
        for (const auto &goal : goals_positions_) {
            if (std::find(positions_.begin(), positions_.end(), goal) == positions_.end()) {
                reached = false;
                break;
            }
        }
        return reached;
    }

    size_t getHash() const {
        size_t hash = 0;
        for (const auto &position : positions_) {
            hash = hash * 31 + position.r ^ position.c;
        }
        for (const auto &goal : goals_positions_) {
            hash = hash * 31 + goal.r ^ goal.c;
        }
        hash = hash * 31 + int(color_);
        hash = hash * 31 + int(symbol_);
        return hash;
    }
};