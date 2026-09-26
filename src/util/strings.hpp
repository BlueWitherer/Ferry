#pragma once

#include "Macros.h"

#include <Geode/utils/string.hpp>

namespace cw::ferry {
    namespace str = geode::utils::string;

    namespace url {
        static constexpr auto apiBase = CW_FERRY_WEB_BASEURL;
    };

    inline std::string apiEndpoint(std::string_view path) {
        return fmt::format("{}{}", url::apiBase, path);
    };

    inline std::string operator""_api(const char* str, size_t len) {
        return apiEndpoint(std::string_view{str, len});
    };
};