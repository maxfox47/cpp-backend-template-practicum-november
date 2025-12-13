#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include "../src/tv.h"

// Тестовый стенд "Телевизор по умолчанию"
class TVByDefault : public testing::Test {
protected:
    TV tv_;
};

TEST_F(TVByDefault, IsOff) {
    EXPECT_FALSE(tv_.IsTurnedOn());
}

TEST_F(TVByDefault, DoesntShowAChannelWhenItIsOff) {
    EXPECT_FALSE(tv_.GetChannel().has_value());
}

TEST_F(TVByDefault, CantSelectAnyChannel) {
    EXPECT_THROW(tv_.SelectChannel(10), std::logic_error);
    EXPECT_EQ(tv_.GetChannel(), std::nullopt);
    tv_.TurnOn();
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(1));
}

TEST_F(TVByDefault, CantSelectLastViewedChannel) {
    EXPECT_THROW(tv_.SelectLastViewedChannel(), std::logic_error);
}

// Тестовый стенд "Включенный телевизор"
class TurnedOnTV : public TVByDefault {
protected:
    void SetUp() override {
        tv_.TurnOn();
    }
};

TEST_F(TurnedOnTV, ShowsChannel1) {
    EXPECT_TRUE(tv_.IsTurnedOn());
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(1));
}

TEST_F(TurnedOnTV, AfterTurningOffTurnsOffAndDoesntShowAnyChannel) {
    tv_.TurnOff();
    EXPECT_FALSE(tv_.IsTurnedOn());
    EXPECT_EQ(tv_.GetChannel(), std::nullopt);
}

TEST_F(TurnedOnTV, CanSelectChannelFrom1To99) {
    for (int channel = TV::MIN_CHANNEL; channel <= TV::MAX_CHANNEL; ++channel) {
        tv_.SelectChannel(channel);
        EXPECT_THAT(tv_.GetChannel(), testing::Optional(channel));
    }
}

TEST_F(TurnedOnTV, ThrowsOutOfRangeForChannelBelowMin) {
    EXPECT_THROW(tv_.SelectChannel(TV::MIN_CHANNEL - 1), std::out_of_range);
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(1));
}

TEST_F(TurnedOnTV, ThrowsOutOfRangeForChannelAboveMax) {
    EXPECT_THROW(tv_.SelectChannel(TV::MAX_CHANNEL + 1), std::out_of_range);
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(1));
}

TEST_F(TurnedOnTV, CanSelectChannel1) {
    tv_.SelectChannel(1);
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(1));
}

TEST_F(TurnedOnTV, CanSelectChannel99) {
    tv_.SelectChannel(99);
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(99));
}

TEST_F(TurnedOnTV, SelectChannelSavesPreviousChannel) {
    tv_.SelectChannel(5);
    tv_.SelectChannel(10);
    tv_.SelectLastViewedChannel();
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(5));
}

TEST_F(TurnedOnTV, SelectLastViewedChannelSwitchesBetweenTwoChannels) {
    tv_.SelectChannel(5);
    tv_.SelectChannel(10);
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(10));
    
    tv_.SelectLastViewedChannel();
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(5));
    
    tv_.SelectLastViewedChannel();
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(10));
    
    tv_.SelectLastViewedChannel();
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(5));
}

TEST_F(TurnedOnTV, SelectChannelDoesNothingIfSameChannel) {
    tv_.SelectChannel(5);
    tv_.SelectChannel(10);
    tv_.SelectChannel(5);
    tv_.SelectLastViewedChannel();
    // После переключения на предыдущий канал должен вернуться к 10
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(10));
}

TEST_F(TurnedOnTV, TurnOnRestoresLastChannel) {
    tv_.SelectChannel(42);
    tv_.TurnOff();
    tv_.TurnOn();
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(42));
}

TEST_F(TurnedOnTV, TurnOnFirstTimeSelectsChannel1) {
    TV new_tv;
    new_tv.TurnOn();
    EXPECT_THAT(new_tv.GetChannel(), testing::Optional(1));
}

TEST_F(TurnedOnTV, MultipleTurnOnDoesNotChangeChannel) {
    tv_.SelectChannel(42);
    tv_.TurnOn();
    tv_.TurnOn();
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(42));
}

TEST_F(TurnedOnTV, SelectLastViewedChannelWhenNoPreviousChannelDoesNothing) {
    // После первого включения нет предыдущего канала
    tv_.SelectLastViewedChannel();
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(1));
}

TEST_F(TurnedOnTV, SelectLastViewedChannelAfterOneChannelChange) {
    tv_.SelectChannel(5);
    tv_.SelectLastViewedChannel();
    // Должен вернуться к каналу 1
    EXPECT_THAT(tv_.GetChannel(), testing::Optional(1));
}

