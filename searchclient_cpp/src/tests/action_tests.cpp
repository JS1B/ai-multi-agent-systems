#include <gtest/gtest.h>
#include "action.hpp"

TEST(ActionTests, NoOpAction) {
    EXPECT_EQ("NoOp", Action::NoOp.name);
    EXPECT_EQ(ActionType::NoOp, Action::NoOp.type);
    EXPECT_EQ(0, Action::NoOp.agentRowDelta);
    EXPECT_EQ(0, Action::NoOp.agentColDelta);
    EXPECT_EQ(0, Action::NoOp.boxRowDelta);
    EXPECT_EQ(0, Action::NoOp.boxColDelta);
}

TEST(ActionTests, MoveActions) {
    // Test MoveN
    EXPECT_EQ("Move(N)", Action::MoveN.name);
    EXPECT_EQ(ActionType::Move, Action::MoveN.type);
    EXPECT_EQ(-1, Action::MoveN.agentRowDelta);
    EXPECT_EQ(0, Action::MoveN.agentColDelta);
    EXPECT_EQ(0, Action::MoveN.boxRowDelta);
    EXPECT_EQ(0, Action::MoveN.boxColDelta);

    // Test MoveS
    EXPECT_EQ("Move(S)", Action::MoveS.name);
    EXPECT_EQ(ActionType::Move, Action::MoveS.type);
    EXPECT_EQ(1, Action::MoveS.agentRowDelta);
    EXPECT_EQ(0, Action::MoveS.agentColDelta);
    EXPECT_EQ(0, Action::MoveS.boxRowDelta);
    EXPECT_EQ(0, Action::MoveS.boxColDelta);
}

TEST(ActionTests, PushActions) {
    // Test PushNN
    EXPECT_EQ("Push(N,N)", Action::PushNN.name);
    EXPECT_EQ(ActionType::Push, Action::PushNN.type);
    EXPECT_EQ(-1, Action::PushNN.agentRowDelta);
    EXPECT_EQ(0, Action::PushNN.agentColDelta);
    EXPECT_EQ(-1, Action::PushNN.boxRowDelta);
    EXPECT_EQ(0, Action::PushNN.boxColDelta);

    // Test PushSE
    EXPECT_EQ("Push(S,E)", Action::PushSE.name);
    EXPECT_EQ(ActionType::Push, Action::PushSE.type);
    EXPECT_EQ(1, Action::PushSE.agentRowDelta);
    EXPECT_EQ(0, Action::PushSE.agentColDelta);
    EXPECT_EQ(0, Action::PushSE.boxRowDelta);
    EXPECT_EQ(1, Action::PushSE.boxColDelta);
}

TEST(ActionTests, PullActions) {
    // Test PullWW
    EXPECT_EQ("Pull(W,W)", Action::PullWW.name);
    EXPECT_EQ(ActionType::Pull, Action::PullWW.type);
    EXPECT_EQ(0, Action::PullWW.agentRowDelta);
    EXPECT_EQ(-1, Action::PullWW.agentColDelta);
    EXPECT_EQ(0, Action::PullWW.boxRowDelta);
    EXPECT_EQ(-1, Action::PullWW.boxColDelta);

    // Test PullES
    EXPECT_EQ("Pull(E,S)", Action::PullES.name);
    EXPECT_EQ(ActionType::Pull, Action::PullES.type);
    EXPECT_EQ(0, Action::PullES.agentRowDelta);
    EXPECT_EQ(1, Action::PullES.agentColDelta);
    EXPECT_EQ(1, Action::PullES.boxRowDelta);
    EXPECT_EQ(0, Action::PullES.boxColDelta);
}

TEST(ActionTests, ActionEquality) {
    EXPECT_EQ(Action::NoOp, Action::NoOp);
    EXPECT_EQ(Action::MoveN, Action::MoveN);
    EXPECT_NE(Action::MoveN, Action::MoveS);
    EXPECT_NE(Action::PushNN, Action::PullNN);
} 