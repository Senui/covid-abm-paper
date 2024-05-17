#ifndef CBS_COVID_H_
#define CBS_COVID_H_

#include "biodynamo.h"
#include "core/multi_simulation/optimization_param.h"
#include "core/randomized_rm.h"

#include "covid_environment.h"
#include "evaluate.h"
#include "initialization.h"
#include "interventions.h"
#include "operations/export_statistics_op.h"
#include "sim_param.h"

namespace bdm {

inline int Simulate(int argc, const char** argv, TimeSeries* result,
                    Param* final_params = nullptr) {
  auto set_param = [&](Param* param) {
    param->Restore(std::move(*final_params));
  };

  auto clo = CommandLineOptions(argc, argv);
  clo.AddOption<std::string>("cbsdir", "",
                             "Full path to the population data (CSV format)");
  clo.AddOption<bool>("randominit", "false",
                      "Use random population for initialization");
  clo.AddOption<bool>("exportstats", "false",
                      "Export custom-defined simulation statistics");
  clo.AddOption<bool>("print_timings", "false",
                      "Print selected timing results per simulation");

  Simulation simulation(&clo, set_param);

  std::cout << *(ThreadInfo::GetInstance()) << std::endl;

  auto* opts = simulation.GetCommandLineOptions();
  auto cbsdir = opts->Get<std::string>("cbsdir");
  auto randominit = opts->Get<bool>("randominit");
  auto exportstats = opts->Get<bool>("exportstats");
  auto print_timings = opts->Get<bool>("print_timings");
  if (cbsdir == "" && !randominit) {
    Log::Fatal(
        "Pass the data dir with the --cbsdir flag or use a random "
        "initialization with the --randominit flag");
  }

  auto* param = simulation.GetParam();
  auto* sparam = param->Get<SimParam>();
  auto* scheduler = simulation.GetScheduler();
  // turn off load balancing as the custom environment does not support it
  scheduler->UnscheduleOp(scheduler->GetOps("load balancing")[0]);
  scheduler->UnscheduleOp(scheduler->GetOps("mechanical forces")[0]);

  auto* covid_env = new CovidEnvironment();
  simulation.SetEnvironment(covid_env);

  // Use a resource manager that allows random reordering of agents to iterate
  // randomly over agents Turn off auto_randomize so it only randomizes the
  // order upon invoking `RandomizeAgentsOrder`
  auto* rand_rm = new RandomizedRm<ResourceManager>(false);
  simulation.SetResourceManager(rand_rm);

  {
    Timing timer("Initialization", scheduler->GetOpTimes());
    InitializePopulation(cbsdir, randominit);
  }

  // Add counters to the simulations to create statistics for plotting
  SetupResultCollection(&simulation);

  // Schedule the operation for updating the statistical data of this model
  auto* update_statistics_op = NewOperation("update statistics");
  scheduler->ScheduleOp(update_statistics_op);

  // Schedule the operation for initializing the infections (must be AFTER update statistics)
  auto* initial_infections = NewOperation("initial infection");
  scheduler->ScheduleOp(initial_infections);

  if (exportstats) {
    auto* export_statistics_op = NewOperation("export statistics");
    scheduler->ScheduleOp(export_statistics_op);
  }

  Timing timer("Simulation", scheduler->GetOpTimes());

  // Run simulation - phase 0 (initial infections)
  // Run until we reach a total number of infection count greater or equal to the estimated initial infections
  std::cout << "Starting Phase 0..." << std::endl;
  kActivePhase = 0;
  std::vector<int> initial_infected;
  std::string data_dir = GetDataDir();
  std::string init_infected_file =
      data_dir + "/initial_infected_per_municipality.csv";

  CsvToVector<int>(init_infected_file, &initial_infected, 1, 0);
  auto real_infection_count = std::accumulate(initial_infected.begin(), initial_infected.end(), 0) / GetAgentToPersonRatio();
  auto init_infection_time = sparam->init_infection_time;
  auto infection_reached = [&]() {
    if (scheduler->GetSimulatedSteps() < init_infection_time) {
      return false;
    } else {
      return true;
    }
  };
  scheduler->SimulateUntil(infection_reached);
  std::cout << "It took " << scheduler->GetSimulatedSteps() << " timesteps to reach the initial infection situation" << std::endl;

  // Now that we reached the estimated initial state, we can remove the artificial infection behavior and let the model run the SEIR behavior only
  scheduler->UnscheduleOp(scheduler->GetOps("initial infection")[0]);

  // Run simulation - phase 1
  std::cout << "Starting Phase 1..." << std::endl;
  kActivePhase = 0;
  scheduler->Simulate(sparam->phase_1_hours);

  // Prepare for phase 2 - working from home policy
  std::cout << "Starting Phase 2..." << std::endl;
  kActivePhase = 1;
  MobilityReductionPhase2();
  AdjustMixingMatrices(kActivePhase);
  SchoolClosure();
  scheduler->Simulate(sparam->phase_2_hours);

  std::cout << "Starting Phase 3..." << std::endl;
  kActivePhase = 2;
  MobilityReductionPhase3();
  scheduler->Simulate(sparam->phase_3_hours);

  std::cout << "Starting Phase 4..." << std::endl;
  kActivePhase = 3;
  AdjustMixingMatrices(kActivePhase);
  scheduler->Simulate(sparam->phase_4_hours);

  timer.~Timing();

  if (exportstats) {
    auto op = scheduler->GetOps("export statistics")[0]
                  ->GetImplementation<ExportStatisticsOp>();
    std::vector<real_t> x_values(op->avg_person_interactions_over_time.size());
    std::iota(std::begin(x_values), std::end(x_values), 0);
    simulation.GetTimeSeries()->Add("ts_avg_person_interactions_over_time", x_values,
                                    op->avg_person_interactions_over_time);
  }

  // We keep track of all timesteps at the operation, and then we add a timeseries entry per timestep (because Timeseries::Data cannot handle tracking vector<real> per timestep)
  // Used to generate a heat map of the hospital admissions of the country
  auto f = sparam->export_infected_per_timestep_frequency;
  if (f != 0) {
    auto update_stat_op = Simulation::GetActive()
                        ->GetScheduler()
                        ->GetOps("update statistics")[0]
                        ->GetImplementation<UpdateStatisticsOp>();
    auto& total_inf_per_mun_over_time = update_stat_op->total_infected_per_municipality_;
    auto& total_per_mun_over_time = update_stat_op->total_per_municipality_;
    for (size_t t = 0; t < total_inf_per_mun_over_time.size(); t++) {
      std::vector<real_t> x_values(total_inf_per_mun_over_time[t].size());
      std::iota(std::begin(x_values), std::end(x_values), 0);
      std::string ts_name = "total_infected_per_municipality_t" + std::to_string(t * f);
      std::string ts_name2 = "total_per_municipality_t" + std::to_string(t * f);
      simulation.GetTimeSeries()->Add(ts_name, x_values, total_inf_per_mun_over_time[t]);
      simulation.GetTimeSeries()->Add(ts_name2, x_values, total_per_mun_over_time[t]);
    }
  }

  if (print_timings) {
    std::cout << *(scheduler->GetOpTimes()) << std::endl;
  }

  // Add any other results or metadata of interest
  simulation.GetTimeSeries()->Add("resolution", {0}, {GetAgentToPersonRatio()});
  if (sparam->mode != "sim-and-analytical") {
    auto opt_params = param->Get<OptimizationParam>();
    simulation.GetTimeSeries()->Add(
        "repetitions", {0}, {static_cast<real_t>(opt_params->repetition)});
  } else {
    simulation.GetTimeSeries()->Add("repetitions", {0},
                                    {static_cast<real_t>(sparam->repeat)});
  }

  // move time series data from simulation to result
  *result = std::move(*simulation.GetTimeSeries());
  return 0;
}

}  // namespace bdm

#endif  // CBS_COVID_H_
