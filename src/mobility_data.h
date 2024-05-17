#ifndef MOBILITY_DATA_H_
#define MOBILITY_DATA_H_

#include <array>
#include <vector>

#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>

#include "omp.h"

#include "person.h"

namespace bdm {

class MobilityData {
 public:
  std::vector<uint32_t> municipality_population_;
  std::vector<std::vector<float>> m_freq_;
  std::vector<std::vector<float>> m_inc_;
  std::vector<uint32_t> municipality_codes_;

  void DrawDirichlet(Person* person, std::vector<uint16_t>* other_locations);

  // Allocate random number generator
  std::vector<gsl_rng*> r_RNG;

  static MobilityData* GetInstance() {
    static MobilityData kMobility;
    return &kMobility;
  }

 private:
  MobilityData() {
    r_RNG.resize(omp_get_max_threads());
    for (auto& r : r_RNG) {
      r = gsl_rng_alloc(gsl_rng_mt19937);
    }
  };

  ~MobilityData() {
    for (auto& r : r_RNG) {
      delete(r);
    }
  }
};

inline void MobilityData::DrawDirichlet(
    Person* person, std::vector<uint16_t>* other_locations) {
  auto mobility_data = MobilityData::GetInstance();
  auto hours_not_home = other_locations->size();
  std::vector<double> alphas;
  // Based on the traveling type a person is, we assign different mobility data
  // to draw samples from using the Dirichlet distribution
  if (person->traveler_type_ == TravelerType::kFrequent) {
    const auto& v = mobility_data->m_freq_[person->home_location_];
    alphas.assign(v.begin(), v.end());
  } else {
    const auto& v = mobility_data->m_inc_[person->home_location_];
    alphas.assign(v.begin(), v.end());
  }

  // We scale the alpha value for the home municipality based on the
  // demographic group of the person
  alphas[person->home_location_] *=
      kDemographyHomeStayScaling[person->demography_];

  assert(alphas.size() == kNumMunicipalities &&
         "The size of alphas was not equal to the amount of municipalities.");

  // Normalize based on the population of the person's home municipality
  for (auto& a : alphas) {
    a /= municipality_population_[person->home_location_];
  }

  double sum_alphas = std::accumulate(alphas.begin(), alphas.end(), 0);
  for (auto& a : alphas) {
    a = (2.5 * a) / sum_alphas;
  }

  // Draw from Dirichlet distribution
  std::vector<double> results(alphas.size());
  gsl_ran_dirichlet(mobility_data->r_RNG[omp_get_thread_num()], alphas.size(), alphas.data(),
                    results.data());

  // `results` now contains a vector with values as sampled from the Dirichlet
  // distribution. Each element represents the probability that a person is in a
  // specific location. We now need to map this to hours spent in those
  // municipalities
  std::vector<int> hours_per_municipality(results.size());
  for (size_t idx = 0; idx < results.size(); idx++) {
    hours_per_municipality[idx] = std::round(kHoursPerDay * results[idx]);
  }

  int sum = std::accumulate(hours_per_municipality.begin(),
                        hours_per_municipality.end(), 0);
  for (size_t idx = 0; idx < hours_per_municipality.size(); idx++) {
    hours_per_municipality[idx] =
        (kHoursPerDay * hours_per_municipality[idx]) / sum;
  }

  // For the remaining hours spent outside the home municipality we extract the
  // hours that are non-zero and the corresponding municipality
  std::vector<uint16_t> municipalities;
  std::vector<uint16_t> hours;

  auto isnotzero = [](auto& v) { return v != 0; };

  auto it = std::find_if(hours_per_municipality.begin(),
                         hours_per_municipality.end(), isnotzero);
  while (it != hours_per_municipality.end()) {
    hours.push_back(*it);
    municipalities.push_back(std::distance(hours_per_municipality.begin(), it));
    it = std::find_if(++it, hours_per_municipality.end(), isnotzero);
  }

  // Normalize hours such that the total is 10 hours. Note that due to rounding
  // the final sum could still be more or less than 10
  sum = std::accumulate(hours.begin(), hours.end(), 0);
  for (size_t idx = 0u; idx < hours.size(); idx++) {
    hours[idx] =
        std::round(hours_not_home * (hours[idx] / static_cast<float>(sum)));
  }

  // for (size_t idx = 0u; idx < municipalities.size(); idx++) {
  //   std::cout << municipalities[idx] << ",";
  // }
  // std::cout << std::endl;
  // for (size_t idx = 0u; idx < hours.size(); idx++) {
  //   std::cout << hours[idx] << ",";
  // }
  // std::cout << std::endl;

  // If there are not exactly 10 hours spent in all the municipalities add the
  // difference to the most-spent municipality
  // If the total hours spent is more than 10 (and thus a negative difference)
  // we prune later on when populating the `other_locations` array
  sum = std::accumulate(hours.begin(), hours.end(), 0);
  if (sum < static_cast<int>(hours_not_home)) {
    auto max_spent = std::max_element(hours.begin(), hours.end());
    int difference = static_cast<int>(hours_not_home) - sum;

    *max_spent += difference;
  }

  size_t idx = 0;
  for (size_t i = 0u; i < hours.size(); i++) {
    for (size_t j = 0u; j < hours[i]; j++) {
      // Prune if we have more than 10 hours spent in all municipalities
      if (idx > hours_not_home - 1) {
        break;
      }
      (*other_locations)[idx] = municipalities[i];
      idx++;
    }
  }
  // for (size_t idx = 0u; idx < other_locations->size(); idx++) {
  //   std::cout << (*other_locations)[idx] << ",";
  // }
  // std::cout << std::endl;
}

}  // namespace bdm

#endif  // MOBILITY_DATA_
