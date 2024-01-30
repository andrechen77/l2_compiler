#pragma once

#include <string_view>
#include <charconv>
#include <set>

namespace utils {
	template<typename T>
    T string_view_to_int(const std::string_view &view) {
        T result;
        auto [ptr, ec] = std::from_chars(view.data(), view.data() + view.size(), result);
        // TODO check for error
        return result;
    }

    template<typename T>
    using set = std::set<T, std::less<void>>;
}