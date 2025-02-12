#ifndef SEARCHCLIENT_GRAPHSEARCH_H
#define SEARCHCLIENT_GRAPHSEARCH_H

#include "action.h"
#include "frontier.h"
#include "state.h"

#include <vector>

std::vector<std::vector<Action>> search(State initial_state, Frontier *frontier);
void printSearchStatus(const std::set<State> &explored, Frontier *frontier);

#endif // SEARCHCLIENT_GRAPHSEARCH_H