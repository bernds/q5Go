
/* Name of package */
#define PACKAGE "q5go"
#define PACKAGE1 "q5goClient"

/* Version number of package */
#define VERSION_1 "2.1.1"

#ifdef GITHUB_CI_BUILD
#define VERSION VERSION_1 " (GitHub action CI build on " __DATE__ ")"
#define CIBUILD "<p>Please note that this is not an official release build, but a GitHub CI build artifact.</p>"
#else
#define CIBUILD
#define VERSION VERSION_1
#endif

#define NEWVERSIONWARNING "<p>Welcome to version " VERSION " of q5go.</p>\n" \
  CIBUILD \
  "<p>This version is primarily a minor bugfix release.</p>" \
  "<p>It also has small enhancements, like copy/paste position in edit mode, and a new button to add an entire directory to the batch analysis.</p>" \
  "<p>For a full list of changes, please refer to the README.</p>" \
  "<p>This message will not be shown anymore on startup.</p>"

#if __cplusplus >= 201703L
#define FALLTHRU [[fallthrough]];
#else
#define FALLTHRU /* fall through */
#endif
