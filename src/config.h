
/* Name of package */
#define PACKAGE "q5go"
#define PACKAGE1 "q5goClient"

/* Version number of package */
#define VERSION "0.9"

#define NEWVERSIONWARNING "<p>Welcome to version " VERSION " of q5go.</p>\n" \
  "<p>This adds support for analysis using KataGo, a new AI that can estimate scores during the game.</p>" \
  "<p>Other changes include support for AI vs AI play, time controls when playing an AI, more Go variants, and a few bug fixes.</p>" \
  "<p>For a full list of changes, please refer to the README.</p>" \
  "<p>This message will not be shown anymore on startup.</p>"

#if __cplusplus >= 201703L
#define FALLTHRU [[fallthrough]];
#else
#define FALLTHRU /* fall through */
#endif
