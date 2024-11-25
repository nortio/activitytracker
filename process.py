#!/usr/bin/env python3

# Copyright 2024 nortio
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the “Software”), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is furnished to do
# so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

import csv
import sys
import datetime
import re
import os

Filters = dict[str, list[tuple[str, re.Pattern[str]]]]
HistoryEntry = dict[str, int]
History = dict[str, HistoryEntry]


def increment_count(data: HistoryEntry, key: str, amount: int):
    if key not in data:
        data[key] = 0
    data[key] += amount


def print_history(data: History, reports):
    for key, value in data.items():
        print(key)
        times, total = print_times(value)
        print("Total: {}".format(total))
        report_path = os.path.join(reports, key)
        if os.path.exists(report_path):
            report = ""
            with open(report_path, mode="r") as file:
                report = file.read()
            print("Report: ", report)
        else:
            print("No report found for that day")
        print(times)



def add_filter(filters: Filters, key: str, val: tuple[str, re.Pattern[str]]):
    if key not in filters:
        filters[key] = []
    filters[key].append(val)


def load_filters(file_path: str) -> Filters:
    with open(file_path, mode="r") as data:
        current = ""
        filters: Filters = {}
        for line in data:
            if line.startswith("#"):
                continue
            if line.strip() == "":
                continue
            if line.startswith("\t"):
                name, pattern = line.strip().split(":", 1)
                add_filter(filters, current, (name, re.compile(pattern.strip())))
            else:
                current = line.strip()
        return filters


def print_times(data):
    lengths = [len(l) for l in data.keys()]
    max_len = 0
    total = 0
    final = ""
    if len(lengths) > 0:
        max_len = max(lengths)
    for key in sorted(data, key=data.get, reverse=True):
        delta = datetime.timedelta(milliseconds=data[key])
        final += "\t{} {}\n".format(key.ljust(max_len), str(delta))
        total += data[key]
    total_string = datetime.timedelta(milliseconds=total)
    return final, total_string


def str_to_bool(val: str):
    if val == "true":
        return True
    else:
        return False


def get_title(row: list[str]):
    # Use wmclass first
    title = row[-1]

    # if wmclass is not defined, use application id
    if not title:
        title = row[-2]

    # if app id is not defined, use title
    if not title:
        title = row[-4]

    return title


def process(file_path: str, filters: Filters) -> History:
    hist: History = {}
    with open(file_path, newline="") as file:
        reader = csv.reader(file, delimiter=",")

        first_row = next(reader)
        previous_timestamp = int(first_row[0])
        previous_title = get_title(first_row)
        previous_date = datetime.date.fromtimestamp(
            previous_timestamp / 1000
        ).isoformat()
        hist[previous_date] = {}

        duration = 0

        # Useless
        # if previous_title:
        #    increment_count(hist[previous_date], previous_title, duration)

        for row in reader:
            if len(row) != 7:
                print("Invalid row:", row)
                pass

            timestamp = int(row[0])
            date = datetime.date.fromtimestamp(timestamp / 1000).isoformat()

            if previous_date != date:
                previous_date = date
                hist[previous_date] = {}

            duration = timestamp - previous_timestamp
            previous_timestamp = timestamp

            locked = str_to_bool(row[1])
            idle = str_to_bool(row[2])

            title = get_title(row)
            if title in filters:
                for val in filters[title]:
                    if val[1].match(row[-4]):
                        title = val[0]

            if previous_title and (not locked and not idle):
                # this is necessary because the extension does not account for shutdowns
                # where time could be counted for the last app even when the computer is off
                if duration < 30000:
                    increment_count(hist[previous_date], previous_title, duration)

            previous_title = title
    return hist


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: process.py <log> <filters> <reports-folder>")
        exit(1)
    filters = load_filters(sys.argv[2])
    reports = sys.argv[3]
    print("Loaded filters: {}".format(filters))
    data = process(sys.argv[1], filters)
    print_history(data, reports)
