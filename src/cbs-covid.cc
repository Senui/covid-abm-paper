// -----------------------------------------------------------------------------
//
// Copyright (C) 2021 CERN & University of Surrey for the benefit of the
// BioDynaMo collaboration. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
//
// See the LICENSE file distributed with this work for details.
// See the NOTICE file distributed with this work for additional information
// regarding copyright ownership.
//
// -----------------------------------------------------------------------------
#include "cbs-covid.h"
#include "data_processing_helpers.h"

#include "core/multi_simulation/experiment.h"
#include "core/multi_simulation/multi_simulation.h"

using namespace bdm;

const ParamGroupUid SimParam::kUid = ParamGroupUidGenerator::Get()->NewUid();

void ExperimentSimAndAnalytical(int argc, const char** argv, const Param* param,
                                uint64_t repeat) {
  auto sim_wrapper = L2F([&](Param* param, TimeSeries* result) {
    Simulate(argc, argv, result, param);
  });

  auto export_results =
      L2F([&](const std::vector<TimeSeries>& results, const TimeSeries& mean,
              const TimeSeries& analytical, const Param& param, real_t err) {
        auto t = std::time(nullptr);
        auto today = *std::localtime(&t);
        std::ostringstream oss;
        oss << std::put_time(&today, "%Y-%m-%d");
        std::string experiments_output_dir =
            "output/single_experiments/" + oss.str();
        if (system(Concat("mkdir -p ", experiments_output_dir).c_str())) {
          Log::Fatal("Simulation::ExportResults",
                     "Failed to make output directory ",
                     experiments_output_dir);
        } else {
          Log::Info("Simulation::ExportResults", "Created output directory ",
                    experiments_output_dir);
        }
        ExportResults(results, mean, param, err, experiments_output_dir);
      });

  auto compute_error = L2F([&](TimeSeries observed, TimeSeries simulated) {
    return ComputeError(observed, simulated);
  });

  TimeSeries observed;
  ImportObservedData(&observed);
  real_t mse = Experiment(sim_wrapper, repeat, param, &observed, &compute_error,
                          &export_results);
  std::cout << " MSE " << mse << std::endl;
}

int main(int argc, const char** argv) {
  Param::RegisterParamGroup(new SimParam());
  auto opts = CommandLineOptions(argc, argv);
  opts.AddOption<std::string>("cbsdir", "",
                              "Full path to the population data (CSV format)");
  opts.AddOption<bool>("randominit", "false",
                       "Use random population for initialization");
  opts.AddOption<bool>("exportstats", "false",
                       "Export custom-defined simulation statistics");
  opts.AddOption<bool>("print_timings", "false",
                       "Print selected timing results per simulation");

  Simulation simulation(&opts);
  auto* param = simulation.GetParam();
  auto* sparam = param->Get<SimParam>();
  auto* opt_param = param->Get<OptimizationParam>();

  // Run the simulation once and compute the error against the observed data
  if (sparam->mode == "sim-and-analytical") {
    std::cout << "Repeat: " << sparam->repeat << std::endl;
    ExperimentSimAndAnalytical(argc, argv, param, sparam->repeat);
    std::cout << "Simulation completed successfully!" << std::endl;
    return 0;
  } else {  // Run the multi-simulation fitting routine
    // Create a timestamped output directory for the experiments as part of the
    // calibration routine (only master rank)
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    std::string experiments_output_dir =
        Concat("output/experiments_", oss.str());
    if (system(Concat("mkdir -p ", experiments_output_dir).c_str())) {
      Log::Fatal("Simulation::ExportResults",
                 "Failed to make output directory ", experiments_output_dir);
    } else {
      Log::Info("Simulation::ExportResults", "Created output directory ",
                experiments_output_dir);
    }

    // Generate the analytical data
    auto compute_error = L2F([&](TimeSeries observed, TimeSeries simulated) {
      return ComputeError(observed, simulated);
    });
    auto export_results =
        L2F([&](const std::vector<TimeSeries>& results, const TimeSeries& mean,
                const TimeSeries& analytical, const Param& param, real_t err) {
          ExportResults(results, mean, param, err, experiments_output_dir);
        });

    TimeSeries observed;
    ImportObservedData(&observed);
    MultiSimulation ms(argc, argv, &observed);
    auto ret = ms.Execute(Simulate, &compute_error, &export_results);
    std::cout << "Simulation completed successfully!" << std::endl;
    return ret;
  }
}
