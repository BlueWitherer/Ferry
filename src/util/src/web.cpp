#include "../web.hpp"

#include <Util.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::ferry;

web::WebRequest request::withAuth(std::string token) {
    auto acc = argon::getGameAccountData();

    return cw::request::base()
        .param("account_id", acc.accountId)
        .param("authtoken", std::move(token))
        .param("user_id", acc.userId)
        .param("username", std::move(acc.username));
}

arc::Future<web::WebRequest> request::withAuthCo(std::string token) {
    co_return *co_await async::waitForMainThread<web::WebRequest>([t = std::move(token)]() {
        return withAuth(std::move(t));
    });
}