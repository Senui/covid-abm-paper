#include "person.h"
#include "csv_helper.h"
#include "sim_param.h"

#include <json.hpp>

using namespace bdm;
using bdm::experimental::TimeSeries;
using nlohmann::json;

inline void ImportObservedData(TimeSeries* observed) {
  auto file = GetDataDir() + "/observed_hospital_doubling.csv";
  std::vector<real_t> observed_vec;
  CsvToVector(file, &observed_vec);
  observed->Add("observed_hospitalization", {}, observed_vec);
}

// Only use the simulated hospitalization values at 0:00 each day and only
// between 13 and 27 March, to correspond with the observed data interval. There
// are 384 hours between 28 Feb 2020 and 14 March 2020 There are 696 hours
// between 27 Feb 2020 and 27 March 2020. We add an additional 24 hours to
// get the hospitalized results at the end of each day
inline real_t ComputeError(TimeSeries observed, TimeSeries simulated) {
  auto simulated_vec = simulated.GetYValues("ts_hospitalized");

  std::vector<real_t> simulated_doubling_hospitalization;
  for (size_t t = 0; t < simulated_vec.size(); t++) {
    if (t % kHoursPerDay == 0 && t >= 384 && t <= 720) {
      simulated_doubling_hospitalization.push_back(simulated_vec[t]);
    }
  }

  real_t err = std::numeric_limits<real_t>::infinity();
  auto observed_vec = observed.GetYValues("observed_hospitalization");
  if (observed_vec.size() != simulated_doubling_hospitalization.size()) {
    Log::Warning("ComputeError",
                 "The observed data and the simulated data are of different "
                 "size. Respectively, ",
                 observed_vec.size(), " and ",
                 simulated_doubling_hospitalization.size(), ".");
    return err;
  }

  std::cout << "simulated hosp: ";
  for (size_t t = 0; t < simulated_doubling_hospitalization.size(); t++) {
    std::cout << simulated_doubling_hospitalization[t] << ",";
  }
  std::cout << std::endl;

  std::cout << "observed hosp: ";
  for (size_t t = 0; t < observed_vec.size(); t++) {
    std::cout << observed_vec[t] << ",";
  }
  std::cout << std::endl;

  err = Math::MSE(observed_vec, simulated_doubling_hospitalization);
  return err;
}

inline void ExportResults(const std::vector<TimeSeries>& results,
                          const TimeSeries& mean, const Param& param,
                          real_t err,
                          const std::string& experiments_output_dir) {
  // Create UUID for the experiment
  TUUID tu;
  std::string uuid = std::string(tu.AsString());

  std::string experiment_output_dir = Concat(experiments_output_dir, "/", uuid);
  if (system(Concat("mkdir -p ", experiment_output_dir).c_str())) {
    Log::Fatal("Simulation::ExportResults", "Failed to make output directory ",
               experiment_output_dir);
  } else {
    Log::Info("Simulation::ExportResults", "Created output directory ",
              experiment_output_dir);
  }

  // Save all collectors to file
  mean.SaveCsv(experiment_output_dir);

  // Add additional parameters of interests to parameter file
  auto j_param = json::parse(param.ToJsonString());
  j_param["mse"] = err;
  j_param["repetitions"] = results[0].GetYValues("repetitions")[0];
  j_param["resolution"] = results[0].GetYValues("resolution")[0];

  std::ofstream param_file;
  param_file.open(Concat(experiment_output_dir, "/param.json"));
  param_file << j_param.dump(4);
  param_file .close();
}
