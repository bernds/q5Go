
/* Name of package */
#define PACKAGE "q5go"
#define PACKAGE1 "q5goClient"

/* Version number of package */
#define VERSION_1 "2.1.2"

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
  "<p>It fixes a problem with saving SGF files when the locale is set to one that uses commas instead of decimal points.</p>" \
  "<p>This message will not be shown anymore on startup.</p>"

#if __cplusplus >= 201703L
#define FALLTHRU [[fallthrough]];
#else
#define FALLTHRU /* fall through */
#endif
