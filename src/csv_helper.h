#ifndef CSV_HELPER_H_
#define CSV_HELPER_H_

#include "core/util/csv_reader.h"
#include "core/util/log.h"

#ifdef __APPLE__
#ifdef _LIBCPP_DEPRECATED_EXPERIMENTAL_FILESYSTEM
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::__fs::filesystem;
#endif
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif


namespace bdm {

// Reads in a CSV and converts it to a C++ 2D vector
template <typename T>
inline void CsvTo2DMatrix(const std::string& file_path,
                          std::vector<std::vector<T>>* matrix,
                          int skip_header = -1) {
  if (!fs::exists(file_path)) {
    Log::Fatal("CsvTo2DMatrix", "File not found: ", file_path);
  }
  rapidcsv::ConverterParams converter(true);
  rapidcsv::SeparatorParams separator;
  rapidcsv::LabelParams labels(skip_header);  // no header

  auto doc = rapidcsv::Document(file_path, labels, separator, converter);
  auto num_rows = doc.GetRowCount();

  for (size_t row = 0; row < num_rows; row++) {
    matrix->push_back(doc.GetRow<T>(row));
  }
}

// Reads in a CSV and converts it to a C++ 2D array
template <typename T, int size>
inline void CsvTo2DMatrix(const std::string& file_path,
                          std::array<std::array<T, size>, size>* matrix,
                          int skip_header = -1) {
  if (!fs::exists(file_path)) {
    Log::Fatal("CsvTo2DMatrix", "File not found: ", file_path);
  }
  rapidcsv::ConverterParams converter(true);
  rapidcsv::SeparatorParams separator;
  rapidcsv::LabelParams labels(skip_header);  // no header

  auto doc = rapidcsv::Document(file_path, labels, separator, converter);
  auto num_rows = doc.GetRowCount();
  auto num_cols = doc.GetColumnCount();

  if (static_cast<int>(num_rows) != size ||
      static_cast<int>(num_cols) != size) {
    Log::Fatal("CsvTo2DMatrix", "Size mismatch: trying to read a (", num_rows,
               "x", num_rows, ") CSV matrix into a (", size, "x", size,
               ") C++ 2D array");
  }

  for (size_t row = 0; row < num_rows; row++) {
    for (size_t col = 0; col < num_cols; col++) {
      if (std::isnan(doc.GetCell<T>(col, row))) {
        Log::Warning("CsvTo2DMatrix", "Found NaN value in ", file_path, ". Entry (", row, ",", col, ")");
      }
      (*matrix)[row][col] = doc.GetCell<T>(col, row);
    }
  }
}

// Reads in a CSV and converts it to a C++ 1D vector
template <typename T>
inline void CsvToVector(const std::string& file_path, std::vector<T>* vector,
                        size_t column = 0, int skip_header = -1) {
  if (!fs::exists(file_path)) {
    Log::Fatal("CsvToVector", "File not found: ", file_path);
  }
  rapidcsv::ConverterParams converter(true);
  rapidcsv::SeparatorParams separator;
  rapidcsv::LabelParams labels(skip_header);  // no header

  auto doc = rapidcsv::Document(file_path, labels, separator, converter);

  *vector = doc.GetColumn<T>(column);
}

}  // namespace bdm

#endif  // CSV_HELPER_H_
