#include "odr/session.h"

#include <nlohmann/json.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#if defined(__APPLE__) || defined(__linux__)
#include <sys/resource.h>
#endif

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("cannot read " + path.string());
    }
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void save_file(const std::filesystem::path& path, const std::string& contents) {
    const auto temporary = path.string() + ".pending";
    {
        std::ofstream output(temporary, std::ios::trunc);
        if (!output || !(output << contents << '\n')) {
            throw std::runtime_error("cannot write " + temporary);
        }
        output.flush();
        if (!output) {
            throw std::runtime_error("cannot flush " + temporary);
        }
    }
    std::filesystem::rename(temporary, path);
}

} // namespace

int main(int argc, char** argv) {
    try {
        using Clock = std::chrono::steady_clock;
        const auto started = Clock::now();
        const auto milliseconds = [](Clock::duration duration) {
            return std::chrono::duration<double, std::milli>(duration).count();
        };
        std::filesystem::path definition = "game/content/mine.json";
        std::filesystem::path script;
        std::filesystem::path save;
        std::filesystem::path load;
        bool validate_only = false;
        bool profile = false;
        double generation_ms = 0.0;
        double load_ms = 0.0;
        double save_ms = 0.0;
        std::size_t save_bytes = 0;
        int commands = 0;
        int battle_commands = 0;
        int max_battle_round = 0;
        for (int arg = 1; arg < argc; ++arg) {
            const std::string option = argv[arg];
            if (option == "--definition" && arg + 1 < argc) {
                definition = argv[++arg];
            } else if (option == "--script" && arg + 1 < argc) {
                script = argv[++arg];
            } else if (option == "--save" && arg + 1 < argc) {
                save = argv[++arg];
            } else if (option == "--load" && arg + 1 < argc) {
                load = argv[++arg];
            } else if (option == "--validate") {
                validate_only = true;
            } else if (option == "--profile") {
                profile = true;
            } else {
                throw std::runtime_error("unknown or incomplete option: " + option);
            }
        }
        const std::string definition_text = read_file(definition);
        using Session = std::unique_ptr<OdrSession, decltype(&odr_destroy)>;
        Session session(odr_create(definition_text.c_str()), &odr_destroy);
        if (!session) {
            throw std::runtime_error("adventure definition failed to validate");
        }
        if (!load.empty()) {
            const std::string previous = read_file(load);
            const auto load_started = Clock::now();
            if (!odr_load(session.get(), previous.c_str())) {
                throw std::runtime_error(odr_error(session.get()));
            }
            load_ms = milliseconds(Clock::now() - load_started);
        }
        if (!validate_only) {
            std::ifstream file;
            if (!script.empty()) {
                file.open(script);
                if (!file) {
                    throw std::runtime_error("cannot read scenario " + script.string());
                }
            }
            std::istream& input = script.empty() ? std::cin : file;
            std::string line;
            unsigned long line_number = 0;
            while (std::getline(input, line)) {
                ++line_number;
                if (line.empty() || line.front() == '#') {
                    continue;
                }
                const nlohmann::json command = nlohmann::json::parse(line);
                const nlohmann::json before = nlohmann::json::parse(odr_snapshot(session.get()));
                const auto command_started = Clock::now();
                if (!odr_apply(session.get(), line.c_str())) {
                    throw std::runtime_error("scenario line " + std::to_string(line_number) + ": " +
                                             odr_error(session.get()));
                }
                ++commands;
                if (command.value("action", "") == "create_run") {
                    generation_ms += milliseconds(Clock::now() - command_started);
                }
                if (before["phase"] == "battle") {
                    ++battle_commands;
                    if (before["battle"].is_object()) {
                        max_battle_round =
                            std::max(max_battle_round, before["battle"]["round"].get<int>());
                    }
                }
            }
            const auto save_started = Clock::now();
            const std::string final_save = odr_save(session.get());
            save_ms = milliseconds(Clock::now() - save_started);
            save_bytes = final_save.size();
            if (!save.empty()) {
                save_file(save, final_save);
            }
        }
        if (profile) {
            nlohmann::json metrics = {{"commands", commands},
                                      {"battle_commands", battle_commands},
                                      {"max_battle_round", max_battle_round},
                                      {"generation_ms", generation_ms},
                                      {"load_ms", load_ms},
                                      {"save_ms", save_ms},
                                      {"save_bytes", save_bytes},
                                      {"elapsed_ms", milliseconds(Clock::now() - started)}};
#if defined(__APPLE__) || defined(__linux__)
            rusage usage{};
            if (getrusage(RUSAGE_SELF, &usage) == 0) {
#if defined(__APPLE__)
                metrics["peak_rss_bytes"] = usage.ru_maxrss;
#else
                metrics["peak_rss_bytes"] = usage.ru_maxrss * 1024;
#endif
            }
#endif
            std::cerr << "ODR_PROFILE " << metrics.dump() << '\n';
        }
        std::cout << nlohmann::json::parse(odr_snapshot(session.get())).dump(2) << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "odr_tool: " << error.what() << '\n';
        return 1;
    }
}
