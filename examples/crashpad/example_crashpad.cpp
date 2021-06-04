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

    std::cerr << __LINE__ << '\n';

    if (database == nullptr || database->GetSettings() == NULL)
        return false;

    std::cerr << __LINE__ << '\n';

    /* Enable automated uploads. */
    database->GetSettings()->SetUploadsEnabled(true);

    return CrashpadClient::StartHandlerAtCrashForBacktrace(
        handler, db, db, url, annotations, arguments, {}
    );
}

bun_handle_t *handle;
static void my_sighandler(int)
{
    void *buf;
    size_t buf_size;
    bun_unwind(handle, buffer, sizeof(buffer));
    fprintf(stderr, "%p %lu\n", buf, buf_size);
}

void
IamDummy()
{
    std::cout << "dummy tid: " << gettid() << "\n";
    memset((void*)(intptr_t)4, 123, 1);
}

int
main()
{
    bun_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.unwind_backend = BUN_BACKEND_LIBUNWIND;
    handle = bun_create(&cfg);

    std::cerr << startCrashHandler() << '\n';

    crashpad::CrashpadInfo::GetCrashpadInfo()
        ->AddUserDataMinidumpStream(BUN_STREAM_ID, buffer, sizeof(buffer));
    std::cerr << bun_register_signal_handers(handle, &my_sighandler) << '\n';

    std::cout << "tid: " << gettid() << "\n";



    IamDummy();
    bun_destroy(handle);
}

pid_t
gettid()
{
    return ((pid_t)syscall(SYS_gettid));
}
