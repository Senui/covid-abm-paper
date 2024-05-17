#include <set>

#include <gtest/gtest.h>

// #define private public
#include "biodynamo.h"

#include "evaluate.h"
#include "covid_environment.h"
#include "person.h"

#define TEST_NAME typeid(*this).name()

namespace bdm {

TEST(Evaluate, SetupResultCollection) {
  Simulation simulation(TEST_NAME);
  auto *rm = simulation.GetResourceManager();
  auto *scheduler = simulation.GetScheduler();

  scheduler->UnscheduleOp(scheduler->GetOps("load balancing")[0]);
  scheduler->UnscheduleOp(scheduler->GetOps("mechanical forces")[0]);
  auto* covid_env = new CovidEnvironment();
  simulation.SetEnvironment(covid_env);
  // Schedule the operation for updating the statistical data of this model
  auto* update_statistics_op = NewOperation("update statistics");
  scheduler->ScheduleOp(update_statistics_op);

  std::array<std::pair<State, int>, 4> seir_distribution = {
      std::make_pair<State, int>(kInfectious, 1500),
      std::make_pair<State, int>(kRecovered, 2000),
      std::make_pair<State, int>(kSusceptible, 6000),
      std::make_pair<State, int>(kExposed, 500)};

  for (auto &pair : seir_distribution) {
    for (int i = 0; i < pair.second; i++) {
      Person *p =
          new Person(Demographic::kElderly, 0, Gender::kMale, 0, 0, pair.first);
      if (pair.first == State::kRecovered) {
        p->hospitalized_ = true;
      }
      rm->AddAgent(p);
    }
  }

  // Set up the collectors
  SetupResultCollection(&simulation);

  // Simulate 1 timestep
  simulation.Simulate(1);

  // Check collectors
  auto ts = simulation.GetTimeSeries();
  EXPECT_EQ(1500 * GetAgentToPersonRatio(), ts->GetYValues("ts_infectious")[0]);
  EXPECT_EQ(500 * GetAgentToPersonRatio(), ts->GetYValues("ts_exposed")[0]);
  EXPECT_EQ(2000 * GetAgentToPersonRatio(), ts->GetYValues("ts_hospitalized")[0]);
  EXPECT_NEAR(0.4, ts->GetYValues("ts_affected_Elderly")[0], 1e-7);
}

}  // namespace bdm
