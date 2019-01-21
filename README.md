## q5Go 0.3

This is a Go player's tool, an SGF editor/GTP interface/IGS client.  It is
based on the old Qt3 version of qGo, but ported to Qt5 and modernized.

The basic goal for this program is to provide an everyday SGF editor
that is fast, easy and convenient to use and does everything you could
want out of such a tool.  Some of the more unusual features include a
Go diagram exporting function for sites like lifein19x19.com or
Sensei's library, as well as SVG vector graphics export.

![screenshot](screens/screenshot.png)

Version 0.3 adds two major new features:
 * An analysis mode using Leela Zero or compatible programs
 * A game tree display

Minor changes and fixes include:
 * Some visual improvements for the user interface
 * Time warnings for online play are working again
 * When loading SGF files, unrecognized properties are now
   remembered and saved as per the specification
 * Faster loading of SGF files

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

### Export
q5Go allows the user to export board positions as ASCII diagrams suitable
for Sensei's Library or the lifein19x19.com forums, or in the SVG vector
graphics format which should be suitable for printing.  In both cases,
the user can select a sub-area of the board to be shown in the export,
and it is possible to set a position as the start of move numbering, so
that sequences of moves can be shown in the exported diagram.

![screenshot](screens/export.png)

### Configurable appearance

The look of the board and stones can be configured to suit the user's
personal taste.  The stones are generated in a shaded 3D look, and both
the shape and the lighting can be changed.

![screenshot](screens/gostones.jpg)

There are several presets for the wood image, and the user can also
supply a custom file.

![screenshot](screens/gostones2.jpg)

## Compiling

Make a build subdirectory, enter it, and either run
```sh
  ../configure
```
or
```sh
  qmake ../src/q5go.pro
```

followed by make.  The latter is probably recommended as the automake system
is somewhat cobbled-together.
