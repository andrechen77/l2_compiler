#include <string_view>
#include <charconv>

namespace utils {
	template <typename T>
    T string_view_to_int(const std::string_view &view) {
        T result;
        auto [ptr, ec] = std::from_chars(view.data(), view.data() + view.size(), result);
        // TODO check for error
        return result;
    }
}