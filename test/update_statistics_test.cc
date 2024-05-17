#include <set>

#include <gtest/gtest.h>
#include "biodynamo.h"

#include "model_facts.h"
#include "operations/update_statistics_op.h"
#include "person.h"

#define TEST_NAME typeid(*this).name()

namespace bdm {

// Create a fake population where each municipality contains N persons. Each
// municipality has a 10% infection rate, except for 5 municipalities (0, 80,
// 120, 160, 240), which each have a 15% infection rate
TEST(UpdateStatistics, CalculateFractions) {
  Simulation simulation(TEST_NAME);
  auto *rm = simulation.GetResourceManager();

  int population_per_municipality = 1000;
  real_t regular_infection_rate = 0.1;
  real_t irregular_infection_rate = 0.15;
  std::set<int> irregular_municipalities = {0, 80, 120, 160, 240};

  for (size_t i = 0; i < kNumMunicipalities; i++) {
    auto inf_rate =
        irregular_municipalities.find(i) == irregular_municipalities.end()
            ? regular_infection_rate
            : irregular_infection_rate;
    for (int j = 0; j < population_per_municipality; j++) {
      State state = j < std::round(inf_rate * population_per_municipality)
                        ? State::kInfectious
                        : State::kSusceptible;
      Person *p =
          new Person(Demographic::kElderly, 0, Gender::kMale, i, i, state);
      rm->AddAgent(p);
    }
  }

  UpdateStatisticsOp stat_op;
  stat_op.Reset();
  stat_op.CalculateFractions();

  EXPECT_EQ(1000u, stat_op.total_[0][Demographic::kElderly]);
  EXPECT_EQ(1000u, stat_op.total_[53][Demographic::kElderly]);
  EXPECT_EQ(1000u, stat_op.total_[98][Demographic::kElderly]);
  EXPECT_EQ(1000u, stat_op.total_[183][Demographic::kElderly]);

  EXPECT_EQ(150u, stat_op.infected_[0][Demographic::kElderly]);
  EXPECT_EQ(150u, stat_op.infected_[80][Demographic::kElderly]);
  EXPECT_EQ(150u, stat_op.infected_[120][Demographic::kElderly]);
  EXPECT_EQ(100u, stat_op.infected_[1][Demographic::kElderly]);
  EXPECT_EQ(100u, stat_op.infected_[29][Demographic::kElderly]);
  EXPECT_EQ(100u, stat_op.infected_[323][Demographic::kElderly]);

  EXPECT_NEAR(0.15, stat_op.fractions_[0][Demographic::kElderly], 1e-7);
  EXPECT_NEAR(0.10, stat_op.fractions_[1][Demographic::kElderly], 1e-7);
}

}  // namespace bdm
