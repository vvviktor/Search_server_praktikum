#pragma once

#include <vector>
#include <string>
#include <set>
#include <string_view>
#include <algorithm>

std::vector<std::string> SplitIntoWords(const std::string_view text);

std::vector<std::string_view> SplitIntoWordsView(std::string_view str);

template<typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (const auto& str: strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}
