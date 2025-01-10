#include "csv.h"
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <format>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

using Days = std::chrono::duration<int64_t, std::ratio<86400>>;
using SystemClock = std::chrono::system_clock;
using UsageDurations = std::unordered_map<std::string, uint64_t>;

#if __has_include(<boost/regex.hpp>)
#include <boost/regex.hpp>
using Regex = boost::regex;
#else
#include <regex>
using Regex = std::regex;
#endif

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

// awful
char *ltrim(char *str) {
    char *s = str;
    while (isspace(*s))
        s++;
    return s;
}

// awful pt2
char *rtrim(char *str) {
    size_t len = strlen(str);
    len--;
    while (len > 0) {
        if (isspace(str[len])) {
            str[len] = 0;
        } else {
            break;
        }
        len--;
    };
    return str;
}

// idc
char *trim(char *str) {
    char *s = ltrim(str);
    return rtrim(s);
}

using RegexPair = std::tuple<boost::regex, std::string>;
using Filters = std::unordered_map<std::string, std::vector<RegexPair>>;

Filters load_filters(const char *file) {
    auto filters = Filters();

    FILE *f = fopen(file, "r");
    if (!f) {
        perror("load_filters fopen");
        return filters;
    }

    char line[256] = {0};


    std::string current = "";

    while (fgets(line, 256, f)) {
        char first_char = *line;

        switch (first_char) {
        case '#': {
            continue;
        }
        case '\t': {
            // TODO: make this more robust and secure
            line[strcspn(line, "\n")] = 0;
            char *filt = line + 1;
            size_t l = strcspn(filt, ":");
            filt[l] = 0;
            auto label = std::string(filt);
            auto regex_string = std::string(trim(&filt[++l]));

            auto regex = Regex(regex_string, Regex::optimize);

            filters[current].emplace_back(regex, label);

            std::cout << std::format("\t{}:{}\n", label, regex_string);
            break;
        }
        case '\n': {
            continue;
        }
        default: {
            line[strcspn(line, "\n")] = 0;
            current = std::string(line);
            printf("%s\n", line);
            break;
        }
        }
    }

    return filters;
}

inline bool operator>(const AppUsage &a, const AppUsage &b) { return a.usage > b.usage; }

int main(int argc, char **argv) {
    if (argc != 4) {
        std::printf("Usage: %s <csv log file> <filters> <silent (y/n)>\n", argv[0]);
        return 1;
    }

    bool silent = *argv[3] == 'y' ? true : false;
    char *filters_file = argv[2];
    char *log_file = argv[1];

    std::puts("Loaded filters:\n");
    Filters filters = load_filters(filters_file);

    uint64_t processed_rows = 0;

    auto start = std::chrono::steady_clock::now();

    io::CSVReader<COLUMNS, io::trim_chars<' '>, io::double_quote_escape<',', '"'>> in(log_file);

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
            auto it = filters.find(id_string);
            if(it != filters.end()) {
                auto regexes = it->second;
                for(const auto& [regex, label] : regexes) {
                    if(regex_match(title, regex)) {
                        id_string = label;
                    }
                }
            }
            history[date][id_string] += delta;
        }

        previous_timestamp = timestamp;
    }

    auto processing_time = std::chrono::steady_clock::now() - start;

    std::cout << std::format("Processed {} rows in {}\n", processed_rows,
                             std::chrono::duration_cast<std::chrono::milliseconds>(processing_time));

    if (silent) {
        return 0;
    }

    for (auto [key, val] : history) {
        auto date = std::chrono::time_point<SystemClock>(Days(key));
        std::cout << std::format("{:%F}:\n", date);
        uint64_t total = 0;

        std::set<AppUsage, std::greater<AppUsage>> apps;
        for (auto [app, dur] : val) {
            apps.emplace(app, dur);
            total += dur;
        }

        std::cout << std::format("Total: {}\n", format_duration(total));

        for (auto [app, usage] : apps) {
            std::cout << std::format("\t{}: {}\n", app, format_duration(usage), usage);
        }

        apps.clear();
    }
}
