#include <flatbuffers/idl.h>
#include <igpack-gen/plan-executor.h>
#include <igpack-gen/schema/igpack-plan.h>

#include <CLI/App.hpp>
#include <CLI/Config.hpp>
#include <CLI/Formatter.hpp>
#include <filesystem>
#include <iostream>

int main(int argc, char** argv) {
  CLI::App app{"igpack-gen - a tool for generating Indigo asset pack bundles",
               "igpack-gen"};

  std::string input_asset_path_root;
  app.add_option("-w,--input_asset_path_root", input_asset_path_root,
                 "Root directory for resolving input asset file paths")
      ->required(true)
      ->check(CLI::ExistingDirectory);

  std::string output_asset_path_root;
  app.add_option("-o,--output_asset_path_root", output_asset_path_root,
                 "Root directory used when writing asset pack files")
      ->required(true);

  std::string input_plan_file_path;
  app.add_option(
         "-i,--input_plan_file", input_plan_file_path,
         "Path of the Indigo asset pack plan file (*.igpack-plan.json) that "
         "should be processed by this tool")
      ->required(true)
      ->check(CLI::ExistingFile);

  std::string schema_path;
  app.add_option("-s,--schema", schema_path,
                 "Path of the schema file igpack-plan.fbs that defines how the "
                 "JSON parser should read in igpack-plan files. Assumed to be "
                 "in PATH by "
                 "default, setting this value gives a specific path to search.")
      ->required(false)
      ->check(CLI::ExistingFile);

  CLI11_PARSE(app, argc, argv);

  // Execute plan...
  std::filesystem::path input_path = input_asset_path_root;
  if (!std::filesystem::is_directory(output_asset_path_root) ||
      !std::filesystem::exists(output_asset_path_root)) {
    if (!std::filesystem::create_directories(output_asset_path_root)) {
      std::cerr << "Failed to create output directory "
                << output_asset_path_root << std::endl;
      return -1;
    }
  }
  std::filesystem::path output_path = output_asset_path_root;

  flatbuffers::Parser parser;

  {
    // Copied per CMake file instruction - must be in same directory as binary
    if (schema_path.empty()) {
      schema_path = "igpack-plan.fbs";
    }
    std::ifstream fin(schema_path);
    if (!fin) {
      std::cerr << "Failed to read igpack-plan schema" << std::endl;
      return -1;
    }

    fin.seekg(0, std::ios::end);
    std::string schema_text(fin.tellg(), '\0');
    fin.seekg(0, std::ios::beg);
    schema_text.assign(std::istreambuf_iterator<char>(fin),
                       std::istreambuf_iterator<char>());

    if (!parser.Parse(schema_text.c_str())) {
      std::cerr << "Failed to process igpack-gen schema: " << parser.error_
                << std::endl;
      return -1;
    }
  }

  {
    std::ifstream fin(input_plan_file_path);
    if (!fin) {
      std::cerr << "Failed to read input plan" << std::endl;
      return -1;
    }

    fin.seekg(0, std::ios::end);
    std::string plan_text(fin.tellg(), '\0');
    fin.seekg(0, std::ios::beg);
    plan_text.assign(std::istreambuf_iterator<char>(fin),
                     std::istreambuf_iterator<char>());

    if (!parser.Parse(plan_text.c_str())) {
      std::cerr << "Failed to parse input plan: " << parser.error_ << std::endl;
      return -1;
    }
  }

  const IgpackGen::IgpackGenPlan* igpack_gen_plan =
      IgpackGen::GetIgpackGenPlan(parser.builder_.GetBufferPointer());

  igpackgen::PlanInvocationDesc desc{};
  desc.Plan = igpack_gen_plan;
  desc.InputAssetPathRoot = input_path;
  desc.OutputAssetPathRoot = output_path;

  igpackgen::PlanExecutor executor{};

  if (!executor.execute_plan(desc)) {
    return -1;
  }

  return 0;
}
