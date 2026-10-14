#pragma once

#include <util/Macros.h>

#include <rhatt/prelude.h>

#include <argon/argon.hpp>

#include <dbuf/ByteReader.hpp>
#include <dbuf/ByteWriter.hpp>

#include <ui/Include.h>

#include <util/Include.h>

#include <Geode/Geode.hpp>

namespace cw::ferry {
    namespace request {
        inline auto setBytes(geode::utils::web::WebRequest other, geode::ByteVector data) {
            return other
                .body(std::move(data));
        };
    };

    using namespace ui;
};