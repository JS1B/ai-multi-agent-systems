#include <gtest/gtest.h>
#include "color.hpp"

TEST(ColorTests, from_stringValidColors) {
    EXPECT_EQ(Color::Blue, from_string("blue"));
    EXPECT_EQ(Color::Red, from_string("RED"));
    EXPECT_EQ(Color::Cyan, from_string("cYaN"));
    EXPECT_EQ(Color::Purple, from_string("purple"));
    EXPECT_EQ(Color::Green, from_string("GREEN"));
    EXPECT_EQ(Color::Orange, from_string("orange"));
    EXPECT_EQ(Color::Pink, from_string("PINK"));
    EXPECT_EQ(Color::Grey, from_string("grey"));
    EXPECT_EQ(Color::Lightblue, from_string("lightblue"));
    EXPECT_EQ(Color::Brown, from_string("brown"));
}

TEST(ColorTests, from_stringInvalidColor) {
    EXPECT_THROW(from_string("invalid"), std::invalid_argument);
    EXPECT_THROW(from_string(""), std::invalid_argument);
    EXPECT_THROW(from_string("blu"), std::invalid_argument);
    EXPECT_THROW(from_string("bluee"), std::invalid_argument);
} 