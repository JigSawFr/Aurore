// Host tests for cadran/ui/format_text.cpp, translated by Cadran's real
// catalog (resources/i18n/<lang>/cadran.json) through ui/tr.hpp's stand-in
// for borealis (catalog.hpp): minutes ("45 min", "2 h", "2 h 05"), a length
// in nanoseconds rounded to the minute, play time over any length ("< 1 min",
// "52 h 30", "278 h"), the day names (in a title and inside a sentence,
// French for the difference), yes / no, "Unavailable", and the chevron of a
// line that opens a screen. Every key the helpers ask for exists in the
// catalog.
#include <cstdint>
#include <cstdio>
#include <string>
#include "check.h"

#include <borealis/cadran/ui/format_text.hpp>
#include <borealis/cadran/ui/tr.hpp>

#include "catalog.hpp"

static void test_minutes()
{
    CHECK(ui::fmt_minutes(0) == "0 min");
    CHECK(ui::fmt_minutes(1) == "1 min" && ui::fmt_minutes(59) == "59 min");
    CHECK(ui::fmt_minutes(60) == "1 h" && ui::fmt_minutes(120) == "2 h" && ui::fmt_minutes(1440) == "24 h");
    CHECK(ui::fmt_minutes(125) == "2 h 05" && ui::fmt_minutes(150) == "2 h 30");
    CHECK(ui::fmt_minutes(61) == "1 h 01");

    // From nanoseconds: rounded to the nearest minute.
    const uint64_t S = 1000000000ULL, MIN = 60 * S;
    CHECK(ui::fmt_duration_ns(0) == "0 min");
    CHECK(ui::fmt_duration_ns(29 * S) == "0 min" && ui::fmt_duration_ns(30 * S) == "1 min");
    CHECK(ui::fmt_duration_ns(90 * MIN + 29 * S) == "1 h 30");
    CHECK(ui::fmt_duration_ns(90 * MIN + 30 * S) == "1 h 31");

    // Play time, from seconds: "< 1 min" under a minute (not for none), no cap.
    CHECK(ui::fmt_duration(0) == "0 min");
    CHECK(ui::fmt_duration(1) == "< 1 min" && ui::fmt_duration(59) == "< 1 min");
    CHECK(ui::fmt_duration(60) == "1 min" && ui::fmt_duration(89) == "1 min" && ui::fmt_duration(90) == "2 min");
    CHECK(ui::fmt_duration(3600) == "1 h" && ui::fmt_duration(3600 + 5 * 60) == "1 h 05");
    CHECK(ui::fmt_duration(99 * 3600 + 59 * 60) == "99 h 59");
    // From 100 h, whole hours, rounded.
    CHECK(ui::fmt_duration(152 * 3600 + 29 * 60) == "152 h" && ui::fmt_duration(152 * 3600 + 30 * 60) == "153 h");
    CHECK(ui::fmt_duration(277 * 3600 + 58 * 60) == "278 h");
    CHECK(ui::fmt_duration(100000ULL * 3600) == "100000 h");
}

static void test_names()
{
    CHECK(ui::day_name(0) == "Sunday" && ui::day_name(1) == "Monday" && ui::day_name(6) == "Saturday");
    CHECK(ui::day_name(-1) == "?" && ui::day_name(7) == "?");
    CHECK(ui::day_name_in_text(3) == "Wednesday" && ui::day_name_in_text(7) == "?" && ui::day_name_in_text(-1) == "?");

    CHECK(ui::yes_no(true) == "Yes" && ui::yes_no(false) == "No");
    CHECK(ui::bool_text(true, true, "on", "off") == "on" && ui::bool_text(true, false, "on", "off") == "off");
    CHECK(ui::bool_text(false, true, "on", "off") == "Unavailable");

    CHECK(ui::opens() == "›" && ui::opens("") == "›" && ui::opens("12 changes") == "12 changes  ›");
}

// Another language: its own words, its own cases ("lundi" inside a sentence).
static void test_french()
{
    REQUIRE(catalog::load("fr"));
    CHECK(ui::day_name(1) == catalog::strings["cadran/days/1"]);
    CHECK(ui::day_name_in_text(1) == "lundi");
    CHECK(ui::fmt_minutes(125) == "2 h 05");
    CHECK(ui::yes_no(true) == "Oui" && ui::bool_text(false, true, "", "") == "Indisponible");
}

int main()
{
    REQUIRE(catalog::load("en-US"));
    test_minutes();
    test_names();
    test_french();
    for (const std::string& k : catalog::missing)
        std::fprintf(stderr, "no such key: %s\n", k.c_str());
    CHECK(catalog::missing.empty());
    return CHECK_DONE("format_text tests passed");
}
