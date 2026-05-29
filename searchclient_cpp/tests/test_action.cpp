#include <cassert>
#include <iostream>

#include "action.hpp"

void testAllValuesCatalog() {
    const auto &actions = Action::allValues();

    assert(actions.size() == 29);
    assert(actions[0] == &Action::NoOp);
    assert(actions[1] == &Action::MoveN);
    assert(actions[5] == &Action::PushNN);
    assert(actions[17] == &Action::PullNN);

    std::cout << "testAllValuesCatalog passed!" << std::endl;
}

void testPermutationCacheShape() {
    const auto &zero_agent_permutations = Action::getAllPermutations(0);
    const auto &one_agent_permutations = Action::getAllPermutations(1);
    const auto &two_agent_permutations = Action::getAllPermutations(2);

    assert(zero_agent_permutations.size() == 1);
    assert(zero_agent_permutations[0].empty());
    assert(one_agent_permutations.size() == Action::allValues().size());
    assert(two_agent_permutations.size() == Action::allValues().size() * Action::allValues().size());
    assert(&one_agent_permutations == &Action::getAllPermutations(1));

    std::cout << "testPermutationCacheShape passed!" << std::endl;
}

void testJointActionFormatting() {
    const std::vector<const Action *> joint_action = {&Action::MoveN, &Action::PushEE, &Action::NoOp};

    assert(formatJointAction(joint_action, false) == "Move(N)|Push(E,E)|NoOp");
    assert(formatJointAction(joint_action, true) == "Move(N)@Move(N)|Push(E,E)@Push(E,E)|NoOp@NoOp");

    std::cout << "testJointActionFormatting passed!" << std::endl;
}

int main() {
    testAllValuesCatalog();
    testPermutationCacheShape();
    testJointActionFormatting();
    return 0;
}
