#pragma once

namespace cw::ferry {
    namespace base {
        template <class T>
        class Singleton {
        protected:
            Singleton() = default;
            ~Singleton() = default;

            Singleton(const Singleton&) = delete;
            Singleton& operator=(const Singleton&) = delete;

            Singleton(Singleton&&) = delete;
            Singleton& operator=(Singleton&&) = delete;

        public:
            static T* get() noexcept {
                static T inst;
                return &inst;
            };
        };
    };
};