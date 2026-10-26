#pragma once

#include "Macros.h"

#include <dankmeme.argon/argon.hpp>

#include <Geode/utils/web.hpp>

namespace cw::ferry {
    namespace request {
        geode::utils::web::WebRequest withAuth(std::string token);
        arc::Future<geode::utils::web::WebRequest> withAuthCo(std::string token);
    };
};