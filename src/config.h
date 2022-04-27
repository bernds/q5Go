
/* Name of package */
#define PACKAGE "q5go"
#define PACKAGE1 "q5goClient"

/* Version number of package */
#define VERSION_1 "2.1.3"

#ifdef GITHUB_CI_BUILD
#define VERSION VERSION_1 " (GitHub action CI build on " __DATE__ ")"
#define CIBUILD "<p>Please note that this is not an official release build, but a GitHub CI build artifact.</p>"
#else
#define CIBUILD
#define VERSION VERSION_1
#endif

#define NEWVERSIONWARNING "<p>Welcome to version " VERSION " of q5go.</p>\n" \
  CIBUILD \
  "<p>This version is a minor update.</p>" \
  "<p>It adds an autoplay feature for viewing game records and slightly extends support for KataGo's ruleset feature.</p>" \
  "<p>This message will not be shown anymore on startup.</p>"

#if __cplusplus >= 201703L
#define FALLTHRU [[fallthrough]];
#else
#define FALLTHRU /* fall through */
#endif
