#include "MWLinkerMap.h"

#include <cassert>
#include <cstddef>
#include <istream>
#include <ostream>
#include <ranges>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <typeinfo>
#include <unordered_map>
#include <utility>

#include <format>
#include <iostream>

#define xstoul(__s) std::stoul(__s, nullptr, 16)

namespace Common
{
template <class... Args>
void Print(std::ostream& os, std::format_string<Args...> fmt, Args&&... args)
{
  const std::string s = std::format(std::move(fmt), args...);
  os.write(s.c_str(), std::ssize(s));
}
}  // namespace Common

// Metrowerks linker maps should be considered binary files containing text with CRLF line endings.
// To account for outside factors, though, this program can support both CRLF and LF line endings.

namespace MWLinker
{
Map::Error Map::Scan(std::istream& stream, std::size_t& line_number)
{
  std::stringstream sstream;
  sstream << stream.rdbuf();
  return this->Scan(sstream, line_number);
}
Map::Error Map::Scan(const std::stringstream& sstream, std::size_t& line_number)
{
  // TODO: donkey dick
  return this->Scan(std::move(sstream).str(), line_number);
}
Map::Error Map::Scan(const std::string_view string_view, std::size_t& line_number)
{
  return this->Scan(string_view.data(), string_view.data() + string_view.length(), line_number);
}

Map::Error Map::ScanTLOZTP(std::istream& stream, std::size_t& line_number)
{
  std::stringstream sstream;
  sstream << stream.rdbuf();
  return this->ScanTLOZTP(sstream, line_number);
}
Map::Error Map::ScanTLOZTP(const std::stringstream& sstream, std::size_t& line_number)
{
  // TODO: donkey dick
  return this->ScanTLOZTP(std::move(sstream).str(), line_number);
}
Map::Error Map::ScanTLOZTP(const std::string_view string_view, std::size_t& line_number)
{
  return this->ScanTLOZTP(string_view.data(), string_view.data() + string_view.length(),
                          line_number);
}

Map::Error Map::ScanSMGalaxy(std::istream& stream, std::size_t& line_number)
{
  std::stringstream sstream;
  sstream << stream.rdbuf();
  return this->ScanSMGalaxy(sstream, line_number);
}
Map::Error Map::ScanSMGalaxy(const std::stringstream& sstream, std::size_t& line_number)
{
  // TODO: donkey dick
  return this->ScanSMGalaxy(std::move(sstream).str(), line_number);
}
Map::Error Map::ScanSMGalaxy(const std::string_view string_view, std::size_t& line_number)
{
  return this->ScanSMGalaxy(string_view.data(), string_view.data() + string_view.length(),
                            line_number);
}

// clang-format off
static const std::regex re_entry_point_name{
//  "Link map of %s\r\n"
    "Link map of (.*)\r?\n"};
static const std::regex re_unresolved_symbol{
//  ">>> SYMBOL NOT FOUND: %s\r\n"
    ">>> SYMBOL NOT FOUND: (.*)\r?\n"};
static const std::regex re_mixed_mode_islands_header{
//  "\r\nMixed Mode Islands\r\n"
    "\r?\nMixed Mode Islands\r?\n"};
static const std::regex re_branch_islands_header{
//  "\r\nBranch Islands\r\n"
    "\r?\nBranch Islands\r?\n"};
static const std::regex re_linktime_size_decreasing_optimizations_header{
//  "\r\nLinktime size-decreasing optimizations\r\n"
    "\r?\nLinktime size-decreasing optimizations\r?\n"};
static const std::regex re_linktime_size_increasing_optimizations_header{
//  "\r\nLinktime size-increasing optimizations\r\n"
    "\r?\nLinktime size-increasing optimizations\r?\n"};
static const std::regex re_section_layout_header{
//  "\r\n\r\n%s section layout\r\n"
    "\r?\n\r?\n(.*) section layout\r?\n"};
static const std::regex re_section_layout_header_modified_a{
    "\r?\n(.*) section layout\r?\n"};
static const std::regex re_section_layout_header_modified_b{
    "(.*) section layout\r?\n"};
static const std::regex re_memory_map_header{
//  "\r\n\r\nMemory map:\r\n"
    "\r?\n\r?\nMemory map:\r?\n"};
static const std::regex re_linker_generated_symbols_header{
//  "\r\n\r\nLinker generated symbols:\r\n"
    "\r?\n\r?\nLinker generated symbols:\r?\n"};
// clang-format on

// Other linker map prints are known to exist, but have never been seen. These include:
// ">>> EXCLUDED SYMBOL %s (%s,%s) found in %s %s\r\n"
// ">>> %s wasn't passed a section\r\n"
// ">>> DYNAMIC SYMBOL: %s referenced\r\n"
// ">>> MODULE SYMBOL NAME TOO LARGE: %s\r\n"
// ">>> NONMODULE SYMBOL NAME TOO LARGE: %s\r\n"
// "<<< Failure in ComputeSizeETI: section->Header->sh_size was %x, rel_size should be %x\r\n"
// "<<< Failure in ComputeSizeETI: st_size was %x, st_size should be %x\r\n"
// "<<< Failure in PreCalculateETI: section->Header->sh_size was %x, rel_size should be %x\r\n"
// "<<< Failure in PreCalculateETI: st_size was %x, st_size should be %x\r\n"
// "<<< Failure in %s: GetFilePos is %x, sect->calc_offset is %x\r\n"
// "<<< Failure in %s: GetFilePos is %x, sect->bin_offset is %x\r\n"

Map::Error Map::Scan(const char* head, const char* const tail, std::size_t& line_number)
{
  if (head == nullptr || tail == nullptr || head > tail)
    return Error::Fail;

  std::cmatch match;
  line_number = 0;

  // Linker maps from Animal Crossing (foresta.map and static.map) and Doubutsu no Mori e+
  // (foresta.map, forestd.map, foresti.map, foresto.map, and static.map) appear to have been
  // modified to strip out the Link Map portion and UNUSED symbols, though the way it was done
  // also removed one of the Section Layout header's preceding newlines.
  if (std::regex_search(head, tail, match, re_section_layout_header_modified_a,
                        std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    const auto error = this->ScanPrologue_SectionLayout(head, tail, line_number, match.str(1));
    if (error != Error::None)
      return error;
    goto NINTENDO_EAD_TRIMMED_LINKER_MAPS_SKIP_TO_HERE;
  }
  // Linker maps from Doubutsu no Mori + (foresta.map2 and static.map2) are modified similarly
  // to their counterparts in Doubutsu no Mori e+, though now with no preceding newlines. The
  // unmodified linker maps were also left on the disc, so maybe just use those instead?
  // Similarly modified linker maps:
  //   The Legend of Zelda - Ocarina of Time & Master Quest
  //   The Legend of Zelda - The Wind Waker (framework.map)
  if (std::regex_search(head, tail, match, re_section_layout_header_modified_b,
                        std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    const auto error = this->ScanPrologue_SectionLayout(head, tail, line_number, match.str(1));
    if (error != Error::None)
      return error;
    goto NINTENDO_EAD_TRIMMED_LINKER_MAPS_SKIP_TO_HERE;
  }
  if (std::regex_search(head, tail, match, re_entry_point_name,
                        std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    this->entry_point_name = match.str(1);
  }
  else
  {
    // If this is not present, the file must not be a Metrowerks linker map.
    return Error::EntryPointNameMissing;
  }
  {
    auto portion = std::make_unique<SymbolClosure>();
    const auto error = portion->Scan(head, tail, line_number, this->unresolved_symbols);
    if (error != Error::None)
      return error;
    if (!portion->Empty())
      this->normal_symbol_closure = std::move(portion);
  }
  {
    auto portion = std::make_unique<EPPC_PatternMatching>();
    const auto error = portion->Scan(head, tail, line_number);
    if (error != Error::None)
      return error;
    if (!portion->Empty())
      this->eppc_pattern_matching = std::move(portion);
  }
  // With '-listdwarf' and DWARF debugging information enabled, a second symbol closure
  // containing info about the .dwarf and .debug sections will appear. Note that, without an
  // EPPC_PatternMatching in the middle, this will blend into the prior symbol closure in the
  // eyes of this scan function.
  {
    auto portion = std::make_unique<SymbolClosure>();
    portion->SetMinVersion(Version::version_3_0_4);
    const auto error = portion->Scan(head, tail, line_number, this->unresolved_symbols);
    if (error != Error::None)
      return error;
    if (!portion->Empty())
      this->dwarf_symbol_closure = std::move(portion);
  }
  // Unresolved symbol post-prints probably belong here (I have not confirmed if they preceed
  // LinkerOpts), but the Symbol Closure scanning code that just happened handles them well enough.
  {
    auto portion = std::make_unique<LinkerOpts>();
    const auto error = portion->Scan(head, tail, line_number);
    if (error != Error::None)
      return error;
    if (!portion->Empty())
      this->linker_opts = std::move(portion);
  }
  if (std::regex_search(head, tail, match, re_mixed_mode_islands_header,
                        std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    auto portion = std::make_unique<MixedModeIslands>();
    const auto error = portion->Scan(head, tail, line_number);
    if (error != Error::None)
      return error;
    this->mixed_mode_islands = std::move(portion);
  }
  if (std::regex_search(head, tail, match, re_branch_islands_header,
                        std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    auto portion = std::make_unique<BranchIslands>();
    const auto error = portion->Scan(head, tail, line_number);
    if (error != Error::None)
      return error;
    this->branch_islands = std::move(portion);
  }
  if (std::regex_search(head, tail, match, re_linktime_size_decreasing_optimizations_header,
                        std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    auto portion = std::make_unique<LinktimeSizeDecreasingOptimizations>();
    const auto error = portion->Scan(head, tail, line_number);
    if (error != Error::None)
      return error;
    this->linktime_size_decreasing_optimizations = std::move(portion);
  }
  if (std::regex_search(head, tail, match, re_linktime_size_increasing_optimizations_header,
                        std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    auto portion = std::make_unique<LinktimeSizeIncreasingOptimizations>();
    const auto error = portion->Scan(head, tail, line_number);
    if (error != Error::None)
      return error;
    this->linktime_size_increasing_optimizations = std::move(portion);
  }
NINTENDO_EAD_TRIMMED_LINKER_MAPS_SKIP_TO_HERE:
  while (std::regex_search(head, tail, match, re_section_layout_header,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    const auto error = this->ScanPrologue_SectionLayout(head, tail, line_number, match.str(1));
    if (error != Error::None)
      return error;
  }
  if (std::regex_search(head, tail, match, re_memory_map_header,
                        std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    const auto error = this->ScanPrologue_MemoryMap(head, tail, line_number);
    if (error != Error::None)
      return error;
  }
  if (std::regex_search(head, tail, match, re_linker_generated_symbols_header,
                        std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    auto portion = std::make_unique<LinkerGeneratedSymbols>();
    const auto error = portion->Scan(head, tail, line_number);
    if (error != Error::None)
      return error;
    this->linker_generated_symbols = std::move(portion);
  }
  return this->ScanForGarbage(head, tail);
}

Map::Error Map::ScanTLOZTP(const char* head, const char* const tail, std::size_t& line_number)
{
  if (head == nullptr || tail == nullptr || head > tail)
    return Error::Fail;

  std::cmatch match;
  line_number = 0;

  this->entry_point_name = "__start";
  // The Legend of Zelda: Twilight Princess features CodeWarrior for GCN 2.7 linker maps that have
  // been post-processed to appear similar to older linker maps. Nintendo EAD probably did this to
  // procrastinate updating the JUTException library. These linker maps contain prologue-free,
  // three-column section layout portions, and nothing else. Also, not that it matters to this
  // scan function, the line endings of the linker maps left on disc were Unix style (LF).
  while (std::regex_search(head, tail, match, re_section_layout_header_modified_b,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    auto portion = std::make_unique<SectionLayout>(match.str(1));
    portion->SetMinVersion(Version::version_3_0_4);
    const auto error = portion->ScanTLOZTP(head, tail, line_number);
    if (error != Error::None)
      return error;
    this->section_layouts.push_back(std::move(portion));
  }
  return this->ScanForGarbage(head, tail);
}

Map::Error Map::ScanSMGalaxy(const char* head, const char* const tail, std::size_t& line_number)
{
  if (head == nullptr || tail == nullptr || head > tail)
    return Error::Fail;

  std::cmatch match;
  line_number = 0;

  // We only see this header once, as every symbol is mashed into an imaginary ".text" section.
  if (!std::regex_search(head, tail, match, re_section_layout_header_modified_a,
                         std::regex_constants::match_continuous))
    return Error::SMGalaxyYouHadOneJob;
  {
    line_number += 1;
    head += match.length();
    auto portion = std::make_unique<SectionLayout>(match.str(1));
    portion->SetMinVersion(Version::version_3_0_4);
    const auto error = portion->Scan4Column(head, tail, line_number);
    if (error != Error::None)
      return error;
    this->section_layouts.push_back(std::move(portion));
  }
  // It seems like a mistake, but for a few examples, a tiny bit of simple-style,
  // headerless, CodeWarrior for Wii 1.0 (at minimum) Memory Map can be found.
  {
    auto portion = std::make_unique<MemoryMap>(false, false, false);
    const auto error = portion->ScanSimple(head, tail, line_number);
    if (error != Error::None)
      return error;
    if (!portion->Empty())
      this->memory_map = std::move(portion);
  }
  return this->ScanForGarbage(head, tail);
}

void Map::Print(std::ostream& stream) const
{
  // "Link map of %s\r\n"
  Common::Print(stream, "Link map of {:s}\r\n", entry_point_name);
  if (normal_symbol_closure)
    normal_symbol_closure->Print(stream);
  if (eppc_pattern_matching)
    eppc_pattern_matching->Print(stream);
  if (dwarf_symbol_closure)
    dwarf_symbol_closure->Print(stream);
  for (const auto& name : unresolved_symbols)
    // ">>> SYMBOL NOT FOUND: %s\r\n"
    Common::Print(stream, ">>> SYMBOL NOT FOUND: {:s}\r\n", name);
  if (linker_opts)
    linker_opts->Print(stream);
  if (mixed_mode_islands)
    mixed_mode_islands->Print(stream);
  if (branch_islands)
    branch_islands->Print(stream);
  if (linktime_size_decreasing_optimizations)
    linktime_size_decreasing_optimizations->Print(stream);
  if (linktime_size_increasing_optimizations)
    linktime_size_increasing_optimizations->Print(stream);
  for (const auto& section_layout : section_layouts)
    section_layout->Print(stream);
  if (memory_map)
    memory_map->Print(stream);
  if (linker_generated_symbols)
    linker_generated_symbols->Print(stream);
}

Version Map::GetMinVersion() const noexcept
{
  Version min_version = std::max({
      normal_symbol_closure ? normal_symbol_closure->min_version : Version::Unknown,
      eppc_pattern_matching ? eppc_pattern_matching->min_version : Version::Unknown,
      dwarf_symbol_closure ? dwarf_symbol_closure->min_version : Version::Unknown,
      linker_opts ? linker_opts->min_version : Version::Unknown,
      mixed_mode_islands ? mixed_mode_islands->min_version : Version::Unknown,
      branch_islands ? branch_islands->min_version : Version::Unknown,
      linktime_size_decreasing_optimizations ? linktime_size_decreasing_optimizations->min_version :
                                               Version::Unknown,
      linktime_size_increasing_optimizations ? linktime_size_increasing_optimizations->min_version :
                                               Version::Unknown,
      memory_map ? memory_map->min_version : Version::Unknown,
      linker_generated_symbols ? linker_generated_symbols->min_version : Version::Unknown,
  });
  for (const auto& section_layout : section_layouts)
    min_version = std::max(section_layout->min_version, min_version);
  return min_version;
}

// clang-format off
static const std::regex re_section_layout_3column_prologue_1{
//  "  Starting        Virtual\r\n"
    "  Starting        Virtual\r?\n"};
static const std::regex re_section_layout_3column_prologue_2{
//  "  address  Size   address\r\n"
    "  address  Size   address\r?\n"};
static const std::regex re_section_layout_3column_prologue_3{
//  "  -----------------------\r\n"
    "  -----------------------\r?\n"};
static const std::regex re_section_layout_4column_prologue_1{
//  "  Starting        Virtual  File\r\n"
    "  Starting        Virtual  File\r?\n"};
static const std::regex re_section_layout_4column_prologue_2{
//  "  address  Size   address  offset\r\n"
    "  address  Size   address  offset\r?\n"};
static const std::regex re_section_layout_4column_prologue_3{
//  "  ---------------------------------\r\n"
    "  ---------------------------------\r?\n"};
// clang-format on

Map::Error Map::ScanPrologue_SectionLayout(const char*& head, const char* const tail,
                                           std::size_t& line_number, std::string name)
{
  std::cmatch match;

  if (std::regex_search(head, tail, match, re_section_layout_3column_prologue_1,
                        std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    if (std::regex_search(head, tail, match, re_section_layout_3column_prologue_2,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      if (std::regex_search(head, tail, match, re_section_layout_3column_prologue_3,
                            std::regex_constants::match_continuous))
      {
        line_number += 1;
        head += match.length();
        auto portion = std::make_unique<SectionLayout>(std::move(name));
        const auto error = portion->Scan3Column(head, tail, line_number);
        if (error != Error::None)
          return error;
        this->section_layouts.push_back(std::move(portion));
      }
      else
      {
        return Error::SectionLayoutBadPrologue;
      }
    }
    else
    {
      return Error::SectionLayoutBadPrologue;
    }
  }
  else if (std::regex_search(head, tail, match, re_section_layout_4column_prologue_1,
                             std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    if (std::regex_search(head, tail, match, re_section_layout_4column_prologue_2,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      if (std::regex_search(head, tail, match, re_section_layout_4column_prologue_3,
                            std::regex_constants::match_continuous))
      {
        line_number += 1;
        head += match.length();
        auto portion = std::make_unique<SectionLayout>(std::move(name));
        portion->SetMinVersion(Version::version_3_0_4);
        const auto error = portion->Scan4Column(head, tail, line_number);
        if (error != Error::None)
          return error;
        this->section_layouts.push_back(std::move(portion));
      }
      else
      {
        return Error::SectionLayoutBadPrologue;
      }
    }
    else
    {
      return Error::SectionLayoutBadPrologue;
    }
  }
  else
  {
    return Error::SectionLayoutBadPrologue;
  }
  return Error::None;
}

// clang-format off
static const std::regex re_memory_map_simple_prologue_1_old{
//  "                   Starting Size     File\r\n"
    "                   Starting Size     File\r?\n"};
static const std::regex re_memory_map_simple_prologue_2_old{
//  "                   address           Offset\r\n"
    "                   address           Offset\r?\n"};
static const std::regex re_memory_map_romram_prologue_1_old{
//  "                   Starting Size     File     ROM      RAM Buffer\r\n"
    "                   Starting Size     File     ROM      RAM Buffer\r?\n"};
static const std::regex re_memory_map_romram_prologue_2_old{
//  "                   address           Offset   Address  Address\r\n"
    "                   address           Offset   Address  Address\r?\n"};
static const std::regex re_memory_map_simple_prologue_1{
//  "                       Starting Size     File\r\n"
    "                       Starting Size     File\r?\n"};
static const std::regex re_memory_map_simple_prologue_2{
//  "                       address           Offset\r\n"
    "                       address           Offset\r?\n"};
static const std::regex re_memory_map_romram_prologue_1{
//  "                       Starting Size     File     ROM      RAM Buffer\r\n"
    "                       Starting Size     File     ROM      RAM Buffer\r?\n"};
static const std::regex re_memory_map_romram_prologue_2{
//  "                       address           Offset   Address  Address\r\n"
    "                       address           Offset   Address  Address\r?\n"};
static const std::regex re_memory_map_srecord_prologue_1{
//  "                       Starting Size     File       S-Record\r\n"
    "                       Starting Size     File       S-Record\r?\n"};
static const std::regex re_memory_map_srecord_prologue_2{
//  "                       address           Offset     Line\r\n"
    "                       address           Offset     Line\r?\n"};
static const std::regex re_memory_map_binfile_prologue_1{
//  "                       Starting Size     File     Bin File Bin File\r\n"
    "                       Starting Size     File     Bin File Bin File\r?\n"};
static const std::regex re_memory_map_binfile_prologue_2{
//  "                       address           Offset   Offset   Name\r\n"
    "                       address           Offset   Offset   Name\r?\n"};
static const std::regex re_memory_map_romram_srecord_prologue_1{
//  "                       Starting Size     File     ROM      RAM Buffer  S-Record\r\n"
    "                       Starting Size     File     ROM      RAM Buffer  S-Record\r?\n"};
static const std::regex re_memory_map_romram_srecord_prologue_2{
//  "                       address           Offset   Address  Address     Line\r\n"
    "                       address           Offset   Address  Address     Line\r?\n"};
static const std::regex re_memory_map_romram_binfile_prologue_1{
//  "                       Starting Size     File     ROM      RAM Buffer Bin File Bin File\r\n"
    "                       Starting Size     File     ROM      RAM Buffer Bin File Bin File\r?\n"};
static const std::regex re_memory_map_romram_binfile_prologue_2{
//  "                       address           Offset   Address  Address    Offset   Name\r\n"
    "                       address           Offset   Address  Address    Offset   Name\r?\n"};
static const std::regex re_memory_map_srecord_binfile_prologue_1{
//  "                       Starting Size     File        S-Record Bin File Bin File\r\n"
    "                       Starting Size     File        S-Record Bin File Bin File\r?\n"};
static const std::regex re_memory_map_srecord_binfile_prologue_2{
//  "                       address           Offset      Line     Offset   Name\r\n"
    "                       address           Offset      Line     Offset   Name\r?\n"};
static const std::regex re_memory_map_romram_srecord_binfile_prologue_1{
//  "                       Starting Size     File     ROM      RAM Buffer    S-Record Bin File Bin File\r\n"
    "                       Starting Size     File     ROM      RAM Buffer    S-Record Bin File Bin File\r?\n"};
static const std::regex re_memory_map_romram_srecord_binfile_prologue_2{
//  "                       address           Offset   Address  Address       Line     Offset   Name\r\n"
    "                       address           Offset   Address  Address       Line     Offset   Name\r?\n"};
// clang-format on

Map::Error Map::ScanPrologue_MemoryMap(const char*& head, const char* const tail,
                                       std::size_t& line_number)
{
  std::cmatch match;

  if (std::regex_search(head, tail, match, re_memory_map_simple_prologue_1_old,
                        std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    if (std::regex_search(head, tail, match, re_memory_map_simple_prologue_2_old,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      auto portion = std::make_unique<MemoryMap>(false);
      const auto error = portion->ScanSimple_old(head, tail, line_number);
      if (error != Error::None)
        return error;
      this->memory_map = std::move(portion);
    }
    else
    {
      return Error::MemoryMapBadPrologue;
    }
  }
  else if (std::regex_search(head, tail, match, re_memory_map_romram_prologue_1_old,
                             std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    if (std::regex_search(head, tail, match, re_memory_map_romram_prologue_2_old,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      auto portion = std::make_unique<MemoryMap>(true);
      const auto error = portion->ScanRomRam_old(head, tail, line_number);
      if (error != Error::None)
        return error;
      this->memory_map = std::move(portion);
    }
    else
    {
      return Error::MemoryMapBadPrologue;
    }
  }
  else if (std::regex_search(head, tail, match, re_memory_map_simple_prologue_1,
                             std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    if (std::regex_search(head, tail, match, re_memory_map_simple_prologue_2,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      auto portion = std::make_unique<MemoryMap>(false, false, false);
      const auto error = portion->ScanSimple(head, tail, line_number);
      if (error != Error::None)
        return error;
      this->memory_map = std::move(portion);
    }
    else
    {
      return Error::MemoryMapBadPrologue;
    }
  }
  else if (std::regex_search(head, tail, match, re_memory_map_romram_prologue_1,
                             std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    if (std::regex_search(head, tail, match, re_memory_map_romram_prologue_2,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      auto portion = std::make_unique<MemoryMap>(true, false, false);
      const auto error = portion->ScanRomRam(head, tail, line_number);
      if (error != Error::None)
        return error;
      this->memory_map = std::move(portion);
    }
    else
    {
      return Error::MemoryMapBadPrologue;
    }
  }
  else if (std::regex_search(head, tail, match, re_memory_map_srecord_prologue_1,
                             std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    if (std::regex_search(head, tail, match, re_memory_map_srecord_prologue_2,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      auto portion = std::make_unique<MemoryMap>(false, true, false);
      const auto error = portion->ScanSRecord(head, tail, line_number);
      if (error != Error::None)
        return error;
      this->memory_map = std::move(portion);
    }
    else
    {
      return Error::MemoryMapBadPrologue;
    }
  }
  else if (std::regex_search(head, tail, match, re_memory_map_binfile_prologue_1,
                             std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    if (std::regex_search(head, tail, match, re_memory_map_binfile_prologue_2,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      auto portion = std::make_unique<MemoryMap>(false, false, true);
      const auto error = portion->ScanBinFile(head, tail, line_number);
      if (error != Error::None)
        return error;
      this->memory_map = std::move(portion);
    }
    else
    {
      return Error::MemoryMapBadPrologue;
    }
  }
  else if (std::regex_search(head, tail, match, re_memory_map_romram_srecord_prologue_1,
                             std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    if (std::regex_search(head, tail, match, re_memory_map_romram_srecord_prologue_2,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      auto portion = std::make_unique<MemoryMap>(true, true, false);
      const auto error = portion->ScanRomRamSRecord(head, tail, line_number);
      if (error != Error::None)
        return error;
      this->memory_map = std::move(portion);
    }
    else
    {
      return Error::MemoryMapBadPrologue;
    }
  }
  else if (std::regex_search(head, tail, match, re_memory_map_romram_binfile_prologue_1,
                             std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    if (std::regex_search(head, tail, match, re_memory_map_romram_binfile_prologue_2,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      auto portion = std::make_unique<MemoryMap>(true, false, true);
      const auto error = portion->ScanRomRamBinFile(head, tail, line_number);
      if (error != Error::None)
        return error;
      this->memory_map = std::move(portion);
    }
    else
    {
      return Error::MemoryMapBadPrologue;
    }
  }
  else if (std::regex_search(head, tail, match, re_memory_map_srecord_binfile_prologue_1,
                             std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    if (std::regex_search(head, tail, match, re_memory_map_srecord_binfile_prologue_2,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      auto portion = std::make_unique<MemoryMap>(false, true, true);
      const auto error = portion->ScanSRecordBinFile(head, tail, line_number);
      if (error != Error::None)
        return error;
      this->memory_map = std::move(portion);
    }
    else
    {
      return Error::MemoryMapBadPrologue;
    }
  }
  else if (std::regex_search(head, tail, match, re_memory_map_romram_srecord_binfile_prologue_1,
                             std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    if (std::regex_search(head, tail, match, re_memory_map_romram_srecord_binfile_prologue_2,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      auto portion = std::make_unique<MemoryMap>(true, true, true);
      const auto error = portion->ScanRomRamSRecordBinFile(head, tail, line_number);
      if (error != Error::None)
        return error;
      this->memory_map = std::move(portion);
    }
    else
    {
      return Error::MemoryMapBadPrologue;
    }
  }
  else
  {
    return Error::MemoryMapBadPrologue;
  }
  return Error::None;
}

Map::Error Map::ScanForGarbage(const char* const head, const char* const tail)
{
  if (head < tail)
  {
    // Gamecube ISO Tool (http://www.wiibackupmanager.co.uk/gcit.html) has a bug that appends null
    // byte padding to the next multiple of 32 bytes at the end of any file it extracts. During my
    // research, I ran into a lot of linker maps afflicted by this bug, enough to justify a special
    // case for garbage consisting of only null bytes.
    if (std::any_of(head, tail, [](const char c) { return c != '\0'; }))
      return Error::GarbageFound;
  }
  return Error::None;
}

// clang-format off
static const std::regex re_symbol_closure_node_normal{
//  "%i] " and "%s (%s,%s) found in %s %s\r\n"
    "   *(\\d+)\\] (.*) \\((.*),(.*)\\) found in (.*) (.*)\r?\n"};
static const std::regex re_symbol_closure_node_normal_unref_dup_header{
//  "%i] " and ">>> UNREFERENCED DUPLICATE %s\r\n"
    "   *(\\d+)\\] >>> UNREFERENCED DUPLICATE (.*)\r?\n"};
static const std::regex re_symbol_closure_node_normal_unref_dups{
//  "%i] " and ">>> (%s,%s) found in %s %s\r\n"
    "   *(\\d+)\\] >>> \\((.*),(.*)\\) found in (.*) (.*)\r?\n"};
static const std::regex re_symbol_closure_node_linker_generated{
//  "%i] " and "%s found as linker generated symbol\r\n"
    "   *(\\d+)\\] (.*) found as linker generated symbol\r?\n"};
// clang-format on

static const std::unordered_map<std::string, const Map::SymbolClosure::Type>
    map_symbol_closure_st_type{
        {"notype", Map::SymbolClosure::Type::notype},
        {"object", Map::SymbolClosure::Type::object},
        {"func", Map::SymbolClosure::Type::func},
        {"section", Map::SymbolClosure::Type::section},
        {"file", Map::SymbolClosure::Type::file},
        {"unknown", Map::SymbolClosure::Type::unknown},
    };
static const std::unordered_map<std::string, const Map::SymbolClosure::Bind>
    map_symbol_closure_st_bind{
        {"local", Map::SymbolClosure::Bind::local},
        {"global", Map::SymbolClosure::Bind::global},
        {"weak", Map::SymbolClosure::Bind::weak},
        {"multidef", Map::SymbolClosure::Bind::multidef},
        {"overload", Map::SymbolClosure::Bind::overload},
        {"unknown", Map::SymbolClosure::Bind::unknown},
    };

Map::Error Map::SymbolClosure::Scan(const char*& head, const char* const tail,
                                    std::size_t& line_number,
                                    std::list<std::string>& unresolved_symbols)
{
  std::cmatch match;

  NodeBase* curr_node = &this->root;
  int curr_hierarchy_level = 0;

  while (true)
  {
    if (std::regex_search(head, tail, match, re_symbol_closure_node_normal,
                          std::regex_constants::match_continuous))
    {
      const int next_hierarchy_level = std::stoi(match.str(1));
      if (next_hierarchy_level <= 0)
        return Error::SymbolClosureInvalidHierarchy;
      if (curr_hierarchy_level + 1 < next_hierarchy_level)
        return Error::SymbolClosureHierarchySkip;
      const std::string type = match.str(3), bind = match.str(4);
      if (!map_symbol_closure_st_type.contains(type))
        return Error::SymbolClosureInvalidSymbolType;
      if (!map_symbol_closure_st_bind.contains(bind))
        return Error::SymbolClosureInvalidSymbolBind;
      line_number += 1;
      head += match.length();

      auto next_node = std::make_unique<NodeNormal>(
          match.str(2), map_symbol_closure_st_type.at(type), map_symbol_closure_st_bind.at(bind),
          match.str(5), match.str(6));

      for (int i = curr_hierarchy_level + 1; i > next_hierarchy_level; --i)
        curr_node = curr_node->parent;
      curr_hierarchy_level = next_hierarchy_level;

      if (std::regex_search(head, tail, match, re_symbol_closure_node_normal_unref_dup_header,
                            std::regex_constants::match_continuous))
      {
        if (std::stoi(match.str(1)) != curr_hierarchy_level)
          return Error::SymbolClosureUnrefDupsHierarchyMismatch;
        if (match.str(2) != next_node->name)
          return Error::SymbolClosureUnrefDupsNameMismatch;
        line_number += 1;
        head += match.length();
        while (std::regex_search(head, tail, match, re_symbol_closure_node_normal_unref_dups,
                                 std::regex_constants::match_continuous))
        {
          if (std::stoi(match.str(1)) != curr_hierarchy_level)
            return Error::SymbolClosureUnrefDupsHierarchyMismatch;
          const std::string unref_dup_type = match.str(2), unref_dup_bind = match.str(3);
          if (!map_symbol_closure_st_type.contains(unref_dup_type))
            return Error::SymbolClosureInvalidSymbolType;
          if (!map_symbol_closure_st_bind.contains(unref_dup_bind))
            return Error::SymbolClosureInvalidSymbolType;
          line_number += 1;
          head += match.length();
          next_node->unref_dups.emplace_back(map_symbol_closure_st_type.at(unref_dup_type),
                                             map_symbol_closure_st_bind.at(unref_dup_bind),
                                             match.str(4), match.str(5));
        }
        if (next_node->unref_dups.empty())
          return Error::SymbolClosureUnrefDupsEmpty;
        this->SetMinVersion(Version::version_2_3_3_build_137);
      }

      next_node->parent = curr_node;
      // Though I do not understand it, the following is a normal occurrence for _dtors$99:
      // "  1] _dtors$99 (object,global) found in Linker Generated Symbol File "
      // "    3] .text (section,local) found in xyz.cpp lib.a"
      if (const bool is_weird = (next_node->name == "_dtors$99" &&  // Redundancy out of paranoia
                                 next_node->module == "Linker Generated Symbol File");
          void(curr_node = curr_node->children.emplace_back(next_node.release()).get()), is_weird)
      {
        auto dummy_node = std::make_unique<NodeBase>();
        dummy_node->parent = curr_node;
        curr_node = curr_node->children.emplace_back(dummy_node.release()).get();
        ++curr_hierarchy_level;
        this->SetMinVersion(Version::version_3_0_4);
      }
      continue;
    }
    if (std::regex_search(head, tail, match, re_symbol_closure_node_linker_generated,
                          std::regex_constants::match_continuous))
    {
      const int next_hierarchy_level = std::stoi(match.str(1));
      if (next_hierarchy_level <= 0)
        return Error::SymbolClosureInvalidHierarchy;
      if (curr_hierarchy_level + 1 < next_hierarchy_level)
        return Error::SymbolClosureHierarchySkip;
      line_number += 1;
      head += match.length();

      auto next_node = std::make_unique<NodeLinkerGenerated>(match.str(2));

      for (int i = curr_hierarchy_level + 1; i > next_hierarchy_level; --i)
        curr_node = curr_node->parent;
      curr_hierarchy_level = next_hierarchy_level;

      next_node->parent = curr_node;
      curr_node = curr_node->children.emplace_back(next_node.release()).get();
      continue;
    }
    // Up until CodeWarrior for GCN 3.0a3 (at the earliest), unresolved symbols were printed as the
    // symbol closure was being walked and printed itself. This gives a good idea of what function
    // was looking for that symbol, but because no hierarchy level is given, it is impossible to be
    // certain without analyzing code. After that version, all unresolved symbols from the symbol
    // closure(s) and EPPC_PatternMatching would (I think) be printed after the DWARF symbol
    // closure. The way it works out, this same scanning code handles that as well. If symbol
    // closures are disabled, this scan function will still parse the unresolved symbol prints.
    // There are also a few linker maps I've found where it appears the unresolved symbols are
    // pre-printed before the first symbol closure. Wouldn't you know it, this scanning code also
    // handles that.
    if (std::regex_search(head, tail, match, re_unresolved_symbol,
                          std::regex_constants::match_continuous))
    {
      do
      {
        line_number += 1;
        head += match.length();
        unresolved_symbols.push_back(match.str(1));
      } while (std::regex_search(head, tail, match, re_unresolved_symbol,
                                 std::regex_constants::match_continuous));
      continue;
    }
    break;
  }
  return Error::None;
}

void Map::SymbolClosure::PrintPrefix(std::ostream& stream, const int hierarchy_level)
{
  if (hierarchy_level >= 0)
    for (int i = 0; i <= hierarchy_level; ++i)
      stream.put(' ');
  Common::Print(stream, "{:d}] ", hierarchy_level);  // "%i] "
}
const char* Map::SymbolClosure::GetName(const Map::SymbolClosure::Type st_type) noexcept
{
  switch (st_type)
  {
  case Type::notype:
    return "notype";
  case Type::object:
    return "object";
  case Type::func:
    return "func";
  case Type::section:
    return "section";
  case Type::file:
    return "file";
  default:
    return "unknown";
  }
}
const char* Map::SymbolClosure::GetName(const Map::SymbolClosure::Bind st_bind) noexcept
{
  switch (st_bind)
  {
  case Bind::local:
    return "local";
  case Bind::global:
    return "global";
  case Bind::weak:
    return "weak";
  default:
    return "unknown";
  case Bind::multidef:
    return "multidef";
  case Bind::overload:
    return "overload";
  }
}

void Map::SymbolClosure::Print(std::ostream& stream) const
{
  this->root.Print(stream, 0);
}

void Map::SymbolClosure::NodeBase::Print(std::ostream& stream, const int hierarchy_level) const
{
  for (const auto& node : this->children)
    node->Print(stream, hierarchy_level + 1);
}

void Map::SymbolClosure::NodeNormal::Print(std::ostream& stream, const int hierarchy_level) const
{
  PrintPrefix(stream, hierarchy_level);
  // libc++ std::format requires lvalue references for the args.
  const char *type_s = GetName(type), *bind_s = GetName(bind);
  // "%s (%s,%s) found in %s %s\r\n"
  Common::Print(stream, "{:s} ({:s},{:s}) found in {:s} {:s}\r\n", name, type_s, bind_s, module,
                file);
  if (!this->unref_dups.empty())
  {
    PrintPrefix(stream, hierarchy_level);
    // ">>> UNREFERENCED DUPLICATE %s\r\n"
    Common::Print(stream, ">>> UNREFERENCED DUPLICATE {:s}\r\n", name);
    for (const auto& unref_dup : unref_dups)
      unref_dup.Print(stream, hierarchy_level);
  }
  for (const auto& node : children)
    node->Print(stream, hierarchy_level + 1);
}

void Map::SymbolClosure::NodeNormal::UnreferencedDuplicate::Print(std::ostream& stream,
                                                                  const int hierarchy_level) const
{
  PrintPrefix(stream, hierarchy_level);
  const char *type_s = GetName(type), *bind_s = GetName(bind);
  // ">>> (%s,%s) found in %s %s\r\n"
  Common::Print(stream, ">>> ({:s},{:s}) found in {:s} {:s}\r\n", type_s, bind_s, module, file);
}

void Map::SymbolClosure::NodeLinkerGenerated::Print(std::ostream& stream,
                                                    const int hierarchy_level) const
{
  PrintPrefix(stream, hierarchy_level);
  // "%s found as linker generated symbol\r\n"
  Common::Print(stream, "{:s} found as linker generated symbol\r\n", name);
  for (const auto& node : children)            // Linker generated symbols should have
    node->Print(stream, hierarchy_level + 1);  // no children but we'll check anyway.
}

// clang-format off
static const std::regex re_code_merging_is_duplicated{
//  "--> duplicated code: symbol %s is duplicated by %s, size = %d \r\n\r\n"
    "--> duplicated code: symbol (.*) is duplicated by (.*), size = (\\d+) \r?\n\r?\n"};
static const std::regex re_code_merging_will_be_replaced{
//  "--> the function %s will be replaced by a branch to %s\r\n\r\n\r\n"
    "--> the function (.*) will be replaced by a branch to (.*)\r?\n\r?\n\r?\n"};
static const std::regex re_code_merging_was_interchanged{
//  "--> the function %s was interchanged with %s, size=%d \r\n"
    "--> the function (.*) was interchanged with (.*), size=(\\d+) \r?\n"};
static const std::regex re_code_folding_header{
//  "\r\n\r\n\r\nCode folded in file: %s \r\n"
    "\r?\n\r?\n\r?\nCode folded in file: (.*) \r?\n"};
static const std::regex re_code_folding_is_duplicated{
//  "--> %s is duplicated by %s, size = %d \r\n\r\n"
    "--> (.*) is duplicated by (.*), size = (\\d+) \r?\n\r?\n"};
static const std::regex re_code_folding_is_duplicated_new_branch{
//  "--> %s is duplicated by %s, size = %d, new branch function %s \r\n\r\n"
    "--> (.*) is duplicated by (.*), size = (\\d+), new branch function (.*) \r?\n\r?\n"};
// clang-format on

Map::Error Map::EPPC_PatternMatching::Scan(const char*& head, const char* const tail,
                                           std::size_t& line_number)
{
  std::cmatch match;

  std::string last;

  while (true)
  {
    bool will_be_replaced = false;
    bool was_interchanged = false;
    // EPPC_PatternMatching looks for functions that are duplicates of one another and prints what
    // it has changed in real-time to the linker map.
    if (std::regex_search(head, tail, match, re_code_merging_is_duplicated,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      std::string first_name = match.str(1);
      std::string second_name = match.str(2);
      const std::uint32_t size = std::stoul(match.str(3));
      if (std::regex_search(head, tail, match, re_code_merging_will_be_replaced,
                            std::regex_constants::match_continuous))
      {
        if (match.str(1) != first_name)
          return Error::EPPC_PatternMatchingMergingFirstNameMismatch;
        if (match.str(2) != second_name)
          return Error::EPPC_PatternMatchingMergingSecondNameMismatch;
        line_number += 1;
        head += match.length();
        will_be_replaced = true;
      }
      this->merging_units.emplace_back(std::move(first_name), std::move(second_name), size,
                                       will_be_replaced, was_interchanged);
      continue;
    }
    if (std::regex_search(head, tail, match, re_code_merging_was_interchanged,
                          std::regex_constants::match_continuous))
    {
      std::string first_name = match.str(1);
      std::string second_name = match.str(2);
      const std::uint32_t size = std::stoul(match.str(3));
      was_interchanged = true;

      line_number += 1;
      head += match.length();
      if (std::regex_search(head, tail, match, re_code_merging_will_be_replaced,
                            std::regex_constants::match_continuous))
      {
        if (match.str(1) != first_name)
          return Error::EPPC_PatternMatchingMergingFirstNameMismatch;
        if (match.str(2) != second_name)
          return Error::EPPC_PatternMatchingMergingSecondNameMismatch;
        line_number += 1;
        head += match.length();
        will_be_replaced = true;
      }
      if (std::regex_search(head, tail, match, re_code_merging_is_duplicated,
                            std::regex_constants::match_continuous))
      {
        if (match.str(1) != first_name)
          return Error::EPPC_PatternMatchingMergingFirstNameMismatch;
        if (match.str(2) != second_name)
          return Error::EPPC_PatternMatchingMergingSecondNameMismatch;
        if (std::stoul(match.str(3)) != size)
          return Error::EPPC_PatternMatchingMergingSizeMismatch;
        line_number += 1;
        head += match.length();
      }
      else
      {
        return Error::EPPC_PatternMatchingMergingInterchangeMissingEpilogue;
      }
      this->merging_units.emplace_back(std::move(first_name), std::move(second_name), size,
                                       will_be_replaced, was_interchanged);
      continue;
    }
    break;
  }
  // After analysis concludes, a redundant summary of changes per file is printed.
  while (std::regex_search(head, tail, match, re_code_folding_header,
                           std::regex_constants::match_continuous))
  {
    auto folding_unit = FoldingUnit(match.str(1));
    line_number += 1;
    head += match.length();
    while (true)
    {
      if (std::regex_search(head, tail, match, re_code_folding_is_duplicated,
                            std::regex_constants::match_continuous))
      {
        line_number += 1;
        head += match.length();
        folding_unit.units.emplace_back(match.str(1), match.str(2), std::stoul(match.str(3)),
                                        false);
        continue;
      }
      if (std::regex_search(head, tail, match, re_code_folding_is_duplicated_new_branch,
                            std::regex_constants::match_continuous))
      {
        if (match.str(1) != match.str(4))  // It is my assumption that they will always match.
          return Error::EPPC_PatternMatchingFoldingNewBranchFunctionNameMismatch;
        line_number += 1;
        head += match.length();
        folding_unit.units.emplace_back(match.str(1), match.str(2), std::stoul(match.str(3)), true);
        continue;
      }
      break;
    }
    this->folding_units.push_back(std::move(folding_unit));
  }
  return Error::None;
}

void Map::EPPC_PatternMatching::Print(std::ostream& stream) const
{
  for (const auto& unit : merging_units)
    unit.Print(stream);
  for (const auto& unit : folding_units)
    unit.Print(stream);
}

void Map::EPPC_PatternMatching::MergingUnit::Print(std::ostream& stream) const
{
  if (was_interchanged)
  {
    // "--> the function %s was interchanged with %s, size=%d \r\n"
    Common::Print(stream, "--> the function {:s} was interchanged with {:s}, size={:d} \r\n",
                  first_name, second_name, size);
    if (will_be_replaced)
    {
      // "--> the function %s will be replaced by a branch to %s\r\n\r\n\r\n"
      Common::Print(stream,
                    "--> the function {:s} will be replaced by a branch to {:s}\r\n\r\n\r\n",
                    first_name, second_name);
    }
    // "--> duplicated code: symbol %s is duplicated by %s, size = %d \r\n\r\n"
    Common::Print(stream,
                  "--> duplicated code: symbol {:s} is duplicated by {:s}, size = {:d} \r\n\r\n",
                  first_name, second_name, size);
  }
  else
  {
    // "--> duplicated code: symbol %s is duplicated by %s, size = %d \r\n\r\n"
    Common::Print(stream,
                  "--> duplicated code: symbol {:s} is duplicated by {:s}, size = {:d} \r\n\r\n",
                  first_name, second_name, size);
    if (will_be_replaced)
    {
      // "--> the function %s will be replaced by a branch to %s\r\n\r\n\r\n"
      Common::Print(stream,
                    "--> the function {:s} will be replaced by a branch to {:s}\r\n\r\n\r\n",
                    first_name, second_name);
    }
  }
}

void Map::EPPC_PatternMatching::FoldingUnit::Print(std::ostream& stream) const
{
  // "\r\n\r\n\r\nCode folded in file: %s \r\n"
  Common::Print(stream, "\r\n\r\n\r\nCode folded in file: {:s} \r\n", name);
  for (const auto& unit : units)
    unit.Print(stream);
}

void Map::EPPC_PatternMatching::FoldingUnit::Unit::Print(std::ostream& stream) const
{
  if (new_branch_function)
    // "--> %s is duplicated by %s, size = %d, new branch function %s \r\n\r\n"
    Common::Print(stream,
                  "--> {:s} is duplicated by {:s}, size = {:d}, new branch function {:s} \r\n\r\n",
                  first_name, second_name, size, first_name);
  else
    // "--> %s is duplicated by %s, size = %d \r\n\r\n"
    Common::Print(stream, "--> {:s} is duplicated by {:s}, size = {:d} \r\n\r\n", first_name,
                  second_name, size);
}

// clang-format off
static const std::regex re_linker_opts_unit_not_near{
//  "  %s/ %s()/ %s - address not in near addressing range \r\n"
    "  (.*)/ (.*)\\(\\)/ (.*) - address not in near addressing range \r?\n"};
static const std::regex re_linker_opts_unit_address_not_computed{
//  "  %s/ %s()/ %s - final address not yet computed \r\n"
    "  (.*)/ (.*)\\(\\)/ (.*) - final address not yet computed \r?\n"};
static const std::regex re_linker_opts_unit_optimized{
//  "! %s/ %s()/ %s - optimized addressing \r\n"
    "! (.*)/ (.*)\\(\\)/ (.*) - optimized addressing \r?\n"};
static const std::regex re_linker_opts_unit_disassemble_error{
//  "  %s/ %s() - error disassembling function \r\n"
    "  (.*)/ (.*)\\(\\) - error disassembling function \r?\n"};
// clang-format on

Map::Error Map::LinkerOpts::Scan(const char*& head, const char* const tail,
                                 std::size_t& line_number)

{
  std::cmatch match;

  while (true)
  {
    if (std::regex_search(head, tail, match, re_linker_opts_unit_not_near,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      this->units.emplace_back(Unit::Kind::NotNear, match.str(1), match.str(2), match.str(3));
      continue;
    }
    if (std::regex_search(head, tail, match, re_linker_opts_unit_disassemble_error,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      this->units.emplace_back(match.str(1), match.str(2));
      continue;
    }
    if (std::regex_search(head, tail, match, re_linker_opts_unit_address_not_computed,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      this->units.emplace_back(Unit::Kind::NotComputed, match.str(1), match.str(2), match.str(3));
      continue;
    }
    // I have not seen a single linker map with this
    if (std::regex_search(head, tail, match, re_linker_opts_unit_optimized,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      this->units.emplace_back(Unit::Kind::Optimized, match.str(1), match.str(2), match.str(3));
      continue;
    }
    break;
  }
  return Error::None;
}

void Map::LinkerOpts::Print(std::ostream& stream) const
{
  for (const auto& unit : units)
    unit.Print(stream);
}

void Map::LinkerOpts::Unit::Print(std::ostream& stream) const
{
  switch (this->unit_kind)
  {
  case Kind::NotNear:
    // "  %s/ %s()/ %s - address not in near addressing range \r\n"
    Common::Print(stream, "  {:s}/ {:s}()/ {:s} - address not in near addressing range \r\n",
                  module, name, reference_name);
    return;
  case Kind::NotComputed:
    // "  %s/ %s()/ %s - final address not yet computed \r\n"
    Common::Print(stream, "  {:s}/ {:s}()/ {:s} - final address not yet computed \r\n", module,
                  name, reference_name);
    return;
  case Kind::Optimized:
    // "! %s/ %s()/ %s - optimized addressing \r\n"
    Common::Print(stream, "! {:s}/ {:s}()/ {:s} - optimized addressing \r\n", module, name,
                  reference_name);
    return;
  case Kind::DisassembleError:
    // "  %s/ %s() - error disassembling function \r\n"
    Common::Print(stream, "  {:s}/ {:s}() - error disassembling function \r\n", module, name);
    return;
  }
}

// clang-format off
static const std::regex re_mixed_mode_islands_created{
//  "  mixed mode island %s created for %s\r\n"
    "  mixed mode island (.*) created for (.*)\r?\n"};
static const std::regex re_mixed_mode_islands_created_safe{
//  "  safe mixed mode island %s created for %s\r\n"
    "  safe mixed mode island (.*) created for (.*)\r?\n"};
// clang-format on

Map::Error Map::MixedModeIslands::Scan(const char*& head, const char* const tail,
                                       std::size_t& line_number)
{
  std::cmatch match;

  // TODO: I have literally never seen Mixed Mode Islands.
  // Similar to Branch Islands, this is conjecture.
  while (true)
  {
    if (std::regex_search(head, tail, match, re_mixed_mode_islands_created,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      this->units.emplace_back(match.str(1), match.str(2), false);
      continue;
    }
    if (std::regex_search(head, tail, match, re_mixed_mode_islands_created_safe,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      this->units.emplace_back(match.str(1), match.str(2), true);
      continue;
    }
    break;
  }
  return Error::None;
}

void Map::MixedModeIslands::Print(std::ostream& stream) const
{
  Common::Print(stream, "\r\nMixed Mode Islands\r\n");
  for (const auto& unit : units)
    unit.Print(stream);
}
void Map::MixedModeIslands::Unit::Print(std::ostream& stream) const
{
  if (is_safe)
    // "  safe mixed mode island %s created for %s\r\n"
    Common::Print(stream, "  safe mixed mode island {:s} created for {:s}\r\n", first_name,
                  second_name);
  else
    // "  mixed mode island %s created for %s\r\n"
    Common::Print(stream, "  mixed mode island {:s} created for {:s}\r\n", first_name, second_name);
}

// clang-format off
static const std::regex re_branch_islands_created{
//  "  branch island %s created for %s\r\n"
    "  branch island (.*) created for (.*)\r?\n"};
static const std::regex re_branch_islands_created_safe{
//  "  safe branch island %s created for %s\r\n"
    "  safe branch island (.*) created for (.*)\r?\n"};
// clang-format on

Map::Error Map::BranchIslands::Scan(const char*& head, const char* const tail,
                                    std::size_t& line_number)
{
  std::cmatch match;

  // TODO: I have only ever seen Branch Islands from Skylanders Swap Force, and on top of that, it
  // was an empty portion. From datamining MWLDEPPC, I can only assume it goes something like this.
  while (true)
  {
    if (std::regex_search(head, tail, match, re_branch_islands_created,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      this->units.emplace_back(match.str(1), match.str(2), false);
      continue;
    }
    if (std::regex_search(head, tail, match, re_branch_islands_created_safe,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      this->units.emplace_back(match.str(1), match.str(2), true);
      continue;
    }
    break;
  }
  return Error::None;
}

void Map::BranchIslands::Print(std::ostream& stream) const
{
  Common::Print(stream, "\r\nBranch Islands\r\n");
  for (const auto& unit : units)
    unit.Print(stream);
}
void Map::BranchIslands::Unit::Print(std::ostream& stream) const
{
  if (is_safe)
    //  "  safe branch island %s created for %s\r\n"
    Common::Print(stream, "  safe branch island {:s} created for {:s}\r\n", first_name,
                  second_name);
  else
    //  "  branch island %s created for %s\r\n"
    Common::Print(stream, "  branch island {:s} created for {:s}\r\n", first_name, second_name);
}

Map::Error Map::LinktimeSizeDecreasingOptimizations::Scan(const char*&, const char* const,
                                                          std::size_t&)
{
  // TODO?  I am not convinced this portion is capable of containing anything.
  return Error::None;
}

void Map::LinktimeSizeDecreasingOptimizations::Print(std::ostream& stream) const
{
  Common::Print(stream, "\r\nLinktime size-decreasing optimizations\r\n");
}

Map::Error Map::LinktimeSizeIncreasingOptimizations::Scan(const char*&, const char* const,
                                                          std::size_t&)
{
  // TODO?  I am not convinced this portion is capable of containing anything.
  return Error::None;
}

void Map::LinktimeSizeIncreasingOptimizations::Print(std::ostream& stream) const
{
  Common::Print(stream, "\r\nLinktime size-increasing optimizations\r\n");
}

// clang-format off
static const std::regex re_section_layout_3column_unit_normal{
//  "  %08x %06x %08x %2i %s \t%s %s\r\n"
    "  ([0-9a-f]{8}) ([0-9a-f]{6}) ([0-9a-f]{8})  ?(\\d+) (.*) \t(.*) (.*)\r?\n"};
static const std::regex re_section_layout_3column_unit_unused{
//  "  UNUSED   %06x ........ %s %s %s\r\n"
    "  UNUSED   ([0-9a-f]{6}) \\.{8} (.*) (.*) (.*)\r?\n"};
static const std::regex re_section_layout_3column_unit_entry{
//  "  %08lx %06lx %08lx %s (entry of %s) \t%s %s\r\n"
    "  ([0-9a-f]{8}) ([0-9a-f]{6}) ([0-9a-f]{8}) (.*) \\(entry of (.*)\\) \t(.*) (.*)\r?\n"};
// clang-format on

Map::Error Map::SectionLayout::Scan3Column(const char*& head, const char* const tail,
                                           std::size_t& line_number)
{
  std::cmatch match;

  while (true)
  {
    if (std::regex_search(head, tail, match, re_section_layout_3column_unit_normal,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      this->units.emplace_back(xstoul(match.str(1)), xstoul(match.str(2)), xstoul(match.str(3)),
                               std::stoi(match.str(4)), match.str(5), match.str(6), match.str(7));
      continue;
    }
    if (std::regex_search(head, tail, match, re_section_layout_3column_unit_unused,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      this->units.emplace_back(xstoul(match.str(1)), match.str(2), match.str(3), match.str(4));
      continue;
    }
    if (std::regex_search(head, tail, match, re_section_layout_3column_unit_entry,
                          std::regex_constants::match_continuous))
    {
      std::string entry_parent_name = match.str(5), module = match.str(6), file = match.str(7);
      for (auto& parent_unit : std::ranges::reverse_view{this->units})
      {
        if (file != parent_unit.file || module != parent_unit.module)
          return Error::SectionLayoutOrphanedEntry;
        if (entry_parent_name != parent_unit.name)
          continue;
        line_number += 1;
        head += match.length();
        parent_unit.entry_children.push_back(&this->units.emplace_back(
            xstoul(match.str(1)), xstoul(match.str(2)), xstoul(match.str(3)), match.str(4),
            &parent_unit, std::move(module), std::move(file)));
        goto ENTRY_PARENT_FOUND;  // I wish C++ had for-else clauses.
      }
      return Error::SectionLayoutOrphanedEntry;
    ENTRY_PARENT_FOUND:
      continue;
    }
    break;
  }
  return Error::None;
}

// clang-format off
static const std::regex re_section_layout_4column_unit_normal{
//  "  %08x %06x %08x %08x %2i %s \t%s %s\r\n"
    "  ([0-9a-f]{8}) ([0-9a-f]{6}) ([0-9a-f]{8}) ([0-9a-f]{8})  ?(\\d+) (.*) \t(.*) (.*)\r?\n"};
static const std::regex re_section_layout_4column_unit_unused{
//  "  UNUSED   %06x ........ ........    %s %s %s\r\n"
    "  UNUSED   ([0-9a-f]{6}) \\.{8} \\.{8}    (.*) (.*) (.*)\r?\n"};
static const std::regex re_section_layout_4column_unit_entry{
//  "  %08lx %06lx %08lx %08lx    %s (entry of %s) \t%s %s\r\n"
    "  ([0-9a-f]{8}) ([0-9a-f]{6}) ([0-9a-f]{8}) ([0-9a-f]{8})    (.*) \\(entry of (.*)\\) \t(.*) (.*)\r?\n"};
static const std::regex re_section_layout_4column_unit_special{
//  "  %08x %06x %08x %08x %2i %s\r\n"
    "  ([0-9a-f]{8}) ([0-9a-f]{6}) ([0-9a-f]{8}) ([0-9a-f]{8})  ?(\\d+) (.*)\r?\n"};
// clang-format on

Map::Error Map::SectionLayout::Scan4Column(const char*& head, const char* const tail,
                                           std::size_t& line_number)
{
  std::cmatch match;

  while (true)
  {
    if (std::regex_search(head, tail, match, re_section_layout_4column_unit_normal,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      this->units.emplace_back(xstoul(match.str(1)), xstoul(match.str(2)), xstoul(match.str(3)),
                               xstoul(match.str(4)), std::stoi(match.str(5)), match.str(6),
                               match.str(7), match.str(8));
      continue;
    }
    if (std::regex_search(head, tail, match, re_section_layout_4column_unit_unused,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      this->units.emplace_back(xstoul(match.str(1)), match.str(2), match.str(3), match.str(4));
      continue;
    }
    if (std::regex_search(head, tail, match, re_section_layout_4column_unit_entry,
                          std::regex_constants::match_continuous))
    {
      std::string entry_parent_name = match.str(6), module = match.str(7), file = match.str(8);
      for (auto& parent_unit : std::ranges::reverse_view{this->units})
      {
        if (file != parent_unit.file || module != parent_unit.module)
          return Error::SectionLayoutOrphanedEntry;
        if (entry_parent_name != parent_unit.name)
          continue;
        line_number += 1;
        head += match.length();
        parent_unit.entry_children.push_back(&this->units.emplace_back(
            xstoul(match.str(1)), xstoul(match.str(2)), xstoul(match.str(3)), xstoul(match.str(4)),
            match.str(5), &parent_unit, std::move(module), std::move(file)));
        goto ENTRY_PARENT_FOUND;  // I wish C++ had for-else clauses.
      }
      return Error::SectionLayoutOrphanedEntry;
    ENTRY_PARENT_FOUND:
      continue;
    }
    if (std::regex_search(head, tail, match, re_section_layout_4column_unit_special,
                          std::regex_constants::match_continuous))
    {
      std::string special_name = match.str(6);
      if (special_name != "*fill*" && special_name != "**fill**")
        return Error::SectionLayoutSpecialNotFill;
      line_number += 1;
      head += match.length();
      this->units.emplace_back(xstoul(match.str(1)), xstoul(match.str(2)), xstoul(match.str(3)),
                               xstoul(match.str(4)), std::stoi(match.str(5)),
                               std::move(special_name));
      continue;
    }
    break;
  }
  return Error::None;
}

// clang-format off
static const std::regex re_section_layout_tloztp_unit_entry{
    "  ([0-9a-f]{8}) ([0-9a-f]{6}) ([0-9a-f]{8})    (.*) \\(entry of (.*)\\) \t(.*) (.*)\r?\n"};
static const std::regex re_section_layout_tloztp_unit_special{
    "  ([0-9a-f]{8}) ([0-9a-f]{6}) ([0-9a-f]{8})  ?(\\d+) (.*)\r?\n"};
// clang-format on

Map::Error Map::SectionLayout::ScanTLOZTP(const char*& head, const char* const tail,
                                          std::size_t& line_number)
{
  std::cmatch match;

  while (true)
  {
    if (std::regex_search(head, tail, match, re_section_layout_3column_unit_normal,
                          std::regex_constants::match_continuous))
    {
      line_number += 1;
      head += match.length();
      this->units.emplace_back(xstoul(match.str(1)), xstoul(match.str(2)), xstoul(match.str(3)), 0,
                               std::stoi(match.str(4)), match.str(5), match.str(6), match.str(7));
      continue;
    }
    if (std::regex_search(head, tail, match, re_section_layout_tloztp_unit_entry,
                          std::regex_constants::match_continuous))
    {
      std::string entry_parent_name = match.str(5), module = match.str(6), file = match.str(7);
      for (auto& parent_unit : std::ranges::reverse_view{this->units})
      {
        if (file != parent_unit.file || module != parent_unit.module)
          return Error::SectionLayoutOrphanedEntry;
        if (entry_parent_name != parent_unit.name)
          continue;
        line_number += 1;
        head += match.length();
        parent_unit.entry_children.push_back(&this->units.emplace_back(
            xstoul(match.str(1)), xstoul(match.str(2)), xstoul(match.str(3)), 0, match.str(4),
            &parent_unit, std::move(module), std::move(file)));
        goto ENTRY_PARENT_FOUND;  // I wish C++ had for-else clauses.
      }
      return Error::SectionLayoutOrphanedEntry;
    ENTRY_PARENT_FOUND:
      continue;
    }
    if (std::regex_search(head, tail, match, re_section_layout_tloztp_unit_special,
                          std::regex_constants::match_continuous))
    {
      std::string special_name = match.str(5);
      if (special_name != "*fill*" && special_name != "**fill**")
        return Error::SectionLayoutSpecialNotFill;
      line_number += 1;
      head += match.length();
      this->units.emplace_back(xstoul(match.str(1)), xstoul(match.str(2)), xstoul(match.str(3)), 0,
                               std::stoi(match.str(4)), std::move(special_name));
      continue;
    }
    break;
  }
  return Error::None;
}

void Map::SectionLayout::Print(std::ostream& stream) const
{
  // "\r\n\r\n%s section layout\r\n"
  Common::Print(stream, "\r\n\r\n{:s} section layout\r\n", name);
  if (min_version < Version::version_3_0_4)
  {
    Common::Print(stream, "  Starting        Virtual\r\n");
    Common::Print(stream, "  address  Size   address\r\n");
    Common::Print(stream, "  -----------------------\r\n");
    for (const auto& unit : this->units)
      unit.Print3Column(stream);
  }
  else
  {
    Common::Print(stream, "  Starting        Virtual  File\r\n");
    Common::Print(stream, "  address  Size   address  offset\r\n");
    Common::Print(stream, "  ---------------------------------\r\n");
    for (const auto& unit : this->units)
      unit.Print4Column(stream);
  }
}

void Map::SectionLayout::Unit::Print3Column(std::ostream& stream) const
{
  switch (this->unit_kind)
  {
  case Kind::Normal:
    // "  %08x %06x %08x %2i %s \t%s %s\r\n"
    Common::Print(stream, "  {:08x} {:06x} {:08x} {:2d} {:s} \t{:s} {:s}\r\n", starting_address,
                  size, virtual_address, alignment, name, module, file);
    return;
  case Kind::Unused:
    // "  UNUSED   %06x ........ %s %s %s\r\n"
    Common::Print(stream, "  UNUSED   {:06x} ........ {:s} {:s} {:s}\r\n", size, name, module,
                  file);
    return;
  case Kind::Entry:
    // "  %08lx %06lx %08lx %s (entry of %s) \t%s %s\r\n"
    Common::Print(stream, "  {:08x} {:06x} {:08x} {:s} (entry of {:s}) \t{:s} {:s}\r\n",
                  starting_address, size, virtual_address, name, entry_parent->name, module, file);
    return;
  case Kind::Special:
    assert(false);
    return;
  }
}

void Map::SectionLayout::Unit::Print4Column(std::ostream& stream) const
{
  switch (this->unit_kind)
  {
  case Kind::Normal:
    // "  %08x %06x %08x %08x %2i %s \t%s %s\r\n"
    Common::Print(stream, "  {:08x} {:06x} {:08x} {:08x} {:2d} {:s} \t{:s} {:s}\r\n",
                  starting_address, size, virtual_address, file_offset, alignment, name, module,
                  file);
    return;
  case Kind::Unused:
    // "  UNUSED   %06x ........ ........    %s %s %s\r\n"
    Common::Print(stream, "  UNUSED   {:06x} ........ ........    {:s} {:s} {:s}\r\n", size, name,
                  module, file);
    return;
  case Kind::Entry:
    // "  %08lx %06lx %08lx %08lx    %s (entry of %s) \t%s %s\r\n"
    Common::Print(stream, "  {:08x} {:06x} {:08x} {:08x}    {:s} (entry of {:s}) \t{:s} {:s}\r\n",
                  starting_address, size, virtual_address, file_offset, name, entry_parent->name,
                  module, file);
    return;
  case Kind::Special:
    // "  %08x %06x %08x %08x %2i %s\r\n"
    Common::Print(stream, "  {:08x} {:06x} {:08x} {:08x} {:2d} {:s}\r\n", starting_address, size,
                  virtual_address, file_offset, alignment, name);
    return;
  }
}

// void Map::SectionLayout::Export(Common::IntermediateDB& db) const
// {
//   for (const auto& unit : this->units)
//     unit->Export(db, this->name);
// }
// void Map::SectionLayout::UnitReal::Export(Common::IntermediateDB& db,
//                                           const std::string& section_name) const
// {
//   auto& db_module =
//       file.empty() ? db.modules[this->module] : db.modules[this->module + " " + this->file];
//   auto& db_symbol = db_module.symbols[this->name];
//   db_symbol.name = this->name;
//   db_symbol.value = this->virtual_address;
//   db_symbol.size = this->size;
//   if (this->name == section_name)
//   {
//     db_symbol.type = Common::IntermediateDB::Symbol::Type::Section;
//     db_symbol.bind = Common::IntermediateDB::Symbol::Bind::Local;
//   }
// }
// void Map::SectionLayout::UnitSpecial::Export(Common::IntermediateDB& db,
//                                              const std::string& section_name) const
// {
// }

// clang-format off
static const std::regex re_memory_map_unit_normal_simple_old{
//  "  %15s  %08x %08x %08x\r\n"
    "   {0,15}(.*)  ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8})\r?\n"};
// clang-format on

Map::Error Map::MemoryMap::ScanSimple_old(const char*& head, const char* const tail,
                                          std::size_t& line_number)
{
  std::cmatch match;

  while (std::regex_search(head, tail, match, re_memory_map_unit_normal_simple_old,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    this->normal_units.emplace_back(match.str(1), xstoul(match.str(2)), xstoul(match.str(3)),
                                    xstoul(match.str(4)));
  }
  return ScanDebug_old(head, tail, line_number);
}

// clang-format off
static const std::regex re_memory_map_unit_normal_romram_old{
//  "  %15s  %08x %08x %08x %08x %08x\r\n"
    "   {0,15}(.*)  ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8})\r?\n"};
// clang-format on

Map::Error Map::MemoryMap::ScanRomRam_old(const char*& head, const char* const tail,
                                          std::size_t& line_number)
{
  std::cmatch match;

  while (std::regex_search(head, tail, match, re_memory_map_unit_normal_romram_old,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    this->normal_units.emplace_back(match.str(1), xstoul(match.str(2)), xstoul(match.str(3)),
                                    xstoul(match.str(4)), xstoul(match.str(5)),
                                    xstoul(match.str(6)));
  }
  return ScanDebug_old(head, tail, line_number);
}

// clang-format off
static const std::regex re_memory_map_unit_debug_old{
//  "  %15s           %06x %08x\r\n" <-- Sometimes the size can overflow six digits
//  "  %15s           %08x %08x\r\n" <-- Starting with CodeWarrior for GCN 2.7
    "   {0,15}(.*)           ([0-9a-f]{6,8}) ([0-9a-f]{8})\r?\n"};
// clang-format on

Map::Error Map::MemoryMap::ScanDebug_old(const char*& head, const char* const tail,
                                         std::size_t& line_number)
{
  std::cmatch match;

  while (std::regex_search(head, tail, match, re_memory_map_unit_debug_old,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    std::string temp = match.str(2);
    if (temp.length() == 8 && temp.front() == '0')  // Make sure it's not just an overflowed value
      this->SetMinVersion(Version::version_3_0_4);
    this->debug_units.emplace_back(match.str(1), xstoul(match.str(2)), xstoul(match.str(3)));
  }
  return Error::None;
}

// clang-format off
static const std::regex re_memory_map_unit_normal_simple{
//  "  %20s %08x %08x %08x\r\n"
    "   {0,20}(.*) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8})\r?\n"};
// clang-format on

Map::Error Map::MemoryMap::ScanSimple(const char*& head, const char* const tail,
                                      std::size_t& line_number)
{
  std::cmatch match;

  while (std::regex_search(head, tail, match, re_memory_map_unit_normal_simple,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    this->normal_units.emplace_back(match.str(1), xstoul(match.str(2)), xstoul(match.str(3)),
                                    xstoul(match.str(4)));
  }
  return ScanDebug(head, tail, line_number);
}

// clang-format off
static const std::regex re_memory_map_unit_normal_romram{
//  "  %20s %08x %08x %08x %08x %08x\r\n"
    "   {0,20}(.*) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8})\r?\n"};
// clang-format on

Map::Error Map::MemoryMap::ScanRomRam(const char*& head, const char* const tail,
                                      std::size_t& line_number)
{
  std::cmatch match;

  while (std::regex_search(head, tail, match, re_memory_map_unit_normal_romram,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    this->normal_units.emplace_back(match.str(1), xstoul(match.str(2)), xstoul(match.str(3)),
                                    xstoul(match.str(4)), xstoul(match.str(5)),
                                    xstoul(match.str(6)));
  }
  return ScanDebug(head, tail, line_number);
}

// clang-format off
static const std::regex re_memory_map_unit_normal_srecord{
//  "  %20s %08x %08x %08x %10i\r\n"
    "   {0,20}(.*) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8})  {0,9}(\\d+)\r?\n"};
// clang-format on

Map::Error Map::MemoryMap::ScanSRecord(const char*& head, const char* const tail,
                                       std::size_t& line_number)
{
  std::cmatch match;

  while (std::regex_search(head, tail, match, re_memory_map_unit_normal_srecord,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    this->normal_units.emplace_back(match.str(1), xstoul(match.str(2)), xstoul(match.str(3)),
                                    xstoul(match.str(4)), std::stoi(match.str(5)));
  }
  return ScanDebug(head, tail, line_number);
}

// clang-format off
static const std::regex re_memory_map_unit_normal_binfile{
//  "  %20s %08x %08x %08x %08x %s\r\n"
    "   {0,20}(.*) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8}) (.*)\r?\n"};
// clang-format on

Map::Error Map::MemoryMap::ScanBinFile(const char*& head, const char* const tail,
                                       std::size_t& line_number)
{
  std::cmatch match;

  while (std::regex_search(head, tail, match, re_memory_map_unit_normal_binfile,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    this->normal_units.emplace_back(match.str(1), xstoul(match.str(2)), xstoul(match.str(3)),
                                    xstoul(match.str(4)), xstoul(match.str(5)), match.str(6));
  }
  return ScanDebug(head, tail, line_number);
}

// clang-format off
static const std::regex re_memory_map_unit_normal_romram_srecord{
//  "  %20s %08x %08x %08x %08x %08x %10i\r\n"
    "   {0,20}(.*) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8})  {0,9}(\\d+)\r?\n"};
// clang-format on

Map::Error Map::MemoryMap::ScanRomRamSRecord(const char*& head, const char* const tail,
                                             std::size_t& line_number)
{
  std::cmatch match;

  while (std::regex_search(head, tail, match, re_memory_map_unit_normal_romram_srecord,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    this->normal_units.emplace_back(match.str(1), xstoul(match.str(2)), xstoul(match.str(3)),
                                    xstoul(match.str(4)), xstoul(match.str(5)),
                                    xstoul(match.str(6)), std::stoi(match.str(7)));
  }
  return ScanDebug(head, tail, line_number);
}

// clang-format off
static const std::regex re_memory_map_unit_normal_romram_binfile{
//  "  %20s %08x %08x %08x %08x %08x   %08x %s\r\n"
    "   {0,20}(.*) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8})   ([0-9a-f]{8}) (.*)\r?\n"};
// clang-format on

Map::Error Map::MemoryMap::ScanRomRamBinFile(const char*& head, const char* const tail,
                                             std::size_t& line_number)
{
  std::cmatch match;

  while (std::regex_search(head, tail, match, re_memory_map_unit_normal_romram_binfile,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    this->normal_units.emplace_back(match.str(1), xstoul(match.str(2)), xstoul(match.str(3)),
                                    xstoul(match.str(4)), xstoul(match.str(5)),
                                    xstoul(match.str(6)), xstoul(match.str(7)), match.str(8));
  }
  return ScanDebug(head, tail, line_number);
}

// clang-format off
static const std::regex re_memory_map_unit_normal_srecord_binfile{
//  "  %20s %08x %08x %08x  %10i %08x %s\r\n"
    "   {0,20}(.*) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8})   {0,9}(\\d+) ([0-9a-f]{8}) (.*)\r?\n"};
// clang-format on

Map::Error Map::MemoryMap::ScanSRecordBinFile(const char*& head, const char* const tail,
                                              std::size_t& line_number)
{
  std::cmatch match;

  while (std::regex_search(head, tail, match, re_memory_map_unit_normal_srecord_binfile,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    this->normal_units.emplace_back(match.str(1), xstoul(match.str(2)), xstoul(match.str(3)),
                                    xstoul(match.str(4)), std::stoi(match.str(5)),
                                    xstoul(match.str(6)), match.str(7));
  }
  return ScanDebug(head, tail, line_number);
}

// clang-format off
static const std::regex re_memory_map_unit_normal_romram_srecord_binfile{
//  "  %20s %08x %08x %08x %08x %08x    %10i %08x %s\r\n"
    "   {0,20}(.*) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8}) ([0-9a-f]{8})     {0,9}(\\d+) ([0-9a-f]{8}) (.*)\r?\n"};
// clang-format on

Map::Error Map::MemoryMap::ScanRomRamSRecordBinFile(const char*& head, const char* const tail,
                                                    std::size_t& line_number)
{
  std::cmatch match;

  while (std::regex_search(head, tail, match, re_memory_map_unit_normal_romram_srecord_binfile,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    this->normal_units.emplace_back(match.str(1), xstoul(match.str(2)), xstoul(match.str(3)),
                                    xstoul(match.str(4)), xstoul(match.str(5)),
                                    xstoul(match.str(6)), std::stoi(match.str(7)),
                                    xstoul(match.str(8)), match.str(9));
  }
  return ScanDebug(head, tail, line_number);
}

// clang-format off
static const std::regex re_memory_map_unit_debug{
//  "  %20s          %08x %08x\r\n"
    "   {0,20}(.*)          ([0-9a-f]{8}) ([0-9a-f]{8})\r?\n"};
// clang-format on

Map::Error Map::MemoryMap::ScanDebug(const char*& head, const char* const tail,
                                     std::size_t& line_number)
{
  std::cmatch match;

  while (std::regex_search(head, tail, match, re_memory_map_unit_debug,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    this->debug_units.emplace_back(match.str(1), xstoul(match.str(2)), xstoul(match.str(3)));
  }
  return Error::None;
}

void Map::MemoryMap::Print(std::ostream& stream) const
{
  Common::Print(stream, "\r\n\r\nMemory map:\r\n");
  if (min_version < Version::version_4_2_build_142)
  {
    if (has_rom_ram)
      PrintRomRam_old(stream);
    else
      PrintSimple_old(stream);
    PrintDebug_old(stream);
  }
  else
  {
    if (has_rom_ram)
      if (has_s_record)
        if (has_bin_file)
          PrintRomRamSRecordBinFile(stream);
        else
          PrintRomRamSRecord(stream);
      else if (has_bin_file)
        PrintRomRamBinFile(stream);
      else
        PrintRomRam(stream);
    else if (has_s_record)
      if (has_bin_file)
        PrintSRecordBinFile(stream);
      else
        PrintSRecord(stream);
    else if (has_bin_file)
      PrintBinFile(stream);
    else
      PrintSimple(stream);
    PrintDebug(stream);
  }
}

void Map::MemoryMap::PrintSimple_old(std::ostream& stream) const
{
  // clang-format off
  Common::Print(stream, "                   Starting Size     File\r\n");
  Common::Print(stream, "                   address           Offset\r\n");
  // clang-format on
  for (const auto& unit : normal_units)
    unit.PrintSimple_old(stream);
}
void Map::MemoryMap::UnitNormal::PrintSimple_old(std::ostream& stream) const
{
  // "  %15s  %08x %08x %08x\r\n"
  Common::Print(stream, "  {:15s}  {:08x} {:08x} {:08x}\r\n", name, starting_address, size,
                file_offset);
}

void Map::MemoryMap::PrintRomRam_old(std::ostream& stream) const
{
  // clang-format off
  Common::Print(stream, "                   Starting Size     File     ROM      RAM Buffer\r\n");
  Common::Print(stream, "                   address           Offset   Address  Address\r\n");
  // clang-format on
  for (const auto& unit : normal_units)
    unit.PrintRomRam_old(stream);
}
void Map::MemoryMap::UnitNormal::PrintRomRam_old(std::ostream& stream) const
{
  // "  %15s  %08x %08x %08x %08x %08x\r\n"
  Common::Print(stream, "  {:15s}  {:08x} {:08x} {:08x} {:08x} {:08x}\r\n", name, starting_address,
                size, file_offset, rom_address, ram_buffer_address);
}

void Map::MemoryMap::PrintDebug_old(std::ostream& stream) const
{
  if (min_version < Version::version_3_0_4)
    for (const auto& unit : debug_units)
      unit.Print_older(stream);
  else
    for (const auto& unit : debug_units)
      unit.Print_old(stream);
}
void Map::MemoryMap::UnitDebug::Print_older(std::ostream& stream) const
{
  // "  %15s           %06x %08x\r\n"
  Common::Print(stream, "  {:15s}           {:06x} {:08x}\r\n", name, size, file_offset);
}
void Map::MemoryMap::UnitDebug::Print_old(std::ostream& stream) const
{
  // "  %15s           %08x %08x\r\n"
  Common::Print(stream, "  {:15s}           {:08x} {:08x}\r\n", name, size, file_offset);
}

void Map::MemoryMap::PrintSimple(std::ostream& stream) const
{
  // clang-format off
  Common::Print(stream, "                       Starting Size     File\r\n");
  Common::Print(stream, "                       address           Offset\r\n");
  // clang-format on
  for (const auto& unit : normal_units)
    unit.PrintSimple(stream);
}
void Map::MemoryMap::UnitNormal::PrintSimple(std::ostream& stream) const
{
  // "  %20s %08x %08x %08x\r\n"
  Common::Print(stream, "  {:20s} {:08x} {:08x} {:08x}\r\n", name, starting_address, size,
                file_offset);
}

void Map::MemoryMap::PrintRomRam(std::ostream& stream) const
{
  // clang-format off
  Common::Print(stream, "                       Starting Size     File     ROM      RAM Buffer\r\n");
  Common::Print(stream, "                       address           Offset   Address  Address\r\n");
  // clang-format on
  for (const auto& unit : normal_units)
    unit.PrintRomRam(stream);
}
void Map::MemoryMap::UnitNormal::PrintRomRam(std::ostream& stream) const
{
  // "  %20s %08x %08x %08x %08x %08x\r\n"
  Common::Print(stream, "  {:20s} {:08x} {:08x} {:08x} {:08x} {:08x}\r\n", name, starting_address,
                size, file_offset, rom_address, ram_buffer_address);
}

void Map::MemoryMap::PrintSRecord(std::ostream& stream) const
{
  // clang-format off
  Common::Print(stream, "                       Starting Size     File       S-Record\r\n");
  Common::Print(stream, "                       address           Offset     Line\r\n");
  // clang-format on
  for (const auto& unit : normal_units)
    unit.PrintSRecord(stream);
}
void Map::MemoryMap::UnitNormal::PrintSRecord(std::ostream& stream) const
{
  // "  %20s %08x %08x %08x %10i\r\n"
  Common::Print(stream, "  {:20s} {:08x} {:08x} {:08x} {:10d}\r\n", name, starting_address, size,
                file_offset, s_record_line);
}

void Map::MemoryMap::PrintBinFile(std::ostream& stream) const
{
  // clang-format off
  Common::Print(stream, "                       Starting Size     File     Bin File Bin File\r\n");
  Common::Print(stream, "                       address           Offset   Offset   Name\r\n");
  // clang-format on
  for (const auto& unit : normal_units)
    unit.PrintBinFile(stream);
}
void Map::MemoryMap::UnitNormal::PrintBinFile(std::ostream& stream) const
{
  // "  %20s %08x %08x %08x %08x %s\r\n"
  Common::Print(stream, "  {:20s} {:08x} {:08x} {:08x} {:08x} {:s}\r\n", name, starting_address,
                size, file_offset, bin_file_offset, bin_file_name);
}

void Map::MemoryMap::PrintRomRamSRecord(std::ostream& stream) const
{
  // clang-format off
  Common::Print(stream, "                       Starting Size     File     ROM      RAM Buffer  S-Record\r\n");
  Common::Print(stream, "                       address           Offset   Address  Address     Line\r\n");
  // clang-format on
  for (const auto& unit : normal_units)
    unit.PrintRomRamSRecord(stream);
}
void Map::MemoryMap::UnitNormal::PrintRomRamSRecord(std::ostream& stream) const
{
  // "  %20s %08x %08x %08x %08x %08x %10i\r\n"
  Common::Print(stream, "  {:20s} {:08x} {:08x} {:08x} {:08x} {:08x} {:10d}\r\n", name,
                starting_address, size, file_offset, rom_address, ram_buffer_address,
                s_record_line);
}

void Map::MemoryMap::PrintRomRamBinFile(std::ostream& stream) const
{
  // clang-format off
  Common::Print(stream, "                       Starting Size     File     ROM      RAM Buffer Bin File Bin File\r\n");
  Common::Print(stream, "                       address           Offset   Address  Address    Offset   Name\r\n");
  // clang-format on
  for (const auto& unit : normal_units)
    unit.PrintRomRamBinFile(stream);
}
void Map::MemoryMap::UnitNormal::PrintRomRamBinFile(std::ostream& stream) const
{
  // "  %20s %08x %08x %08x %08x %08x   %08x %s\r\n"
  Common::Print(stream, "  {:20s} {:08x} {:08x} {:08x} {:08x} {:08x}   {:08x} {:s}\r\n", name,
                starting_address, size, file_offset, rom_address, ram_buffer_address,
                bin_file_offset, bin_file_name);
}

void Map::MemoryMap::PrintSRecordBinFile(std::ostream& stream) const
{
  // clang-format off
  Common::Print(stream, "                       Starting Size     File        S-Record Bin File Bin File\r\n");
  Common::Print(stream, "                       address           Offset      Line     Offset   Name\r\n");
  // clang-format on
  for (const auto& unit : normal_units)
    unit.PrintSRecordBinFile(stream);
}
void Map::MemoryMap::UnitNormal::PrintSRecordBinFile(std::ostream& stream) const
{
  // "  %20s %08x %08x %08x  %10i %08x %s\r\n"
  Common::Print(stream, "  {:20s} {:08x} {:08x} {:08x}  {:10d} {:08x} {:s}\r\n", name,
                starting_address, size, file_offset, s_record_line, bin_file_offset, bin_file_name);
}

void Map::MemoryMap::PrintRomRamSRecordBinFile(std::ostream& stream) const
{
  // clang-format off
  Common::Print(stream, "                       Starting Size     File     ROM      RAM Buffer    S-Record Bin File Bin File\r\n");
  Common::Print(stream, "                       address           Offset   Address  Address       Line     Offset   Name\r\n");
  // clang-format on
  for (const auto& unit : normal_units)
    unit.PrintRomRamSRecordBinFile(stream);
}
void Map::MemoryMap::UnitNormal::PrintRomRamSRecordBinFile(std::ostream& stream) const
{
  // "  %20s %08x %08x %08x %08x %08x    %10i %08x %s\r\n"
  Common::Print(stream, "  {:20s} {:08x} {:08x} {:08x} {:08x} {:08x}    {:10d} {:08x} {:s}\r\n",
                name, starting_address, size, file_offset, rom_address, ram_buffer_address,
                s_record_line, bin_file_offset, bin_file_name);
}

void Map::MemoryMap::PrintDebug(std::ostream& stream) const
{
  for (const auto& unit : debug_units)
    unit.Print(stream);
}
void Map::MemoryMap::UnitDebug::Print(std::ostream& stream) const
{
  // "  %20s          %08x %08x\r\n"
  Common::Print(stream, "  {:20s}          {:08x} {:08x}\r\n", name, size, file_offset);
}

// clang-format off
static const std::regex re_linker_generated_symbols_unit{
//  "%25s %08x\r\n"
    " {0,25}(.*) ([0-9a-f]{8})\r?\n"};
// clang-format on

Map::Error Map::LinkerGeneratedSymbols::Scan(const char*& head, const char* const tail,
                                             std::size_t& line_number)
{
  std::cmatch match;

  while (std::regex_search(head, tail, match, re_linker_generated_symbols_unit,
                           std::regex_constants::match_continuous))
  {
    line_number += 1;
    head += match.length();
    this->units.emplace_back(match.str(1), xstoul(match.str(2)));
  }
  return Error::None;
}

void Map::LinkerGeneratedSymbols::Print(std::ostream& stream) const
{
  Common::Print(stream, "\r\n\r\nLinker generated symbols:\r\n");
  for (const auto& unit : units)
    unit.Print(stream);
}

void Map::LinkerGeneratedSymbols::Unit::Print(std::ostream& stream) const
{
  // "%25s %08x\r\n"
  Common::Print(stream, "{:25s} {:08x}\r\n", name, value);
}
}  // namespace MWLinker

namespace Common
{
};  // namespace Common
