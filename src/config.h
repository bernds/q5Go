
/* Name of package */
#define PACKAGE "q5go"
#define PACKAGE1 "q5goClient"

/* Version number of package */
#define VERSION "1.0"

#define NEWVERSIONWARNING "<p>Welcome to version " VERSION " of q5go.</p>\n" \
  "<p>This version adds some tutorials for beginners.</p><p>Edit mode has been changed a little, and there is now support for undo/redo.</p>" \
  "<p>When started without any arguments, the program now displays a greeter dialog.</p>" \
  "<p>For a full list of changes, please refer to the README.</p>" \
  "<p>This message will not be shown anymore on startup.</p>"

#if __cplusplus >= 201703L
#define FALLTHRU [[fallthrough]];
#else
#define FALLTHRU /* fall through */
#endif
