#pragma once

#include <argon/argon.hpp>

#include <dbuf/ByteReader.hpp>
#include <dbuf/ByteWriter.hpp>

#include <ui/Include.h>

#include <util/Macros.h>
#include <util/Include.h>

#include <Geode/Geode.hpp>

namespace cw::ferry {
    namespace request {
        inline auto base() {
            auto loader = geode::Loader::get();

            return geode::utils::web::WebRequest()
                .userAgent(fmt::format("Ferry/{} ({}, Geode {}, GD {})",
                    geode::Mod::get()->getVersion().toVString(false),
                    geode::utils::platform::getString(),
                    loader->getVersion(),
                    loader->getGameVersion()))
                .timeout(std::chrono::seconds(15));
        };

        inline auto withAuth(int accountId, std::string token) {
            return geode::utils::web::WebRequest()
                .param("account_id", accountId)
                .param("authtoken", std::move(token));
        };

        inline auto setBytes(geode::utils::web::WebRequest other, geode::ByteVector data) {
            return other
                .body(std::move(data));
        };
    };

    using namespace ui;
};