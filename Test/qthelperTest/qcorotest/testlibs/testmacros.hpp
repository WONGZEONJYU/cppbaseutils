#ifndef XUTILS2_TEST_MACROS_HPP
#define XUTILS2_TEST_MACROS_HPP 1

#pragma once

#define QCORO_DELAY(expr) QTimer::singleShot(10ms, [&] { expr; })

#define QCORO_TEST_TIMEOUT(expr) { \
    auto const start { std::chrono::steady_clock::now()}; \
    bool const ok { expr }; \
    auto const end { std::chrono::steady_clock::now() }; \
    QCORO_VERIFY(!ok); \
    QCORO_VERIFY((end - start) < 500ms); \
}

#endif
