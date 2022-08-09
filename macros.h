#ifndef BARS_TOOL_MACROS_H
#define BARS_TOOL_MACROS_H

// this is a little hacky, but it works as intended because this gets tedious to write out
// typed read
#define tread(o, t) read(reinterpret_cast<char*>(&o), sizeof(t))
// quick typed read (inferred from output object)
#define qread(o) read(reinterpret_cast<char*>(&o), sizeof(o))

// swap string into a default constructed stringstream to immediately discard. which therefore clears it :D
#define ssclear(s) std::stringstream().swap(s)

// python inspired range syntax
#define range(n) std::ranges::views::iota(0, (int)n)

#endif //BARS_TOOL_MACROS_H
