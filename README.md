## q5Go 0.1

This is a Go player's tool, an SGF editor/GTP interface/IGS client.  It is
based on the old Qt3 version of qGo, but ported to Qt5 and modernized.

The basic goal for this program is to provide an everyday SGF editor that
is fast, easy and convenient to use and does everything you could want out
of such a tool.

In this version, a lot of old qGo code that was in really poor shape was
replaced entirely.  While this fixes a large number of issues, it also
means that the code should be considered beta quality, since a lot of it
is very new.  In particular it is not yet recommended to use the built-in
IGS client for playing games, unless you want to help with testing.
Observing should work fine.

There are some changes in the feature list in this version:
 * Added a GTP implementation that works with Leela Zero.  GTP support
   was also improved to be more asynchronous, so as to not lock up the GUI
   while waiting for the engine.
   This is also still slightly experimental.
 * Edit mode behaves differently.  There is now a toolbar to place
   marks at all times. and a button to change the player to move.
   Edit mode is available through a toggle button rather than as a side
   tab, and is only used for adding/removing stones.  New board positions
   are added as a child when leaving edit mode.
 * Some less useful features have been removed when it was easier than
   porting them Qt5 and the replaced foundation.  This includes the user
   toolbar, cutting and pasting variations, "mouse gestures" and
   configurable background colors.
 * Import and export have changed somewhat.  It is now possible to select
   a rectangular sub-area for ASCII and picture export.
   ASCII export now generates diagrams usable in the lifein19x19 forums
   and Sensei's library, including move numbers, with the option of creating
   multiple diagrams for a sequence of moves automatically.  As a
   consequence, the character set used for ASCII display is no longer
   configurable.  ASCII import was removed on the grounds of not
   being very useful.
 * There is a new SVG export to create a vector graphics representation of
   the board.  This also supports exporting subparts of the board.
 * The functionality for saving window sizes was simplified to just always
   save comment orientation.  Size and layout are now remembered separately
   for every encountered screen dimension.
   The functionality to save up to 10 different board sizes with
   keyboard shortcuts was removed.
 * Variation display was improved and the user can chooser between letters
   and ghost stones, as well as choosing whether to use the setting found
   in SGF files or ignore it.

This update was done by Bernd Schmidt <bernds_cb1@t-online.de>.

See VERSION_HISTORY for a history of changes.

## Compiling

Make a build subdirectory, enter it, and either run
  ../configure
or
  qmake ../src/q5go.pro

followed by make.  The latter is probably recommended as the automake system is somewhat
cobbled-together.