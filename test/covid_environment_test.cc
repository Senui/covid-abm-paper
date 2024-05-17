#include <gtest/gtest.h>
#include "biodynamo.h"
#include "unit/test_util/test_util.h"

#include "covid_environment.h"
#include "interventions.h"
#include "model_facts.h"

#define TEST_NAME typeid(*this).name()

namespace bdm {

TEST(CovidEnvironment, AdjustMixingMatrices) {
  Simulation simulation(TEST_NAME);

  CovidEnvironment* env = new CovidEnvironment();

  simulation.SetEnvironment(env);

  auto& mixmat_home = env->GetMixingMatrix(kHome);
  auto& mixmat_work = env->GetMixingMatrix(kWork);
  auto& mixmat_other = env->GetMixingMatrix(kOther);
  auto& mixmat_school = env->GetMixingMatrix(kSchool);
  auto& mixmat_workschool = env->GetMixingMatrix(kWorkSchool);

  auto eps = abs_error<real_t>::value;
  EXPECT_NEAR(6.874619913482362676e-01, mixmat_home[0][0], eps);
  EXPECT_NEAR(6.997546340816274135e-02, mixmat_work[3][2], eps);
  EXPECT_NEAR(1.474070253869433467e+00, mixmat_other[10][6], eps);
  EXPECT_NEAR(1.884050302777406927e+00, mixmat_school[2][1], eps);
  EXPECT_NEAR(2.707680440007707912e+00, mixmat_workschool[1][1], eps);

  AdjustMixingMatrices(0);

  EXPECT_NEAR(6.874619913482362676e-01 * 1.915564191997508603e-01, mixmat_home[0][0], eps);
  EXPECT_NEAR(6.997546340816274135e-02 * 2.490211607097619906e-01, mixmat_work[3][2], eps);

  AdjustMixingMatrices(2);

  EXPECT_NEAR(6.874619913482362676e-01 * 1.915564191997508603e-01 * 1.553163510433659189e+00, mixmat_home[0][0], eps);
  EXPECT_NEAR(6.997546340816274135e-02 * 2.490211607097619906e-01 * 5.313340380932151108e-01, mixmat_work[3][2], eps);
}

}  // namespace bdm
