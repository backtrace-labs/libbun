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

static pid_t gettid();

static bool
startCrashHandler()
{
    using namespace crashpad;

    std::map<std::string, std::string> annotations;
    std::vector<std::string> arguments;
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

/*
 * The dummy function we want to show up in our callstack. Its only purpose is
 * to print the thread id and simulate a crash.
 */
void
IamDummy()
{
    std::cout << "dummy function thread id: " << gettid() << "\n";
    __builtin_trap();
}

/*
 * Allocate the memory for the buffer. It may be allocated on the stack, but
 * its lifetime must be long enough for signal handlers and the buffer
 * should be accessible from a signal handler.
 */
static std::vector<char> buffer_backend(0x10000);
struct bun_buffer buffer;

/*
 * Handle for use in FirstChahnceHandler.
 */
static struct bun_handle *global_handle;

/*
 * Signal handler executed by CrashpadClient::SetFirstChanceExceptionHandler.
 */
bool FirstChanceHandler(int signum, siginfo_t *info, ucontext_t *context)
{
    (void) signum;
    (void) info;
    (void) context;

    bun_unwind(global_handle, &buffer);

    return false;
}

int
main()
{
    /*
     * Create the handle using the default backend. One can use a specific
     * backend to force it, for example BUN_BACKEND_LIBUNWIND.
     */
    struct bun_handle handle;
    bool bun_initialized = bun_handle_init(&handle, BUN_BACKEND_DEFAULT);
    if (bun_initialized) {
        std::cout << "Successfully initialized libbun.\n";
        global_handle = &handle;
    } else {
        std::cout << "Failed to initialize libbun.\n";
        return EXIT_FAILURE;
    }

    /*
     * Start the Crashpad handler, the code for this function has been taken
     * and adapted from Backtrace's documentation.
     */
    bool crashpad_initialized = startCrashHandler();
    if (crashpad_initialized) {
        std::cout << "Successfully initialized crashpad.\n";
    } else {
        std::cout << "Failed to initialize crashpad.\n";
        return EXIT_FAILURE;
    }

    bool buffer_initialized = bun_buffer_init(&buffer, buffer_backend.data(),
        buffer_backend.size());
    if (buffer_initialized) {
        std::cout << "Successfully initialized buffer.\n";
    } else {
        std::cout << "Failed to initialize buffer.\n";
        return EXIT_FAILURE;
    }
    /*
     * Set the Minidump data stream to our buffer.
     */
    crashpad::CrashpadInfo::GetCrashpadInfo()->AddUserDataMinidumpStream(
        BUN_STREAM_ID, buffer_backend.data(), buffer_backend.size());

    /*
     * Set signal/exception handler for the libbun stream.
     */
    crashpad::CrashpadClient::SetFirstChanceExceptionHandler(FirstChanceHandler);

    std::cout << "main function thread id: " << gettid() << "\n";

    /*
     * Call the crashing function.
     */
    IamDummy();

    /*
     * Properly destroy the handle.
     *
     * In this example, this code will never be reached and is here only for
     * informational purposes.
     */
    bun_handle_deinit(&handle);

    return EXIT_SUCCESS;
}

static pid_t
gettid()
{

    return ((pid_t)syscall(SYS_gettid));
}
