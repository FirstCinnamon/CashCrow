#include <vector>
#include <fstream>
#include "rapidcsv.h"

// from raw data to eligible format data

int main(int argc, char **argv) {
  char *const src = argv[1];
  char *const dest = argv[2];

  rapidcsv::Document doc(src, rapidcsv::LabelParams(-1, -1));
  std::vector<std::string> time = doc.GetColumn<std::string>(1);
  std::vector<std::string> price = doc.GetColumn<std::string>(2);

  // assert that two vector lengths are the same
  if (time.size() != price.size()) {
    return -1; // parse error
  }

  // remove existing file if any
  int _ = remove(dest);

  std::ofstream write_file;
  write_file.open(dest, std::ios_base::app);

  if (!write_file) { // file creation error
    return -2;
  } else {
    for (size_t i = 0; i < time.size(); i++) {
      write_file << time[i] + "," + price[i] + "\n";
    }
  }

  write_file.close();

  return 0;
}