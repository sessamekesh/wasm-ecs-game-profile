#include <igpack-gen/wgsl-processor.h>

#include <iostream>
#include <sstream>
#include <string>
using namespace igpackgen;

namespace {
const std::string kWhitespaceCharacters = " \n\r\t";

std::string ltrim(std::string s) {
  size_t start = s.find_first_not_of(kWhitespaceCharacters);
  return (start == std::string::npos) ? "" : s.substr(start);
}

std::string rtrim(std::string s) {
  size_t end = s.find_last_not_of(kWhitespaceCharacters);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

std::string trim(std::string s) { return ltrim(rtrim(s)); }

std::string strip_trailing_comment(std::string s) {
  size_t start = s.find("//");
  return (start == std::string::npos) ? s : s.substr(0, start);
}
}  // namespace

bool WgslProcessor::copy_wgsl_source(
    flatbuffers::FlatBufferBuilder& fbb,
    std::vector<flatbuffers::Offset<IgAsset::SingleAsset>>& asset_list,
    const IgpackGen::CopyWgslSourceAction& action,
    const std::string& igasset_name, const std::string& wgsl_source) const {
  if (wgsl_source.empty()) {
    std::cerr << "Cannot copy WGSL source - input is empty" << std::endl;
    return false;
  }

  std::stringstream input(wgsl_source);
  std::stringstream output;

  for (std::string line; std::getline(input, line);) {
    if (action.strip_comments()) {
      line = trim(strip_trailing_comment(line));
      if (line == "") {
        continue;
      }
    }

    output << line << "\n";
  }

  std::string processed_source = output.str();

  auto source = fbb.CreateString(processed_source);
  auto vertex_entry_point = fbb.CreateString(action.vertex_entry_point());
  auto fragment_entry_point = fbb.CreateString(action.fragment_entry_point());
  auto compute_entry_point = fbb.CreateString(action.compute_entry_point());
  auto wgsl_source_data =
      IgAsset::CreateWgslSource(fbb, source, vertex_entry_point,
                                fragment_entry_point, compute_entry_point);

  auto name = fbb.CreateString(igasset_name);

  auto single_asset = IgAsset::CreateSingleAsset(
      fbb, name, IgAsset::SingleAssetData_WgslSource, wgsl_source_data.Union());

  asset_list.push_back(single_asset);
  return true;
}
