/*
 * The Restful Matching-Engine.
 * Copyright (C) 2013, 2016 Swirly Cloud Limited.
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if
 * not, write to the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include <test/Test.hpp>

#include <iostream>
#include <regex>

#undef SWIRLY_TEST_CASE
#define SWIRLY_TEST_CASE(name) void test##name();
#include <TestCases.txt>

#undef SWIRLY_TEST_CASE
#define SWIRLY_TEST_CASE(name) {#name, test##name},

namespace {
static const struct TestCase {
  const char* name;
  void (*fn)(void);
} TestCases[] = {
#include <TestCases.txt>
};

bool run(const TestCase& tc)
{
  using namespace std;

  bool pass{false};

  cout << "checking " << tc.name << " ... ";
  cout.flush();
  try {
    tc.fn();
    cout << "pass" << endl;
    pass = true;
  } catch (const TestException& e) {
    cout << "fail" << endl;
    cerr << e.file() << ':' << e.line() << ": " << e.what() << endl;
  } catch (const exception& e) {
    cout << "fail" << endl;
    cerr << "exception: " << e.what() << endl;
  }
  return pass;
}
} // anonymous

int main(int argc, char* argv[])
{
  using namespace std;

  srand(time(nullptr));

  int failed{0}, total{0};
  if (argc > 1) {
    // Run specific test-cases according to regex arguments.
    for (int i{1}; i < argc; ++i) {
      const regex pattern{argv[i]};
      for (auto& tc : TestCases) {
        if (regex_match(tc.name, pattern)) {
          if (!run(tc)) {
            ++failed;
          }
          ++total;
        }
      }
    }
  } else {
    // Run all test-cases.
    for (auto& tc : TestCases) {
      if (!run(tc)) {
        ++failed;
      }
      ++total;
    }
  }
  int ret{1};
  if (total == 0) {
    cerr << "no tests found\n";
  } else if (failed > 0) {
    cerr << failed << " test(s) out of " << total << " failed\n";
  } else {
    cout << "all tests passed\n";
    ret = 0;
  }
  return ret;
}