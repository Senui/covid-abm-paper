#include <gtest/gtest.h>
#include "biodynamo.h"

#include "csv_helper.h"
#include "model_facts.h"

#define TEST_NAME typeid(*this).name()

namespace bdm {

TEST(CsvHelper, CsvTo2DMatrix) {
  std::vector<std::vector<float>> m_freq_;
  std::vector<std::vector<float>> m_inc_;
  std::string data_dir = GetDataDir();

  // Read in M_freq.csv
  std::string full_path = data_dir + "/M_freq.csv";
  CsvTo2DMatrix(full_path, &m_freq_);

  // Read in M_inc.csv
  full_path = data_dir + "/M_inc.csv";
  CsvTo2DMatrix(full_path, &m_inc_);

  EXPECT_EQ(kNumMunicipalities, m_freq_.size());
  EXPECT_EQ(kNumMunicipalities, m_freq_[0].size());
  EXPECT_NEAR(920436.79643959f, m_freq_[1][0], 1e-9);
  EXPECT_NEAR(920436.79643959f, m_freq_[0][1], 1e-9);
  EXPECT_NEAR(7118231.18197822f, m_freq_[7][1], 1e-9);

  EXPECT_EQ(kNumMunicipalities, m_inc_.size());
  EXPECT_EQ(kNumMunicipalities, m_inc_[0].size());
  EXPECT_NEAR(3542074.60603644f, m_inc_[1][0], 1e-9);
  EXPECT_NEAR(3542074.60603644f, m_inc_[0][1], 1e-9);
  EXPECT_NEAR(20359646.4586547f, m_inc_[7][1], 1e-9);
}

TEST(CsvHelper, CsvToVector) {
  std::vector<uint32_t> municipality_population_;
  std::string data_dir = GetDataDir();
  std::string full_path = data_dir + "/inwoners_gemeente_2018.csv";
  CsvToVector(full_path, &municipality_population_, 1, 0);

  EXPECT_EQ(25390u, municipality_population_[0]);
  EXPECT_EQ(kNumMunicipalities, municipality_population_.size());
}

}  // namespace bdm
