#include <gtest/gtest.h>
#include "biodynamo.h"

#include "data_processing_helpers.h"

#define TEST_NAME typeid(*this).name()

namespace bdm {

using experimental::TimeSeries;

TEST(CbsCovid, ImportObservedData) {
  TimeSeries ts;
  ImportObservedData(&ts);

  auto data = ts.GetYValues("observed_hospitalization");

  EXPECT_EQ(15u, data.size());
}

}  // namespace bdm
