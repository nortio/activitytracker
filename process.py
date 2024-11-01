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

def increment_count(data, key, amount):
    if key not in data:
        data[key] = 0
    data[key] += amount

def print_history(data):
    for key, value in data.items():
        print(key)
        print_times(value)

def print_times(data):
    lengths = [len(l) for l in data.keys()]
    max_len = 0
    total = 0
    if len(lengths) > 0:
        max_len = max(lengths)
    for key in sorted(data, key=data.get, reverse=True):
        delta = datetime.timedelta(milliseconds=data[key])
        print("\t{} {}".format(key.ljust(max_len), str(delta)))
        total += data[key]
    print("Total: {}".format(datetime.timedelta(milliseconds=total)))

def str_to_bool(val):
    if val == "true":
        return True
    else:
        return False

def get_title(row):
    # Use wmclass first
    title = row[-1]

    # if wmclass is not defined, use application id
    if not title:
        title = row[-2]

    # if app id is not defined, use title
    if not title:
        title = row[-4]

    return title


def process(file):
    hist = {}
    with open(file, newline='') as file:
        reader = csv.reader(file, delimiter=',')

        first_row = next(reader)
        previous_timestamp = int(first_row[0])
        previous_title = get_title(first_row)
        previous_date = datetime.date.fromtimestamp(previous_timestamp / 1000).isoformat()
        hist[previous_date] = {}
        
        duration = 0
        
        # Useless
        # if previous_title:
        #    increment_count(hist[previous_date], previous_title, duration)
        
        for row in reader:

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

            if previous_title and not locked and not idle:
                increment_count(hist[previous_date], previous_title, duration)

            previous_title = title 
    return hist
            
            
if __name__ == "__main__":
    data = process(sys.argv[1])
    print_history(data)
    
