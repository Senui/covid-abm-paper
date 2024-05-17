#ifndef INFECTION_BEHAVIOR_H_
#define INFECTION_BEHAVIOR_H_

#include <random>

#include "core/behavior/behavior.h"
#include "core/container/math_array.h"

#include "covid_environment.h"
#include "model_facts.h"
#include "operations/update_statistics_op.h"
#include "person.h"
#include "sim_param.h"

// For each person we run the SEIR model. Depending on their state (Susceptible,
// Exposed, Infectious, Recovered), we compute the next likely state.
// The likeliness of the next state is determined by the following:
// 1) A Person's demographic group
// 2) Restrictions imposed by government / municipality
// 3) Mixing with other Persons

static std::default_random_engine generator;

namespace bdm {

inline float DemographicMixing(Person* person) {
  float ret = 0;

  auto* sim = Simulation::GetActive();
  auto* env = bdm_static_cast<CovidEnvironment*>(sim->GetEnvironment());

  UpdateStatisticsOp* stat_op = sim->GetScheduler()
                                    ->GetOps("update statistics")[0]
                                    ->GetImplementation<UpdateStatisticsOp>();

  const auto& fractions = stat_op->fractions_;
  auto demography = person->demography_;
  auto municipality = person->location_;
  const auto mix_mat = env->GetMixingMatrix(person->situation_);

  // fetch contact matrix
  for (auto other_demo = 0; other_demo < kNumDemographies; other_demo++) {
    // `fractions` takes into account the infected fraction of the population per municipality per demography
    // `normalize_interactions` normalizes the interactions per timestep (i.e. per hour) based on the average number of interactions a person has on a day
    ret +=
        mix_mat[demography][other_demo] * fractions[municipality][demography];
  }
  return ret;
}

inline real_t PhaseToBeta(const SimParam* sparam, uint8_t phase) {
  if (phase == 0) {
    return sparam->beta1;
  } else if (phase == 1) {
    return sparam->beta2;
  } else if (phase == 1) {
    return sparam->beta3;
  } else {
    return sparam->beta4;
  }
}

struct InfectionBehavior : public Behavior {
  BDM_BEHAVIOR_HEADER(InfectionBehavior, Behavior, 1);

  void DrawFromDistributions(State state = State::kSusceptible) {
    auto* sim = Simulation::GetActive();
    auto* rand = sim->GetRandom();
    auto* sparam = sim->GetParam()->Get<SimParam>();

    // Draw from weibull distribution to determine incubation / infection time
    std::weibull_distribution<float> incubation_distribution(
        sparam->incubation_shape_param, sparam->incubation_scale_param);
    std::weibull_distribution<float> infection_distribution(
        sparam->infection_shape_param, sparam->infection_scale_param);
    std::weibull_distribution<float> hospitalization_distribution(
        sparam->hospitalization_shape_param,
        sparam->hospitalization_scale_param);
    std::lognormal_distribution<float> hospitalization_los(
        sparam->hospital_average_mean, sparam->hospital_average_sigma);
    incubation_time_threshold_ = incubation_distribution(generator);
    infection_time_threshold_ = infection_distribution(generator);
    hospitalization_time_threshold_ = hospitalization_distribution(generator);
    hospital_length_of_stay_ = kHoursPerDay * hospitalization_los(generator);

    // For those initialized with non-kSusceptible state, we also initialize
    // the time they are in that state
    if (state == State::kExposed) {
      incubation_time_ = rand->Uniform(0, incubation_time_threshold_);
    } else if (state == State::kInfectious) {
      infection_time_ = rand->Uniform(0, infection_time_threshold_);
    }
  }

  InfectionBehavior() { DrawFromDistributions(); }

  virtual ~InfectionBehavior() {}

  void Run(Agent* a) override {
    auto* person = bdm_static_cast<Person*>(a);
    auto* sim = Simulation::GetActive();
    auto* random = sim->GetRandom();
    auto* sparam = sim->GetParam()->Get<SimParam>();
    auto g = person->demography_;
    // We only decided once per agent if they will be hospitalized based on the changes defined at kHospitalizationPerDemography
    if (!initialized_) {
      if (random->Uniform(0, 1) <= kHospitalizationPerDemography[g]) {
        hospitalize_person_ = true;
      }
      initialized_ = true;
    }
    if (person->state_ == kSusceptible) {
      auto t = sim->GetScheduler()->GetSimulatedSteps();
      uint8_t hour_of_day = t % kHoursPerDay;
      auto s = kDailySleepPattern[hour_of_day];
      auto mix_sum = DemographicMixing(person);
      auto lambda =
          kSusceptibility[g] * PhaseToBeta(sparam, kActivePhase) * s * mix_sum;
      if (random->Uniform(0, 1) <= lambda && lambda > 0) {
        person->state_ = kExposed;
      }
    } else if (person->state_ == kExposed) {
      if (incubation_time_ > incubation_time_threshold_) {
        person->state_ = kInfectious;
      } else {
        incubation_time_++;
      }
    } else if (person->state_ == kInfectious) {
      if (infection_time_ > infection_time_threshold_) {
        person->state_ = kRecovered;
      } else {
        hospitalization_time_++;
        infection_time_++;
        if (hospitalization_time_ > hospitalization_time_threshold_ &&
            hospitalize_person_) {
          person->hospitalized_ = true;
        }
      }
    } else if (person->state_ == kRecovered) {
      // It's possible that a person is recovered before the time lag of
      // hospitalization is reached. We continue the hospitalization time lag in
      // this state to account for this
      hospitalization_time_++;
      if (hospitalization_time_ > hospitalization_time_threshold_ &&
          hospitalize_person_) {
        person->hospitalized_ = true;
      }

      if (person->hospitalized_) {
        if (time_in_hospital_ > hospital_length_of_stay_) {
          person->hospitalized_ = false;
        } else {
          time_in_hospital_++;
        }
      }
    }
  }

  uint32_t infection_time_ = 0;
  uint32_t infection_time_threshold_ = 0;
  uint32_t incubation_time_ = 0;
  uint32_t incubation_time_threshold_ = 0;
  uint32_t hospitalization_time_ = 0;
  uint32_t hospitalization_time_threshold_ = 0;
  uint32_t hospital_length_of_stay_ = 0;
  uint32_t time_in_hospital_ = 0;
  bool hospitalize_person_ = false;
  bool initialized_ = false;
};

}  // namespace bdm

#endif  // INFECTION_BEHAVIOR_H_
