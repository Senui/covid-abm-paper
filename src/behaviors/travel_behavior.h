#ifndef TRAVEL_BEHAVIOR_H_
#define TRAVEL_BEHAVIOR_H_

#include "model_facts.h"
#include "person.h"

namespace bdm {

struct TravelBehavior : public Behavior {
  BDM_BEHAVIOR_HEADER(TravelBehavior, Behavior, 1);

  TravelBehavior() {}

  void Run(Agent* agent) override {
    Person* person = bdm_static_cast<Person*>(agent);
    // If a person is working from home, we put this person at home
    if (person->home_stay_) {
      person->location_ = person->home_location_;
    } else {
      auto sim = Simulation::GetActive();
      auto current_timestep = sim->GetScheduler()->GetSimulatedSteps();
      auto hour_of_week = current_timestep % (kHoursPerDay * kDaysPerWeek);

      auto* schedule = person->GetWeeklyTravelSchedule();
      auto next_location = (*schedule)[hour_of_week];
      person->Travel(next_location);
    }
  }
};

}  // namespace bdm

#endif  // TRAVEL_BEHAVIOR_H_
