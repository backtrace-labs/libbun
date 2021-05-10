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

#define gettid() ((pid_t)syscall(SYS_gettid))

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
    std::string handler_path("/home/pzakrzewski/crashpad/repo/crashpad/out/Default/crashpad_handler");

    /*
     * YOU MUST CHANGE THIS VALUE.
     *
     * This should point to your server dump submission port
     * (labeled as "http/writer" in the listener configuration
     * pane. Preferably, the SSL enabled port should
     * be used. If Backtrace is hosting your instance, the default
     * port is 6098.
     */
    std::string url("http://yolo.sp.backtrace.io:6097/");

    /*
     * YOU MUST CHANGE THIS VALUE.
     *
     * Set this to the submission token under the project page.
     * Learn more at:
     * https://documentation.backtrace.io/coronerd_setup/#tokens
     */
    annotations["token"] = "c5a3a1675dbcfa1d12fd7efbc7bef9350874654e55ff86478115c8bc073f54dc";

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

bun_t *handle;
static void my_sighandler(int)
{
    std::cout << "handler tid: " << gettid() << "\n";
    void *buf;
    size_t buf_size;
    bun_unwind(handle, &buf, &buf_size);
    fprintf(stderr, "%p %lu\n", buf, buf_size);
    crashpad::CrashpadInfo::GetCrashpadInfo()
        ->AddUserDataMinidumpStream(BUN_STREAM_ID, buf, buf_size);
}

void IamDummy()
{
    std::cout << "dummy tid: " << gettid() << "\n";
    memset((void*)(intptr_t)4, 123, 1);
    // throw 123;
}

int main()
{
    std::cout << "lolol" << std::endl;
    bun_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.unwind_backend = BUN_LIBUNWIND;
    cfg.buffer_size = 65536;
    cfg.buffer = calloc(1, cfg.buffer_size);
    handle = bun_create(&cfg);

    std::cerr << startCrashHandler() << '\n';
    std::cerr << bun_register_signal_handers(handle, &my_sighandler) << '\n';

    std::cout << "tid: " << gettid() << "\n";

    char buf[] = "MAIN TEST TEST TEST TEST";
    crashpad::CrashpadInfo::GetCrashpadInfo()
        ->AddUserDataMinidumpStream(55557, buf, sizeof(buf));
    IamDummy();
    bun_destroy(handle);
}
