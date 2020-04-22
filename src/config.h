
/* Name of package */
#define PACKAGE "q5go"
#define PACKAGE1 "q5goClient"

/* Version number of package */
#define VERSION "1.1"

#define NEWVERSIONWARNING "<p>Welcome to version " VERSION " of q5go.</p>\n" \
  "<p>This version mostly contains tweaks and improvements to existing features.</p>" \
  "<p>Working with very large SGF files should be faster, and there are some additional options for time warnings in online games.</p>" \
  "<p>For a full list of changes, please refer to the README.</p>" \
  "<p>This message will not be shown anymore on startup.</p>"

#if __cplusplus >= 201703L
#define FALLTHRU [[fallthrough]];
#else
#define FALLTHRU /* fall through */
#endif
