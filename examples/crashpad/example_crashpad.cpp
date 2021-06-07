#include <stddef.h>
#include <string.h>

#include <bun/bun.h>
#include <bun/stream.h>

#include <iostream>

#include <client/crash_report_database.h>
#include <client/settings.h>
#include <client/crashpad_client.h>
#include <client/crashpad_info.h>

#include <unistd.h>
#include <sys/syscall.h>

#ifndef SYS_gettid
#error "SYS_gettid unavailable on this system"
#endif

pid_t gettid();

char buffer[0x10000];

static bool
startCrashHandler()
{
    using namespace crashpad;

    std::map<std::string, std::string> annotations;
    std::vector<std::string> arguments;
    // CrashpadClient client;
    bool rc;

    /*
     * ENSURE THIS VALUE IS CORRECT.
     *
     * This is the directory you will use to store and
     * queue crash data.
     *
     * In the example we use the current directory.
     */
    std::string db_path(".");

    /*
     * ENSURE THIS VALUE IS CORRECT.
     *
     * Crashpad has the ability to support crashes both in-process
     * and out-of-process. The out-of-process handler is
     * significantly more robust than traditional in-process crash
     * handlers. This path may be relative.
     */
    std::string handler_path("/path/to/crashpad/handler");

    /*
     * YOU MUST CHANGE THIS VALUE.
     *
     * This should point to your server dump submission port
     * (labeled as "http/writer" in the listener configuration
     * pane. Preferably, the SSL enabled port should
     * be used. If Backtrace is hosting your instance, the default
     * port is 6098.
     */
    std::string url("https://your_host.sp.backtrace.io:6098/");

    /*
     * YOU MUST CHANGE THIS VALUE.
     *
     * Set this to the submission token under the project page.
     * Learn more at:
     * https://documentation.backtrace.io/coronerd_setup/#tokens
     */
    annotations["token"] = "your_token";

    /*
     * THE FOLLOWING ANNOTATIONS MUST BE SET.
     *
     * Backtrace supports many file formats. Set format to minidump
     * so it knows how to process the incoming dump.
     */
    annotations["format"] = "minidump";

    /*
     * REMOVE THIS FOR ACTUAL BUILD.
     *
     * We disable crashpad rate limiting for this example.
     */
    arguments.push_back("--no-rate-limit");

    base::FilePath db(db_path);
    base::FilePath handler(handler_path);

    std::unique_ptr<CrashReportDatabase> database =
        crashpad::CrashReportDatabase::Initialize(db);

    if (database == nullptr || database->GetSettings() == NULL)
        return false;

    /* Enable automated uploads. */
    database->GetSettings()->SetUploadsEnabled(true);

    return CrashpadClient::StartHandlerAtCrashForBacktrace(
        handler, db, db, url, annotations, arguments, {}
    );
}

void
IamDummy()
{
    std::cout << "dummy tid: " << gettid() << "\n";
    __builtin_trap();
}

int
main()
{
    std::vector<char> buffer(0x10000);
    /*
     * Create the handle using the default backend. One can use a specific
     * backend to force it, for example BUN_BACKEND_LIBUNWIND.
     */
    bun_handle_t *handle = bun_create(BUN_BACKEND_DEFAULT);

    std::cerr << startCrashHandler() << '\n';

    crashpad::CrashpadInfo::GetCrashpadInfo()
        ->AddUserDataMinidumpStream(BUN_STREAM_ID, buffer.data(), buffer.size());
    std::cerr << bun_sigaction_set(handle, buffer.data(), buffer.size()) << '\n';

    std::cout << "tid: " << gettid() << "\n";

    IamDummy();
    bun_destroy(handle);
}

pid_t
gettid()
{
    return ((pid_t)syscall(SYS_gettid));
}
