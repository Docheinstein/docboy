#ifndef RUNTESTS_H
#define RUNTESTS_H

#include <string>
#include <vector>

#include "testutils/runners.h"

void run_test_roms_from_params(const std::string& roms_folder, const std::string& results_folder,
                               const std::vector<RunnerAdapter::Params>& all_params);

void run_test_roms_from_json(const std::string& roms_folder, const std::string& results_folder,
                             const std::string& json_path, const std::string& section_filter = "");

void run_framebuffer_test_roms_from_folder(const std::string& roms_folder, const std::string& results_folder,
                                           const std::string& test_name_filter = "");
void run_framebuffer_test_roms_from_folder_recursive(const std::string& roms_folder, const std::string& results_folder,
                                                     const std::string& test_name_filter = "");

void run_memory_test_roms_from_folder(const std::string& roms_folder, const MemoryExpectation& expectation,
                                      const std::string& test_name_filter = "");
void run_memory_test_roms_from_folder_recursive(const std::string& roms_folder, const MemoryExpectation& expectation,
                                                const std::string& test_name_filter = "");

#endif // RUNTESTS_H
