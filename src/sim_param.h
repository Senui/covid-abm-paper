#ifndef SIM_PARAM_H_
#define SIM_PARAM_H_

#include <vector>

#include "biodynamo.h"

#include "model_facts.h"

namespace bdm {

/// This class defines parameters that are specific to this simulation.
class SimParam : public ParamGroup {
 private:
  BDM_CLASS_DEF_OVERRIDE(SimParam, 1);

 public:
  static const ParamGroupUid kUid;
  virtual ~SimParam() {}
  ParamGroup* NewCopy() const override { return new SimParam(*this); }
  ParamGroupUid GetUid() const override { return kUid; }

  // Amount of times to repeat the simulation (for statistical reasons)
  int repeat = 0;
  // Mode at which to execute this simulation (single simulation or distributed
  // fitting)
  std::string mode = "sim-and-analytical";
  uint64_t population_size = 17000;
  // Flag to export affected population per demography over time
  bool export_affected = false;
  real_t init_infection_rate = 0.1;
  int init_infection_time = 17 * 24;
  int export_infected_per_timestep_frequency = 0;
  // Average number of interactions per day per person. From: https://journals.plos.org/plosmedicine/article?id=10.1371/journal.pmed.0050074
  real_t avg_interactions = 13.4;
  // Average number of interactions per day per person using the mixing matrices found in src/data/
  real_t emperical_avg_interactions = 140.3;
  // Overrides the population_size / num_agents scaling factor
  real_t custom_agent_to_person_ratio = 0;
  // total_hours = hours between 27 feb - 1 june (2020) = 2256
  uint32_t total_hours = 2256;
  // hours between 27 feb - 12 march = 336
  uint32_t phase_1_hours = 336;
  // hours between 12 march - 23 march = 264
  uint32_t phase_2_hours = 264;
  // hours between 23 march - 11 may = 1176
  uint32_t phase_3_hours = 1176;
  // hours between 11 may - 1 jun = 504
  uint32_t phase_4_hours = 504;
  float phase_2_mobility_reduction = 0.372;
  float phase_3_mobility_reduction = 0.424;
  float phase_4_mobility_reduction = 0.201;
  real_t beta1 = 2;
  real_t beta2 = 0.1078;
  real_t beta3 = 0.4675;
  real_t beta4 = 0.1196;
  // Percentage of parents whose children did homeschooling during phase 2
  float phase_2_homeschooling_parents = 0.12;
  experimental::Style root_style;
  bool no_fixed_seed = false;
  std::string result_plot = "result";
  bool no_legend = false;
  float incubation_shape_param = 20;
  float incubation_scale_param = 4.6 * kHoursPerDay;
  float infection_shape_param = 1;
  float infection_scale_param = 5 * kHoursPerDay;
  float hospitalization_shape_param = 14;
  float hospitalization_scale_param = 10 * kHoursPerDay;
  // TODO: make this dependent on demography:
  // https://www.mdpi.com/1660-4601/17/20/7560/htm (Section 3.2)
  float hospital_average_mean = 2.48;
  float hospital_average_sigma = 0.913;
  int homestay_mean = 15;
  int homestay_sigma = 6;
  real_t initial_infection_scaling = 10;
  real_t initial_exposed_infected_ratio = 3;
};

}  // namespace bdm

#endif  // SIM_PARAM_H_
