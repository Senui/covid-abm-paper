#ifndef COVID_ENVIRONMENT_H_
#define COVID_ENVIRONMENT_H_

#include "core/environment/environment.h"

#include "csv_helper.h"
#include "model_facts.h"
#include "person.h"
#include "sim_param.h"

#include <string>
#include <vector>

namespace bdm {

class CovidEnvironment : public Environment {
 public:
  CovidEnvironment() {
    std::vector<std::string> filenames = {"Mix_h.csv", "Mix_o.csv", "Mix_s.csv",
                                          "Mix_w.csv", "Mix_ws.csv"};
    std::vector<Situation> situation_name = {
        Situation::kHome, Situation::kOther, Situation::kSchool,
        Situation::kWork, Situation::kWorkSchool};

    // Convert rapidcsv::Document to 2D array (indexed as [row][column])
    int idx = 0;
    for (auto file : filenames) {
      std::string data_dir = GetDataDir();
      std::string full_path = data_dir + "/" + file;
      CsvTo2DMatrix<real_t, kNumDemographies>(
          full_path, &(mixing_matrices_[situation_name[idx]]));
      idx++;
    }
    NormalizeInteractions();
  }

  // Apply normalizations on hourly interactions, such that they make sense on a daily level
  void NormalizeInteractions() {
    auto* sparam = Simulation::GetActive()->GetParam()->Get<SimParam>();
    real_t norm_factor = sparam->avg_interactions / sparam->emperical_avg_interactions;
    for (auto& mixmat : mixing_matrices_) {
      for (size_t row = 0; row < kNumDemographies; row++) {
        for (size_t col = 0; col < kNumDemographies; col++) {
          mixmat.second[row][col] = mixmat.second[row][col] * norm_factor;
        }
      }
    }
  }

  const std::array<std::array<real_t, kNumDemographies>, kNumDemographies>&
  GetMixingMatrix(Situation situation) {
    return mixing_matrices_[situation];
  }

  // Peforms a element-wise multiplication of the reduction_matrix with all
  // mixing matrices
  void ApplyReduction(const std::array<std::array<real_t, kNumDemographies>,
                                       kNumDemographies>& reduction_matrix) {
    for (auto& mixmat : mixing_matrices_) {
      assert(reduction_matrix.size() == mixmat.second.size());
      for (size_t row = 0; row < kNumDemographies; row++) {
        for (size_t col = 0; col < kNumDemographies; col++) {
          mixmat.second[row][col] = mixmat.second[row][col] * reduction_matrix[row][col];
        }
      }
    }
  }

  void Clear() override {}

  void UpdateImplementation() override {}

  std::array<int32_t, 6> GetDimensions() const override {
    return {0, 0, 0, 0, 0, 0};
  }

  std::array<int32_t, 2> GetDimensionThresholds() const override {
    return {0, 0};
  }

  LoadBalanceInfo* GetLoadBalanceInfo() override {
    Log::Fatal(
        "CovidEnvironment::GetLoadBalanceInfo",
        "You tried to call GetLoadBalanceInfo in an environment that does "
        "not support it.");
    return nullptr;
  }

  Environment::NeighborMutexBuilder* GetNeighborMutexBuilder() override {
    return nullptr;
  };

  std::unordered_map<std::string, std::vector<Agent*>> agents_per_city_;

  void ForEachNeighbor(Functor<void, Agent*>& lambda, const Agent& query,
                       void* criteria) override {}
  void ForEachNeighbor(Functor<void, Agent*, real_t>& lambda,
                       const Agent& query, real_t squared_radius) override{};

  void ForEachNeighbor(Functor<void, Agent*, real_t>& lambda,
                       const Real3& query_position, real_t squared_radius,
                       const Agent* query_agent = nullptr) override{};

 private:
  std::unordered_map<Situation, std::array<std::array<real_t, kNumDemographies>,
                                           kNumDemographies>>
      mixing_matrices_;
};

}  // namespace bdm

#endif  // COVID_ENVIRONMENT_H_
