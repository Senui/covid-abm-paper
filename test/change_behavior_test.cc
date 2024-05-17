#include <gtest/gtest.h>
#include "biodynamo.h"

#include "initialization.h"
#include "mobility_data.h"
#include "person.h"

#define TEST_NAME typeid(*this).name()

namespace bdm {

TEST(ChangeBehavior, Run) {
  Simulation simulation(TEST_NAME);
  auto *scheduler = simulation.GetScheduler();
  scheduler->UnscheduleOp(scheduler->GetOps("load balancing")[0]);
  scheduler->UnscheduleOp(scheduler->GetOps("mechanical forces")[0]);
  
  auto* covid_env = new CovidEnvironment();
  simulation.SetEnvironment(covid_env);
  
  auto* update_statistics_op = NewOperation("update statistics");
  scheduler->ScheduleOp(update_statistics_op);
  
  Person unemployed_home_person(Demographic::kHigherAgeUnemployed);
  unemployed_home_person.home_stay_ = true;

  Person middle_working(Demographic::kMiddleAgeWorking, 30, kMale, 16, 18);

  Person home_working_parent(Demographic::kHigherAgeWorking, 50, kFemale, 29, 29);

  ChangeSituationBehavior behavior;
  behavior.Run(&unemployed_home_person);
  behavior.Run(&middle_working);
  behavior.Run(&home_working_parent);

  // Situations at midnight (timestep 0)
  EXPECT_EQ(kHome, unemployed_home_person.situation_);
  EXPECT_EQ(kOther, middle_working.situation_);
  EXPECT_EQ(kHome, home_working_parent.situation_);

  // Skip to 10 am
  simulation.Simulate(10);
  behavior.Run(&unemployed_home_person);
  behavior.Run(&middle_working);
  behavior.Run(&home_working_parent);

  EXPECT_EQ(kHome, unemployed_home_person.situation_);
  EXPECT_EQ(kWork, middle_working.situation_);
  EXPECT_EQ(kWork, home_working_parent.situation_);
}

}  // namespace bdm
