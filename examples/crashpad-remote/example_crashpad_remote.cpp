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
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "bcd.h"

#ifndef SYS_gettid
#error "SYS_gettid unavailable on this system"
#endif

static pid_t gettid();
static bcd_t bcd;

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
    std::cerr << "dummy function thread id: " << gettid() << "\n";
    __builtin_trap();
}

/*
 * Allocate the memory for the buffer. It may be allocated on the stack, but
 * its lifetime must be long enough for signal handlers and the buffer
 * should be accessible from a signal handler.
 */
// static std::vector<char> buffer_backend(0x10000);
static struct bun_buffer buffer;

/*
 * Handle for use in FirstChahnceHandler.
 */
static struct bun_handle handle;

/*
 * Signal handler executed by CrashpadClient::SetFirstChanceExceptionHandler.
 */
bool FirstChanceHandler(int signum, siginfo_t *info, ucontext_t *context)
{
    (void) signum;
    (void) info;
    (void) context;

    // bun_unwind(&handle, &buffer);

    bcd_emit(&bcd, "1");

    return false;
}

/* Default size of buffer. */
#define BUFFER_SIZE	4096

/* This descriptor is shared between the child and parent. */
static int buffer_fd;

/* This is the pointer to the buffer used by the BCD child. */
static char *buffer_child;

static void *
buffer_reader(int fd, int flags)
{
	void *r;

	r = mmap(NULL, BUFFER_SIZE, flags, MAP_SHARED, fd, 0);
	if (r == MAP_FAILED)
		abort();

	return r;
}

static void *
buffer_create(void)
{
	void *r;
	int fd;

	fd = memfd_create("_backtrace_buffer", MFD_CLOEXEC);
	if (fd == -1)
		abort();

	if (ftruncate(fd, BUFFER_SIZE) == -1)
		abort();

	buffer_fd = fd;

	r = buffer_reader(buffer_fd, PROT_READ | PROT_WRITE);

	fprintf(stderr, "%ju: Returned pointer: %p (buffer_child = %p)\n",
	    (uintmax_t)getpid(), r, buffer_child);

	return r;
}

static int
request_handler(pid_t tid)
{
	time_t now = time(NULL);

	// /* We write data to the buffer. */
	fprintf(stderr, "A new requested generated at: %ju",
	    (uintmax_t)now);

    // bun_unwind_remote(&handle, &buffer, tid);
	/* We return -1 to tell BCD it only needs to execute this function. */
	return -1;
}

static void
monitor_init(void)
{
    write(2, "FOO\n", 4);
    FILE *fp = fopen("/tmp/poop", "a");
	fprintf(fp, "Child is %ju\n", (uintmax_t)getpid());
    fclose(fp);
	fprintf(stderr, "Child is %ju\n", (uintmax_t)getpid());

	if (prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0) == -1)
		perror("prctl");

	/*
	 * This is called after the parent process has set buffer_fd. Set
	 * a memory mapping to the same descriptor.
	 */
	buffer_child = (char *)buffer_reader(buffer_fd, PROT_READ | PROT_WRITE);
	if (buffer_child == NULL)
		abort();

	sprintf(buffer_child, "Empty contents.");
	fprintf(stderr, "%ju: Returned+pointer: %p (buffer_child = %p)\n",
	    (uintmax_t)getpid(), buffer_child, buffer_child);
	return;
}

int
main()
{
    struct bcd_config cf{};
	bcd_error_t e{};

    const char *buffer_{};

    bool bun_initialized = bun_handle_init(&handle, BUN_BACKEND_DEFAULT);
    if (bun_initialized) {
        std::cerr << "Successfully initialized libbun.\n";
    } else {
        std::cerr << "Failed to initialize libbun.\n";
        return EXIT_FAILURE;
    }

	/* Initialize a shared memory region. */
	buffer_ = (char *)buffer_create();
    std::cerr << "buffer: " << (void *)buffer_ << "\n";

    bool buffer_initialized = bun_buffer_init(&buffer, (void *)buffer_, BUFFER_SIZE);
    if (buffer_initialized) {
        std::cerr << "Successfully initialized buffer.\n";
    } else {
        std::cerr << "Failed to initialize buffer.\n";
        return EXIT_FAILURE;
    }
    // buffer.data = (char *)buffer_;
    // buffer.size = BUFFER_SIZE;

	/* Request handler to be called when processing errors by BCD worker. */
	cf.request_handler = request_handler;

	/* Set a function to be called by the child for setting permissions. */
	cf.monitor_init = monitor_init;

	/* Initialize the BCD configuration file. */
	if (bcd_config_init(&cf, &e) == -1) {
        std::cerr << "attach " << bcd_error_errno(&e) << ": " <<
            bcd_error_message(&e) << "\n";
        return EXIT_FAILURE;
    }

	if (bcd_init(&cf, &e) == -1) {
		char msg[128];
		sprintf(msg, "error: failed to init: %s (%s)\n",
			e.message, strerror(e.errnum));
        std::cerr << msg;
        return EXIT_FAILURE;
	}

	if (bcd_attach(&bcd, &e) == -1) {

        std::cerr << "attach " << bcd_error_errno(&e) << ": " <<
            bcd_error_message(&e) << "\n";
        perror("attach");
        return EXIT_FAILURE;
    }

    // std::cerr << "init " << bcd_error_errno(&e) << ": " <<
        // bcd_error_message(&e) << "\n";
    // return 0;
    /*
     * Create the handle using the default backend. One can use a specific
     * backend to force it, for example BUN_BACKEND_LIBUNWIND.
     */



    /*
     * Start the Crashpad handler, the code for this function has been taken
     * and adapted from Backtrace's documentation.
     */
    bool crashpad_initialized = startCrashHandler();
    if (crashpad_initialized) {
        std::cerr << "Successfully initialized crashpad.\n";
    } else {
        std::cerr << "Failed to initialize crashpad.\n";
        return EXIT_FAILURE;
    }

    /*
     * Set the Minidump data stream to our buffer.
     */
    crashpad::CrashpadInfo::GetCrashpadInfo()->AddUserDataMinidumpStream(
        BUN_STREAM_ID, buffer_, BUFFER_SIZE);

    /*
     * Set signal/exception handler for the libbun stream.
     */
    crashpad::CrashpadClient::SetFirstChanceExceptionHandler(FirstChanceHandler);



    std::cerr << "main function thread id: " << gettid() << "\n";

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
