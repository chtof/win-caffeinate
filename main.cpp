
// win-caffeinate
//
// Copyright iorate 2017-2018.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Require Windows 7 or later.
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#include <algorithm>
#include <iostream>
#include <regex>
#include <string>
#include <utility>
#include <vector>
#include <nonsugar.hpp>
#include <windows.h>

namespace {

struct scope_exit
{
    template <class F>
    struct impl
    {
        impl(impl const &) = delete;
        impl &operator=(impl const &) = delete;

        ~impl() { m_f(); }

        F m_f;
    };

    template <class F>
    impl<F> operator^(F &&f) const
    {
        return { std::move(f) };
    }
};

} // unnamed namespace

#define PP_CAT_I(i, j) i ## j
#define PP_CAT(i, j) PP_CAT_I(i, j)
#define SCOPE_EXIT auto const &PP_CAT(scope_exit_, __LINE__) = ::scope_exit() ^ [&]

int main(int argc, char **argv)
try {
    // Parse the command line.
    auto const cmd = nonsugar::command<char>("caffeinate", "prevent the system from sleeping")
        .flag<'h'>({'h'}, {"help"}, "", "display this help and exit")
        .flag<'v'>({'v'}, {"version"}, "", "display the version info and exit")
        .flag<'d'>({'d'}, {"display"}, "", "prevent the display from sleeping")
        .flag<'i'>({'i'}, {"system-idle"}, "", "prevent the system from idle sleeping (default)")
        .flag<'t', int>({'t'}, {"timeout"}, "PERIOD", "specify the timeout value in seconds")
        .flag<'w', int>({'w'}, {"wait"}, "PID", "wait for the process with the specified pid to\nexit")
        .argument<'U', std::vector<std::string>>("UTILITY")
        ;
    auto const opts = nonsugar::parse(argc, argv, cmd);
    if (opts.has<'h'>()) {
        std::cout << nonsugar::usage(cmd);
        return 0;
    }
    if (opts.has<'v'>()) {
        std::cout << "caffeinate 1.2\n";
        return 0;
    }

    // Create a power request.
    REASON_CONTEXT rc = { POWER_REQUEST_CONTEXT_VERSION, POWER_REQUEST_CONTEXT_SIMPLE_STRING };
    wchar_t wcs[] = L"caffeinate";
    rc.Reason.SimpleReasonString = wcs;
    auto const request = PowerCreateRequest(&rc);
    if (request == INVALID_HANDLE_VALUE) throw 0;
    SCOPE_EXIT { CloseHandle(request); };

    // Prevent the display from sleeping.
    auto const disp = opts.has<'d'>();
    if (disp && PowerSetRequest(request, PowerRequestDisplayRequired) == 0) throw 0;
    SCOPE_EXIT { if (disp) PowerClearRequest(request, PowerRequestDisplayRequired); };

    // Prevent the system from idle sleeping.
    auto const sys = !opts.has<'d'>() || opts.has<'i'>();
    if (sys && PowerSetRequest(request, PowerRequestSystemRequired) == 0) throw 0;
    SCOPE_EXIT { if (sys) PowerClearRequest(request, PowerRequestSystemRequired); };

    auto const args = opts.get<'U'>();
    if (!args.empty()) {
        // Create a process.
        std::string util;
        for (auto const &s : args) {
            util += '"' + std::regex_replace(s, std::regex(R"_((\\*)")_"), R"($1$1\\")") + "\" ";
        }
        util.back() = '\0';
        STARTUPINFO si = { sizeof(STARTUPINFO) };
        PROCESS_INFORMATION pi;
        auto const cp = CreateProcess(
            nullptr, &util[0], nullptr, nullptr, 0, 0, nullptr, nullptr, &si, &pi);
        if (cp == 0) {
            // Get the last error.
            char *buf;
            auto const fm = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr,
                GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                reinterpret_cast<char *>(&buf), 0, nullptr);
            if (fm == 0) throw 0;
            SCOPE_EXIT { LocalFree(buf); };
            std::cerr << "caffeinate: " << buf;
            return 0;
        }
        SCOPE_EXIT { CloseHandle(pi.hProcess); };
        CloseHandle(pi.hThread);

        // Wait for the process.
        if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_FAILED) throw 0;
        return 0;
    }

    auto const timeout = opts.has<'t'>() ?
        static_cast<DWORD>((std::max)(opts.get<'t'>(), 0)) * 1000 :
        INFINITE;
    if (opts.has<'w'>()) {
        // Open the process.
        auto const process = OpenProcess(SYNCHRONIZE, 0, static_cast<DWORD>(opts.get<'w'>()));
        if (process == nullptr) return 0;
        SCOPE_EXIT { CloseHandle(process); };

        // Wait for the process.
        if (WaitForSingleObject(process, timeout) == WAIT_FAILED) throw 0;
        return 0;
    }

    // Sleep for the specified timeout, otherwise forever.
    Sleep(timeout);
} catch (nonsugar::error const &e) {
    std::cerr << e.message() << '\n';
    return 1;
} catch (...) {
    std::cerr << "caffeinate: unexpected error occurred\n";
    return 1;
}
