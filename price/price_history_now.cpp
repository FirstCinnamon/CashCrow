#include <cstdlib>
#include <vector>
#include <fstream>
#include "rapidcsv.h"

// generate stock price history until now

int main(int argc, char **argv) {
  size_t minutes = atoi(argv[1]);
  char *const src = argv[2];
  char *const dest = argv[3];

  rapidcsv::Document doc(src, rapidcsv::LabelParams(-1, -1));
  std::vector<std::string> time = doc.GetColumn<std::string>(0);
  std::vector<std::string> price = doc.GetColumn<std::string>(1);

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
    for (size_t i = 0; i <= minutes; i++) {
      write_file << time[i] + "," + price[i] + "\n";
    }
  }

  write_file.close();

  return 0;
}