#include "utils.hpp"

#include <algorithm>
#include <numeric>
#include <string>
#include <vector>

namespace utils {
// Define the extern variable
std::string whitespaces = " \t\n\r\f\v";

std::string join(const std::vector<std::string> &strings, const std::string &separator) {
    std::string result;
    for (const auto &str : strings) {
        result += str;
        if (str != strings.back()) {
            result += separator;
        }
    }
    return result;
}

std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(whitespaces);
    if (std::string::npos == first) {
        return str;  // String is all whitespace
    }
    size_t last = str.find_last_not_of(whitespaces);
    return str.substr(first, (last - first + 1));
}

// Replace all whitespaces with a single space
std::string normalizeWhitespace(const std::string &str) {
    std::string result;
    for (char c : str) {
        if (whitespaces.find(c) != std::string::npos) {
            result += ' ';
            continue;
        }
        result += c;
    }
    return result;
}

std::string toLower(const std::string &str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), ::tolower);
    return lowerStr;
}

/*
    Sum of Individual Costs (SIC) is the sum of the number of actions in all plans.
    It is used to measure the quality of a solution.
    The lower the SIC, the better.
*/
size_t SIC(const std::vector<std::vector<std::vector<const Action *>>> &solutions) {
    size_t sum_of_individual_costs = 0;
    for (const auto &agent_plans : solutions) {
        sum_of_individual_costs += agent_plans.size();
    }
    return sum_of_individual_costs;
}

size_t makespan(const std::vector<std::vector<std::vector<const Action *>>> &solutions) {
    size_t makespan = 0;
    for (const auto &agent_plans : solutions) {
        makespan = std::max(makespan, agent_plans.size());
    }
    return makespan;
}

size_t fuel_used(const std::vector<std::vector<std::vector<const Action *>>> &solutions) {
    size_t fuel_used = 0;
    for (const auto &agent_plans : solutions) {
        for (const auto &plan : agent_plans) {
            for (const auto &action : plan) {
                if (action->type != ActionType::NoOp) {
                    fuel_used += 1;
                }
            }
        }
    }
    return fuel_used;
}

size_t CBS_cost(const std::vector<std::vector<std::vector<const Action *>>> &solutions) {
    return 10 * makespan(solutions) + SIC(solutions);
}

}  // namespace utils