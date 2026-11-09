// Unit tests for the iCalendar parser embedded in dynamic-island-for-windows.wh.cpp.
// Build (from the repo root), after extracting the engine markers into /tmp/ics_engine.inc:
//   g++ -std=c++20 -Wall -Wextra -Werror -I/tmp tests/ics_parser_test.cpp -o /tmp/ics_parser_test
//   TZ=America/Los_Angeles /tmp/ics_parser_test
//
// The engine region is extracted so this file can run on Linux. Windhawk compiles
// the same text as part of the mod, where it converts instants to the PC's local time.

#include "ics_engine.inc"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

int g_failed = 0;

void Expect(bool cond, const char* message) {
    if (!cond) {
        std::cerr << "FAIL: " << message << "\n";
        ++g_failed;
    }
}

std::string NewYorkZone() {
    return
        "BEGIN:VTIMEZONE\r\n"
        "TZID:America/New_York\r\n"
        "BEGIN:DAYLIGHT\r\n"
        "TZOFFSETFROM:-0500\r\n"
        "TZOFFSETTO:-0400\r\n"
        "TZNAME:EDT\r\n"
        "DTSTART:19700308T020000\r\n"
        "RRULE:FREQ=YEARLY;BYMONTH=3;BYDAY=2SU\r\n"
        "END:DAYLIGHT\r\n"
        "BEGIN:STANDARD\r\n"
        "TZOFFSETFROM:-0400\r\n"
        "TZOFFSETTO:-0500\r\n"
        "TZNAME:EST\r\n"
        "DTSTART:19701101T020000\r\n"
        "RRULE:FREQ=YEARLY;BYMONTH=11;BYDAY=1SU\r\n"
        "END:STANDARD\r\n"
        "END:VTIMEZONE\r\n";
}

di_ics::ExpandWindow YearWindow2026() {
    return di_ics::WindowFromLocalDates(di_ics::Civil{2026, 1, 1, 0, 0, 0},
                                        di_ics::Civil{2026, 8, 1, 0, 0, 0});
}

const di_ics::Occurrence* FindUtc(const std::vector<di_ics::Occurrence>& items, di_ics::Civil utcCivil) {
    const int64_t stamp = di_ics::CivilToUnixUtc(utcCivil);
    for (const auto& item : items) {
        if (item.startUtc == stamp) return &item;
    }
    return nullptr;
}

void TestWeekdayAnchor() {
    // 2026-01-01 was a Thursday, so the 6th is a Tuesday.
    Expect(di_ics::Weekday(di_ics::Civil{2026, 1, 1, 0, 0, 0}) == 4, "Jan 1 2026 is Thursday");
    Expect(di_ics::Weekday(di_ics::Civil{2026, 1, 6, 0, 0, 0}) == 2, "Jan 6 2026 is Tuesday");
    Expect(di_ics::Weekday(di_ics::Civil{2026, 3, 8, 0, 0, 0}) == 0, "Mar 8 2026 is Sunday");
    Expect(di_ics::Weekday(di_ics::Civil{2026, 11, 1, 0, 0, 0}) == 0, "Nov 1 2026 is Sunday");
}

void TestWeeklyClassExdateAndOverride() {
    // Tuesday/Thursday 10:10-11:25 America/New_York.
    // Jan 13 (Tuesday) is an EXDATE. Jan 15 (Thursday) is moved to 14:00 via RECURRENCE-ID.
    // The summary is folded and contains an escaped comma.
    const std::string ics =
        "BEGIN:VCALENDAR\r\n"
        "VERSION:2.0\r\n" +
        NewYorkZone() +
        "BEGIN:VEVENT\r\n"
        "UID:cs4789@test\r\n"
        "SUMMARY:CS 4789\\, Lab\r\n"
        "LOCATION:Gates Hall\r\n"
        "DTSTART;TZID=America/New_York:20260106T101000\r\n"
        "DTEND;TZID=America/New_York:20260106T112500\r\n"
        "RRULE:FREQ=WEEKLY;WKST=SU;BYDAY=TU,TH\r\n"
        "EXDATE;TZID=America/New_York:20260113T101000\r\n"
        "END:VEVENT\r\n"
        "BEGIN:VEVENT\r\n"
        "UID:cs4789@test\r\n"
        "SUMMARY:CS 4789 moved\r\n"
        "LOCATION:Zoom\r\n"
        "DTSTART;TZID=America/New_York:20260115T140000\r\n"
        "DTEND;TZID=America/New_York:20260115T151500\r\n"
        "RECURRENCE-ID;TZID=America/New_York:20260115T101000\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR\r\n";

    const auto items = di_ics::ExpandCalendar(ics, YearWindow2026());

    // Winter: 10:10 EST (UTC-5) is 15:10 UTC. Jan 6 Tuesday and Jan 8 Thursday survive.
    const auto* jan6 = FindUtc(items, {2026, 1, 6, 15, 10, 0});
    const auto* jan8 = FindUtc(items, {2026, 1, 8, 15, 10, 0});
    Expect(jan6 != nullptr, "Jan 6 class is present");
    Expect(jan8 != nullptr, "Jan 8 class is present");
    if (jan6) {
        Expect(jan6->summary == "CS 4789, Lab", "escaped comma in SUMMARY");
        Expect(jan6->location == "Gates Hall", "LOCATION kept");
        Expect(!jan6->allDay, "class is timed");
        Expect(jan6->endUtc - jan6->startUtc == 75 * 60, "class lasts 75 minutes");
    }

    Expect(FindUtc(items, {2026, 1, 13, 15, 10, 0}) == nullptr, "EXDATE removed Jan 13");
    Expect(FindUtc(items, {2026, 1, 15, 15, 10, 0}) == nullptr, "RECURRENCE-ID removed the original Jan 15");

    const auto* moved = FindUtc(items, {2026, 1, 15, 19, 0, 0});
    Expect(moved != nullptr, "moved class is at 14:00 Eastern (19:00 UTC)");
    if (moved) {
        Expect(moved->summary == "CS 4789 moved", "override summary");
        Expect(moved->location == "Zoom", "override location");
    }

    // The next Tuesday after the exception still happens.
    Expect(FindUtc(items, {2026, 1, 20, 15, 10, 0}) != nullptr, "Jan 20 class is present");

    // Summer: 10:10 EDT (UTC-4) is 14:10 UTC. Proves the VTIMEZONE rule is applied
    // per occurrence rather than a hardcoded offset.
    bool sawJuly = false;
    for (const auto& item : items) {
        if (item.summary != "CS 4789, Lab") continue;
        const di_ics::Civil utc = di_ics::UnixUtcToUtcCivil(item.startUtc);
        if (utc.month == 7 && utc.hour == 14 && utc.minute == 10) sawJuly = true;
    }
    Expect(sawJuly, "July class is 10:10 EDT (14:10 UTC)");

    // PC local time. The test process is America/Los_Angeles: 15:10 UTC is 07:10 PST.
    if (jan6) {
        Expect(jan6->hour == 7 && jan6->minute == 10, "Jan 6 is 07:10 in Los Angeles");
        Expect(jan6->year == 2026 && jan6->month == 1 && jan6->day == 6, "Jan 6 local date");
    }
}

void TestAllDay() {
    const std::string ics =
        "BEGIN:VCALENDAR\r\n"
        "BEGIN:VEVENT\r\n"
        "SUMMARY:Career fair\r\n"
        "DTSTART;VALUE=DATE:20260120\r\n"
        "DTEND;VALUE=DATE:20260122\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR\r\n";
    const auto items = di_ics::ExpandCalendar(ics, YearWindow2026());
    Expect(items.size() == 1, "one all-day event");
    if (items.size() == 1) {
        const auto& ev = items[0];
        Expect(ev.allDay, "VALUE=DATE is all-day");
        Expect(ev.summary == "Career fair", "all-day summary");
        Expect(ev.year == 2026 && ev.month == 1 && ev.day == 20, "all-day starts Jan 20");
        Expect(ev.spanEndYear == 2026 && ev.spanEndMonth == 1 && ev.spanEndDay == 22,
               "all-day end is exclusive Jan 22");
        // A date must not slide to the previous evening just because the PC is behind UTC.
        Expect(ev.day == 20, "all-day date is not converted through UTC");
    }
}

void TestUtcEvent() {
    const std::string ics =
        "BEGIN:VCALENDAR\r\n"
        "BEGIN:VEVENT\r\n"
        "SUMMARY:UTC seminar\r\n"
        "LOCATION:Room 4\r\n"
        "DTSTART:20260315T150000Z\r\n"
        "DTEND:20260315T160000Z\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR\r\n";
    const auto items = di_ics::ExpandCalendar(ics, YearWindow2026());
    Expect(items.size() == 1, "one UTC event");
    if (items.empty()) return;
    const auto& ev = items[0];
    Expect(ev.startUtc == di_ics::CivilToUnixUtc({2026, 3, 15, 15, 0, 0}), "UTC stamp kept");
    Expect(ev.endUtc - ev.startUtc == 3600, "UTC event lasts one hour");
    // March 15 2026 is after the US DST change, so Los Angeles is UTC-7.
    Expect(ev.hour == 8 && ev.minute == 0, "15:00 UTC is 08:00 in Los Angeles");
    Expect(ev.day == 15 && ev.month == 3, "UTC event stays on March 15 locally");
}

void TestTzidWithoutHardcoding() {
    // A zone that is not New York, described only by its VTIMEZONE block.
    const std::string ics =
        "BEGIN:VCALENDAR\r\n"
        "BEGIN:VTIMEZONE\r\n"
        "TZID:Pacific/Auckland\r\n"
        "BEGIN:STANDARD\r\n"
        "TZOFFSETFROM:+1300\r\n"
        "TZOFFSETTO:+1200\r\n"
        "DTSTART:19700405T030000\r\n"
        "RRULE:FREQ=YEARLY;BYMONTH=4;BYDAY=1SU\r\n"
        "END:STANDARD\r\n"
        "BEGIN:DAYLIGHT\r\n"
        "TZOFFSETFROM:+1200\r\n"
        "TZOFFSETTO:+1300\r\n"
        "DTSTART:19700927T020000\r\n"
        "RRULE:FREQ=YEARLY;BYMONTH=9;BYDAY=-1SU\r\n"
        "END:DAYLIGHT\r\n"
        "END:VTIMEZONE\r\n"
        "BEGIN:VEVENT\r\n"
        "SUMMARY:Auckland office hours\r\n"
        "DTSTART;TZID=Pacific/Auckland:20260120T090000\r\n"
        "DTEND;TZID=Pacific/Auckland:20260120T100000\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR\r\n";
    const auto items = di_ics::ExpandCalendar(ics, YearWindow2026());
    Expect(items.size() == 1, "Auckland event parsed");
    if (items.empty()) return;
    // January is New Zealand daylight time (UTC+13). 09:00 NZDT = 20:00 UTC the previous day.
    const di_ics::Civil utc = di_ics::UnixUtcToUtcCivil(items[0].startUtc);
    Expect(utc.year == 2026 && utc.month == 1 && utc.day == 19 && utc.hour == 20 && utc.minute == 0,
           "Auckland January offset comes from the feed, not a hardcoded zone");
}

void TestFoldingEscapesIntervalAndUntil() {
    const std::string ics =
        "BEGIN:VCALENDAR\r\n" +
        NewYorkZone() +
        "BEGIN:VEVENT\r\n"
        "SUMMARY:Line folded \r\n"
        " title\\nextra\r\n"
        "DTSTART;TZID=America/New_York:20260106T101000\r\n"
        "DURATION:PT45M\r\n"
        "RRULE:FREQ=WEEKLY;INTERVAL=2;BYDAY=TU;UNTIL=20260120T151000Z\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR\r\n";
    const auto items = di_ics::ExpandCalendar(ics, YearWindow2026());
    // Jan 6 is included (15:10 UTC == UNTIL). Jan 20 is 15:10 UTC which is not greater
    // than UNTIL, so it is included. Jan 13 is skipped by INTERVAL=2. Feb 3 is after UNTIL.
    Expect(FindUtc(items, {2026, 1, 6, 15, 10, 0}) != nullptr, "first interval week and folded summary");
    Expect(FindUtc(items, {2026, 1, 13, 15, 10, 0}) == nullptr, "INTERVAL=2 skips the middle Tuesday");
    Expect(FindUtc(items, {2026, 1, 20, 15, 10, 0}) != nullptr, "UNTIL is inclusive");
    Expect(FindUtc(items, {2026, 2, 3, 15, 10, 0}) == nullptr, "UNTIL cuts off later weeks");
    if (const auto* first = FindUtc(items, {2026, 1, 6, 15, 10, 0})) {
        Expect(first->summary == "Line folded title\nextra", "unfold keeps the real space and decodes \\n");
        Expect(first->endUtc - first->startUtc == 45 * 60, "DURATION is honoured");
    }
}

void TestMonthlyCountAndCancelledOverride() {
    const std::string ics =
        "BEGIN:VCALENDAR\r\n" +
        NewYorkZone() +
        "BEGIN:VEVENT\r\n"
        "SUMMARY:First Monday\r\n"
        "DTSTART;TZID=America/New_York:20260105T120000\r\n"
        "DTEND;TZID=America/New_York:20260105T130000\r\n"
        "RRULE:FREQ=MONTHLY;BYDAY=1MO;COUNT=3\r\n"
        "END:VEVENT\r\n"
        "BEGIN:VEVENT\r\n"
        "SUMMARY:Cancelled instance\r\n"
        "DTSTART;TZID=America/New_York:20260202T150000\r\n"
        "DTEND;TZID=America/New_York:20260202T160000\r\n"
        "RECURRENCE-ID;TZID=America/New_York:20260202T120000\r\n"
        "STATUS:CANCELLED\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR\r\n";
    const auto items = di_ics::ExpandCalendar(ics, YearWindow2026());
    int mondays = 0;
    bool sawCancelled = false;
    for (const auto& item : items) {
        if (item.summary == "First Monday") ++mondays;
        if (item.summary == "Cancelled instance") sawCancelled = true;
        const di_ics::Civil utc = di_ics::UnixUtcToUtcCivil(item.startUtc);
        if (utc.month == 2 && utc.day == 2) {
            Expect(false, "cancelled February instance should be gone");
        }
    }
    Expect(mondays == 2, "COUNT=3 with one cancelled override leaves two Mondays");
    Expect(!sawCancelled, "STATUS:CANCELLED override is not shown");
    // Jan 5 and Mar 2 remain. Noon EST is 17:00 UTC.
    Expect(FindUtc(items, {2026, 1, 5, 17, 0, 0}) != nullptr, "first Monday in January");
    // March 2 is before the second Sunday, so New York is still on EST (17:00 UTC).
    Expect(FindUtc(items, {2026, 3, 2, 17, 0, 0}) != nullptr, "first Monday in March is still EST");
}

di_ics::AgendaInput Timed(int y, int m, int d, int hh, int mm, int durMin, bool location = false) {
    di_ics::AgendaInput ev;
    ev.year = y;
    ev.month = m;
    ev.day = d;
    ev.hour = hh;
    ev.minute = mm;
    ev.spanYear = y;
    ev.spanMonth = m;
    ev.spanDay = d;
    const di_ics::Civil local{y, m, d, hh, mm, 0};
    // The test process is America/Los_Angeles, so local wall time is that zone.
    ev.startUtc = di_ics::CivilToUnixLocal(local);
    ev.endUtc = ev.startUtc + static_cast<int64_t>(durMin) * 60;
    const di_ics::Civil end = di_ics::UnixUtcToLocal(ev.endUtc);
    ev.spanEndYear = end.year;
    ev.spanEndMonth = end.month;
    ev.spanEndDay = end.day;
    if (ev.spanEndYear == y && ev.spanEndMonth == m && ev.spanEndDay == d) {
        const di_ics::Civil next = di_ics::AddDays(di_ics::Civil{y, m, d, 0, 0, 0}, 1);
        ev.spanEndYear = next.year;
        ev.spanEndMonth = next.month;
        ev.spanEndDay = next.day;
    }
    ev.hasLocation = location;
    return ev;
}

di_ics::AgendaInput AllDay(int y, int m, int d, int days = 1) {
    di_ics::AgendaInput ev;
    ev.allDay = true;
    ev.year = y;
    ev.month = m;
    ev.day = d;
    ev.spanYear = y;
    ev.spanMonth = m;
    ev.spanDay = d;
    const di_ics::Civil end = di_ics::AddDays(di_ics::Civil{y, m, d, 0, 0, 0}, days);
    ev.spanEndYear = end.year;
    ev.spanEndMonth = end.month;
    ev.spanEndDay = end.day;
    return ev;
}

const di_ics::AgendaLine* FindLine(const di_ics::AgendaLayout& layout, int index, bool header) {
    for (const auto& line : layout.lines) {
        if (line.dayHeader == header && line.eventIndex == index) return &line;
    }
    return nullptr;
}

void TestAgendaLayout() {
    // 2026-01-06 18:00 UTC is 10:00 PST. Today is Tuesday Jan 6.
    const int64_t now = di_ics::CivilToUnixUtc({2026, 1, 6, 18, 0, 0});
    const di_ics::Civil local = di_ics::UnixUtcToLocal(now);
    Expect(local.year == 2026 && local.month == 1 && local.day == 6 && local.hour == 10,
           "agenda anchor is 10:00 on Jan 6 in Los Angeles");

    // 7:10-8:25 already ended. 9:00-11:00 is in progress. 23:59 is a deadline.
    auto past = Timed(2026, 1, 6, 7, 10, 75);
    auto nowClass = Timed(2026, 1, 6, 9, 0, 120, true);
    auto due = Timed(2026, 1, 6, 23, 59, 0);
    auto tomorrow = AllDay(2026, 1, 7);
    auto thursday = Timed(2026, 1, 8, 14, 0, 50);
    auto tooFar = Timed(2026, 1, 13, 9, 0, 60);  // offset 7, outside a 7-day window
    std::vector<di_ics::AgendaInput> items = {past, nowClass, due, tomorrow, thursday, tooFar};

    const di_ics::AgendaLayout layout = di_ics::BuildAgenda(items, now, 7);
    Expect(layout.lines.size() == 8, "today header+3, tomorrow header+all-day, Thursday header+class");

    Expect(layout.lines[0].dayHeader && layout.lines[0].dayOffset == 0, "today header first");
    Expect(layout.lines[1].eventIndex == 0 && layout.lines[1].past &&
               layout.lines[1].role == di_ics::AgendaRole::Timed,
           "morning class is past");
    Expect(layout.lines[2].eventIndex == 1 && layout.lines[2].happening &&
               layout.lines[2].h == di_ics::AgendaMetrics::kRowHLoc,
           "9:00 class is happening and keeps a location line");
    Expect(layout.lines[3].eventIndex == 2 && layout.lines[3].role == di_ics::AgendaRole::Deadline &&
               !layout.lines[3].past,
           "23:59 zero-length event is a deadline");

    Expect(layout.lines[4].dayHeader && layout.lines[4].dayOffset == 1, "tomorrow header");
    Expect(layout.lines[5].role == di_ics::AgendaRole::AllDay && layout.lines[5].eventIndex == 3,
           "tomorrow all-day");
    Expect(layout.lines[6].dayHeader && layout.lines[6].day == 8 && layout.lines[6].dayOffset == 2,
           "empty Wednesday is skipped, Thursday is offset 2");
    Expect(FindLine(layout, 5, false) == nullptr, "Jan 13 is outside the 7-day window");

    // A normal 23:00 meeting is not a deadline. A 23:50 start is.
    auto evening = Timed(2026, 1, 6, 23, 0, 40);
    auto late = Timed(2026, 1, 6, 23, 50, 20);
    const di_ics::AgendaLayout nights = di_ics::BuildAgenda({evening, late}, now, 1);
    Expect(nights.lines[1].role == di_ics::AgendaRole::Timed, "23:00 with a duration stays a meeting");
    Expect(nights.lines[2].role == di_ics::AgendaRole::Deadline, "23:50 start is a deadline");

    const di_ics::AgendaLayout empty = di_ics::BuildAgenda({}, now, 7);
    Expect(empty.lines.size() == 2, "empty agenda still shows today");
    Expect(empty.lines[0].dayHeader && empty.lines[0].dayOffset == 0, "empty today header");
    Expect(empty.lines[1].role == di_ics::AgendaRole::Empty, "today says nothing scheduled");
    Expect(empty.contentHeight == di_ics::AgendaMetrics::kDayHeaderH + di_ics::AgendaMetrics::kRowH +
                                      di_ics::AgendaMetrics::kRowGap,
           "empty today height");

    const float rowTop = di_ics::AgendaMetrics::kDayHeaderH;
    Expect(layout.lines[1].y == rowTop, "first event sits under the day header");
    Expect(layout.lines[2].y == rowTop + di_ics::AgendaMetrics::kRowH + di_ics::AgendaMetrics::kRowGap,
           "location row follows a normal row");
}

void TestHeadsUpAndGarbage() {
    const std::string ics =
        "BEGIN:VCALENDAR\r\n" +
        NewYorkZone() +
        "BEGIN:VEVENT\r\n"
        "SUMMARY:CS 4789\r\n"
        "DTSTART;TZID=America/New_York:20260106T101000\r\n"
        "DTEND;TZID=America/New_York:20260106T112500\r\n"
        "END:VEVENT\r\n"
        "BEGIN:VEVENT\r\n"
        "SUMMARY:All day quiet\r\n"
        "DTSTART;VALUE=DATE:20260106\r\n"
        "END:VEVENT\r\n"
        "END:VCALENDAR\r\n";
    const auto items = di_ics::ExpandCalendar(ics, YearWindow2026());
    const int64_t start = di_ics::CivilToUnixUtc({2026, 1, 6, 15, 10, 0});
    Expect(di_ics::HeadsUpLabel(items, start - 12 * 60, 15) == "CS 4789 in 12 min",
           "heads-up 12 minutes out");
    Expect(di_ics::HeadsUpLabel(items, start - 30, 10) == "CS 4789 now", "heads-up at the start");
    Expect(di_ics::HeadsUpLabel(items, start - 20 * 60, 10).empty(), "outside the heads-up window");
    Expect(di_ics::HeadsUpLabel(items, start + 5 * 60, 10).empty(), "already started, no heads-up");
    Expect(di_ics::ExpandCalendar("this is not a calendar", YearWindow2026()).empty(), "garbage ICS is empty");
    Expect(di_ics::ExpandCalendar("", YearWindow2026()).empty(), "empty ICS is empty");
}

}  // namespace

int main() {
    setenv("TZ", "America/Los_Angeles", 1);
    tzset();

    TestWeekdayAnchor();
    TestWeeklyClassExdateAndOverride();
    TestAllDay();
    TestUtcEvent();
    TestTzidWithoutHardcoding();
    TestFoldingEscapesIntervalAndUntil();
    TestMonthlyCountAndCancelledOverride();
    TestHeadsUpAndGarbage();
    TestAgendaLayout();

    if (g_failed != 0) {
        std::cerr << g_failed << " assertion(s) failed\n";
        return 1;
    }
    std::cout << "ics parser tests passed\n";
    return 0;
}
