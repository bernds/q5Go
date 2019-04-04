## q5Go 0.8.1

This is a tool for Go players which performs the following functions:
- SGF editor
- Analysis frontend for Leela Zero (or compatible engines)
- GTP interface
- IGS client
- Export to a variety of formats

The basic goal for this program is to provide an everyday SGF editor
that is fast, easy and convenient to use and does everything you could
want out of such a tool.  Some of the more unusual features include a
Go diagram exporting function for sites like lifein19x19.com or
Sensei's library, as well as SVG vector graphics or slide export.
q5go also supports some non-standard Go variants.

This program is based on the old Qt3 version of qGo, but ported to Qt5
and modernized.

![screenshot](screens/screenshot.png)

Version 0.8.1 is just a compatibility fix for the Leela Zero "next" branch
that will become 0.17.  These are the major user-visible changes in 0.8:
 * Analyzing games with multiple engines now produces sensible results.
   q5go remembers winrates for each engine name and komi combination and
   displays multiple winrate graphs.
 * For engines with fixed komi, batch analysis can now flip the position
   to effectively analyze with reverse komi.
 * For live analysis, it is now possible to choose the analysis engine.
 * There is a new slide export feature which combines the board and
   comments into a single image for use in slide shows or videos.
   File name sequences can be generated automatically.
 * It is now possible to choose between area and territory scoring in
   off-line games.

Minor fixes and improvements include:
 * The board coordinates now scales with the size of the display, their
   size relative to the stones is configurable in the preferences.
 * It is now possible to choose whether to tile or scale the wood
   background image used for the board.
 * The evaluation graph is automatically displayed when a game obtains
   an evaluation for the first time.
 * In online match games, remaining time is now saved to the SGF for both
   player and opponent.
 * It is now possible to hold down LMB and drag in the evaluation graph
   to move to a different position.
 * In the database dialog, "Open" and "Cancel" now work (previously
   only double clicks opened the game).
 * In off-line mode, scoring a scored position again now keeps liveness
   and seki status.
 * A bug was fixed that caused the "Update" button to disappear when
   scoring or editing an off-line copy of an observed game.
 * Loading an SGF file for a game against an engine is now implemented.

See VERSION_HISTORY for a history of changes.

## Overview of features

### Analysis mode
q5Go supports not only play against AI engines, but can also connect to
Leela Zero to use it as an an analysis tool, displaying statistics such
as win-rates and visit counts, and displaying variations.  This is
available both for local SGF editing, and for observing on-line games.
By middle-clicking or shift-clicking on a displayed variation, it can
be added to the game record.

![screenshot](screens/analysis.png)

There is also a batch analysis mode, where the user can queue a number of
SGF files for analysis. Evaluations and variations are added automatically,
the variations are presented as diagrams.  The file can be observed
during analysis.

![screenshot](screens/batch.png)
![screenshot](screens/new-analysis.png)

It is possible to run analysis with multiple engines, or with different
komi values.  q5go keeps track of these separately and displays multiple
evaluation graphs.

![screenshot](screens/multieval.png)

### Export
q5Go allows the user to export board positions as ASCII diagrams suitable
for Sensei's Library or the lifein19x19.com forums, or in the SVG vector
graphics format which should be suitable for printing.  In both cases,
the user can select a sub-area of the board to be shown in the export,
and it is possible to set a position as the start of move numbering, so
that sequences of moves can be shown in the exported diagram.

![screenshot](screens/export.png)

Another option is slide export; this produces images of a fixed size
containing both the board and the comments.  The user can specify a
filename template, saving diagrams can then be automated: at each step,
the pattern "%n" in the template is replaced with an incrementing
diagram number.

![screenshot](screens/slideexport.png)

### SGF diagrams

It is possible to set a game position as the start of a diagram.  This is
has several use cases:
- Subdividing a game record into printable diagrams.  Figures can be
  exported to ASCII and SVG, in whole or in part, just like regular
  board positions.
- Allowing variations to be shown in a separate board display
- Neatly organizing engine lines when performing an analysis

### Configurable appearance

The look of the board and stones can be configured to suit the user's
personal taste.  The stones are generated in a shaded 3D look, and both
the shape and the lighting can be changed.

![screenshot](screens/gostones.jpg)

There are several presets for the wood image, and the user can also
supply a custom file.

![screenshot](screens/gostones2.jpg)

### Configurable user interface layout

All optional elements of the board window reside in freely moveable and
resizable docks, giving the user flexibility to create exactly the layout
they want.  These layouts can be saved and restored, and the program
tries to restore the correct layout whenever a new window is opened.

![screenshot](screens/docks.png)

### Database support

q5go can access a Kombilo database and search it by player name or
game event or date. This functions as an alternative file open dialog
with preview functionality.

![screenshot](screens/database.png)

### Go variants

q5go supports rectangular and toroidal boards.  Note that the latter
can only be saved in a non-standard SGF format since the specification
does not allow for it.  When playing on a torus, q5go can be configured
to extend the board past its regular dimensions, duplicating parts of
the position for a better overview.  Also, the board can be dragged
with the middle mouse button.

![screenshot](screens/variants.jpg)

The screenshot shows the variant game dialog and a (different) position
with both axes set to be toroidal.

## Compiling

On Linux, make a build subdirectory, enter it, and run
```sh
  qmake ../src/q5go.pro PREFIX=/where/you/want/to/install
```
followed by make and make install.  If the pandoc tool is installed, this
README.md file will be converted to html and installed, and can then be
viewed through a menu option.

On Windows, download the Qt tools and import the q5go.pro project file
into Qt Creator.
