#ifndef CHANGE_SITUATION_BEHAVIOR_H_
#define CHANGE_SITUATION_BEHAVIOR_H_

#include "model_facts.h"
#include "person.h"

#include "core/simulation.h"

namespace bdm {

/// The situation of a person refers to the social context (e.g. if a person is
/// at school, work, home, etc). The situation is determined by the following
/// factors:
/// 1. Time of day: is it daytime (08:00 - 18:00) or night-time?
/// 2. Location (home or not?)
/// 3. Demography
struct ChangeSituationBehavior : public Behavior {
  BDM_BEHAVIOR_HEADER(ChangeSituationBehavior, Behavior, 1);

  ChangeSituationBehavior() {}

  void Run(Agent* agent) override {
    Person* person = bdm_static_cast<Person*>(agent);
    auto sim = Simulation::GetActive();
    auto current_timestep = sim->GetScheduler()->GetSimulatedSteps();
    auto hour_of_day = current_timestep % kHoursPerDay;
    auto is_home = person->home_location_ == person->location_;
    auto demography = person->demography_;

    if (hour_of_day < 9 || hour_of_day > 17) {
      if (is_home) {
        person->situation_ = kHome;
      } else {
        person->situation_ = kOther;
      }
    } else {
      if (person->home_stay_) {
        person->situation_ = kHome;
      } else if (is_home) {
        person->situation_ = kDayTimeMixingHome[demography];
      } else {
        person->situation_ = kDayTimeMixingOther[demography];
      }
    }
  }
};

}  // namespace bdm

#endif  // CHANGE_SITUATION_BEHAVIOR_H_
