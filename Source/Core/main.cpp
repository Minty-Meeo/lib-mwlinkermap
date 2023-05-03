#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>

#include "MWLinkerMap.h"
#include "PrintUtil.h"

#define TIME_ATTACK_COUNT 20

static void tempfunc(const char* name, int choice)
{
  util::println(std::cout, "{:s}", name);

  std::ifstream infile(name);
  if (!infile.is_open())
  {
    util::println(std::cerr, "Could not open!");
    return;
  }
  std::stringstream sstream;
  sstream << infile.rdbuf();
  std::string temp = std::move(sstream).str();

  MWLinker::Map linker_map;
  MWLinker::Map::SymbolClosure::Warn::do_warn_odr_violation = false;
  MWLinker::Map::SymbolClosure::Warn::do_warn_sym_on_flag_detected = false;
  MWLinker::Map::EPPC_PatternMatching::Warn::do_warn_folding_odr_violation = false;
  MWLinker::Map::EPPC_PatternMatching::Warn::do_warn_folding_repeat_object = false;
  MWLinker::Map::EPPC_PatternMatching::Warn::do_warn_merging_odr_violation = false;
  MWLinker::Map::SectionLayout::Warn::do_warn_lcomm_after_comm = false;
  MWLinker::Map::SectionLayout::Warn::do_warn_odr_violation = false;
  MWLinker::Map::SectionLayout::Warn::do_warn_repeat_compilation_unit = false;
  MWLinker::Map::SectionLayout::Warn::do_warn_sym_on_flag_detected = false;

  std::size_t scan_line_number = 0;
  MWLinker::Map::Error error = MWLinker::Map::Error::None;
  std::array<std::chrono::milliseconds, TIME_ATTACK_COUNT> time_attack{};
  for (std::size_t i = 0; i < TIME_ATTACK_COUNT; ++i)
  {
    linker_map = MWLinker::Map{};  // Reset linker map
    const auto time_start = std::chrono::high_resolution_clock::now();
    switch (choice)
    {
    case 0:
      error = linker_map.Scan(temp, scan_line_number);
      break;
    case 1:
      error = linker_map.ScanTLOZTP(temp, scan_line_number);
      break;
    case 2:
      error = linker_map.ScanSMGalaxy(temp, scan_line_number);
      break;
    default:
      util::println(std::cerr, "bad choice");
      return;
    }
    const auto time_end = std::chrono::high_resolution_clock::now();
    time_attack[i] = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start);
  }

  while (temp.back() == '\0')
    temp.pop_back();

  std::size_t print_line_number = 0;
  linker_map.Print(sstream, print_line_number);

  // std::ofstream outfile{std::string{name} + "_NEW", std::ios_base::binary};
  // outfile << sstream.rdbuf();
  // outfile.close();

  std::string temp2 = std::move(sstream).str();

  const bool matches = (temp == temp2);
  // This clobbering to check while debugging that I made no ownership mistakes. Some portions of
  // the linker map have a lookup map which have std::string_view keys.  The owners of the memory
  // containing the string is meant to be the unit within the linker map portion / the value stored
  // by reference in the map.
  std::fill(temp.begin(), temp.end(), '\0');
  const MWLinker::Version min_version = linker_map.GetMinVersion();
  util::println(
      std::cout,
      "scan line: {:d}   print line: {:d}   err: {:d}   matches: {:s}   min_version: {:d}  "
      " time: {:d}ms",
      scan_line_number, print_line_number, static_cast<int>(error), matches,
      static_cast<int>(min_version),
      (std::accumulate(time_attack.begin(), time_attack.end(), std::chrono::milliseconds{}) /
       time_attack.size())
          .count());
}

int main(const int argc, const char** argv)
{
  if (argc < 2)
  {
    util::println(std::cerr, "Provide the name");
    return EXIT_FAILURE;
  }
  if (argc < 3)
  {
    tempfunc(argv[1], 0);
  }
  else
  {
    if (!std::strcmp(argv[1], "tloztp"))
      tempfunc(argv[2], 1);
    else if (!std::strcmp(argv[1], "smgalaxy"))
      tempfunc(argv[2], 2);
    else
      tempfunc(argv[1], 0);
  }

  return EXIT_SUCCESS;
}
