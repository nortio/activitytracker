#include "csv.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>

using Days = std::chrono::duration<int64_t, std::ratio<86400>>;
using SystemClock = std::chrono::system_clock;
using UsageDurations = std::unordered_map<std::string, uint64_t>;

const int COLUMNS = 7;

bool string_to_bool(char *val) {
    if (*val == 't') {
        return true;
    } else {
        return false;
    }
}

bool is_empty(char *str) {
    if (*str == '\0') {
        return true;
    } else {
        return false;
    }
}

char *get_identifier(char *title, char *app_id, char *wm_class) {
    if (wm_class != nullptr && !is_empty(wm_class)) {
        return wm_class;
    }

    if (app_id != nullptr && !is_empty(app_id)) {
        return app_id;
    }

    if (title != nullptr && !is_empty(title)) {
        return title;
    }

    return nullptr;
}

std::string format_duration(uint64_t delta_ms) {
    uint64_t seconds = std::round(delta_ms / 1000.0F);
    uint64_t hours = seconds / 3600;
    uint64_t minutes = (seconds - (hours * 3600)) / 60;
    seconds = (seconds - (hours * 3600) - (minutes * 60));

    return std::format("{:02}:{:02}:{:02}", hours, minutes, seconds);
}

int64_t get_date(uint64_t delta_ms) {
    auto time_point = SystemClock::from_time_t(delta_ms / 1000);
    auto date = std::chrono::floor<Days>(time_point);
    return date.time_since_epoch().count();
    // std::format("{:%F}", date);
}

struct AppUsage {
    std::string identifier;
    uint64_t usage;
};

inline bool operator>(const AppUsage &a, const AppUsage &b) { return a.usage > b.usage; }

int main(int argc, char **argv) {
    if (argc != 3) {
        std::printf("Usage: %s <csv log file> <silent (y/n)>\n", argv[0]);
        return 1;
    }

    uint64_t processed_rows = 0;

    auto start = std::chrono::steady_clock::now();

    io::CSVReader<COLUMNS, io::trim_chars<' '>, io::double_quote_escape<',', '"'>> in(argv[1]);

    std::map<int64_t, UsageDurations> history;

    uint64_t timestamp;
    char *is_locked;
    char *is_idle;
    char *title;
    uint64_t pid;
    char *app_id;
    char *wm_class;

    // first row
    in.read_row(timestamp, is_locked, is_idle, title, pid, app_id, wm_class);
    int64_t date = get_date(timestamp);

    uint64_t previous_timestamp = timestamp;

    while (in.read_row(timestamp, is_locked, is_idle, title, pid, app_id, wm_class)) {
        processed_rows++;

        char *identifier = get_identifier(title, app_id, wm_class);
        bool locked = string_to_bool(is_locked);
        bool idle = string_to_bool(is_idle);

        if (identifier == nullptr || idle || locked) {
            previous_timestamp = timestamp;

            continue;
        }

        auto current_date = get_date(timestamp);
        if (current_date != date) {
            date = current_date;
            history[date] = UsageDurations{};
        }

        uint64_t delta = timestamp - previous_timestamp;

        if (delta < 30000) {
            auto id_string = std::string(identifier);
            history[date][id_string] += delta;
        }

        previous_timestamp = timestamp;
    }

    auto processing_time = std::chrono::steady_clock::now() - start;

    std::cout << std::format("Processed {} rows in {}\n", processed_rows,
                             std::chrono::duration_cast<std::chrono::milliseconds>(processing_time));

    if (*argv[2] == 'y') {
        return 0;
    }

    for (auto [key, val] : history) {
        auto date = std::chrono::time_point<SystemClock>(Days(key));
        std::cout << std::format("{:%F}:\n", date);

        std::set<AppUsage, std::greater<AppUsage>> apps;
        for (auto [app, dur] : val) {
            apps.emplace(app, dur);
        }

        for (auto [app, usage] : apps) {
            std::cout << std::format("\t{}: {}\n", app, format_duration(usage), usage);
        }

        apps.clear();
    }
}
