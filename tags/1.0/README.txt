maze3dflyer

A graphics demo that generates and displays random 3D mazes
and lets them be navigated by keyboard control.
In the future, a screensaver with autopilot is planned.


To install from a zip archive:

Extract the contents (executable and Data folder) somewhere so that the maze3dflyer.exe and the Data folder are siblings.


Use:

Run the executable (maze3dflyer.exe) from the folder that is the parent of Data.


Goal:

Find your way through the maze from the entrance (green ring) to the exit (red).


Controls:

Esc: exit
H: toggle display of help text
WASD: move
Arrow keys: turn
Mouse: steer (if mouse grab is on)
Home/End: go to maze entrance/exit

Shift: toggle higher speed
M or mouse-click: toggle mouse grab*
T: toggle display of framerate
L: toggle display of best score list (arrow shows current maze config)
U: toggle status bar display
G: toggle maze outline*
F1: toggle full-screen mode

Space: stop and snap camera position/orientation to grid
F7: toggle "developer" mode*
N: generate new maze of same dimensions
C: toggle collision checking (allow passing through walls or not)*
+: 'solve' current level instantly
K: sKip current level (fast)
???: show path from entrance to exit (not yet implemented)
P: toggle autopilot (not yet implemented)*


*The state of these settings is reflected in a status bar
along bottom of window.


High scores:

Maze3dflyer keeps "high scores" (best times) for each maze configuration. A maze configuration consists
of the maze size in three dimensions, together with the sparsity (see below). For example, the configuration
for a 3x3x4 maze with sparsity 2 is expressed as 3x3x4/2, and the best score for that configuration would
be listed as "3x3x4/2 .... 0:03.24", indicating a fastest time of 3.24 seconds.
Currently, no information about *who* achieved a particular score is recorded.

Scores are saved to a file called "maze3dflyer_scores.txt" in the current directory where maze3dflyer is run.
If you want to share a scores file among multiple players, arrange for maze3dflyer.exe to run in the same folder.
However there is no protection against simultaneous players overwriting the score file and clobbering each other's scores,
for example if a network share is used.

The scores file is not installed with the program; it does not exist until maze3dflyer has been run and a new time
has been set (i.e. the first time a player solves a maze). Before the scores file exists, maze3dflyer may put
out a debugging message saying the file cannot be found. This message can be ignored.


Command line options:

WxHxD: maze dimensions, in cells. Default: 8x8x8. Max: 20x20x20.
WxHxD/s: maze dimensions and sparsity. Higher sparsity gives more space between passages. Default: 3.
-r: random size and sparsity for maze
-b #: branch clustering. Higher numbers, up to 6, cause branching to occur more evenly throughout the maze. Default: 2.
-f: full-screen mode
-h: do not show help text initially
Example: maze3dflyer -h 5x5x12/2 -f

(c) 2008 by Lars Huttar
email: huttar dot net, username is lars
http://www.huttar.net/lars-kathy/home.html

Acknowledgements:
OpenGL explanations and examples:
http://nehe.gamedev.net/data/lessons/lesson.asp?lesson=10 for the basis of constructing a virtual world and moving around
http://nehe.gamedev.net/data/lessons/lesson.asp?lesson=Quaternion_Camera_Class for composed 3D camera rotation
http://www.morrowland.com/apron/tutorials/gl/gl_jpeg_loader.zip for JPEG loader
Textures (some only historically used):
http://hazelwhorley.com/textures.html (Sahara skybox)
http://www.3dmd.net/
http://www.stormvisions.com/3DGS/roof1.jpg
http://www.sharecg.com/
Fonts:
Gentium (http://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&item_id=Gentium)
Helvetica
