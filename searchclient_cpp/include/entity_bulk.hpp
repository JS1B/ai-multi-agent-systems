#pragma once

#include <cassert>
#include <vector>

#include "cell2d.hpp"
#include "chargrid.hpp"
#include "color.hpp"

class EntityBulk {
   private:
    std::vector<Cell2D> positions_;
    std::vector<Cell2D> goals_positions_;  // if any
    const Color color_;
    const char symbol_;

    mutable size_t hash_ = 0;

   public:
    EntityBulk() = delete;
    EntityBulk(const std::vector<Cell2D> &positions, const std::vector<Cell2D> &goals, const Color &color, char symbol)
        : positions_(positions), goals_positions_(goals), color_(color), symbol_(symbol) {
        positions_.shrink_to_fit();
        goals_positions_.shrink_to_fit();
    }
    EntityBulk(const EntityBulk &) = default;
    EntityBulk &operator=(const EntityBulk &) = delete;
    ~EntityBulk() = default;

    std::vector<EntityBulk> split() const {
        std::vector<EntityBulk> splitted_bulks;
        for (const auto &position : positions_) {
            splitted_bulks.push_back(EntityBulk(std::vector<Cell2D>{position}, goals_positions_, color_, symbol_));
        }
        return splitted_bulks;
    }

    EntityBulk merge(const EntityBulk &other) const {
        assert(color_ == other.color_);
        assert(symbol_ == other.symbol_);

        std::vector<Cell2D> merged_positions = positions_;
        merged_positions.insert(merged_positions.end(), other.positions_.begin(), other.positions_.end());
        return EntityBulk(merged_positions, goals_positions_, color_, symbol_);
    }

    bool operator==(const EntityBulk &other) const {
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
        // if (hash_ != 0) {
        //     return hash_;
        // }
        for (const auto &position : positions_) {
            hash_ = hash_ * 31 + position.r ^ position.c;
        }
        for (const auto &goal : goals_positions_) {
            hash_ = hash_ * 31 + goal.r ^ goal.c;
        }
        hash_ = hash_ * 31 + int(color_);
        hash_ = hash_ * 31 + int(symbol_);
        return hash_;
    }
};