# maze3dflyer

An OpenGL graphics demo/game. It generates a random 3D maze with some configurable properties, and displays the maze using textured 3D graphics.
You can "fly" through and around the maze using standard movement controls.

## News
* 2015/03/20: moved from Google Code to GitHub
* 2009/10/30: released v1.5.4 (minor feature release)
  * added pictures on walls (just Mona Lisa for now)
* 2009/10/29: released v1.5.3 (minor feature release)
  * fixed issue #4, problem with going forward and sideways at the same time
* 2009/10/23: released v1.5.2 (minor feature release)
  * added keys to adjust key/mouse sensitivity
* 2009/10/23: released v1.5.1 (bug fix release)
  * Full screen mode toggle now works (textures reload).
* 2009/06/12: released v1.5
  * Show maze as it's being generated (fun to watch). Blue = the queue of cells from which the maze is to expand. Red = cells that are marked "forbidden" for growth because they are too close to other passages.
  * Rather than just finding the route from entrance to exit, you now need to collect all the white balls on the way. More challenging.
  * See-through grates give some visibility in/out of maze.
  * Many other tweaks.
  * Let me know what you think!
[http://maze3dflyer.googlecode.com/svn/trunk/screenshots/maze3d-1.5-animGen.jpg http://maze3dflyer.googlecode.com/svn/trunk/screenshots/maze3d-1.5-animGen-th.jpg] [http://maze3dflyer.googlecode.com/svn/trunk/screenshots/maze3d-1.5-prizes.jpg http://maze3dflyer.googlecode.com/svn/trunk/screenshots/maze3d-1.5-prizes-th.jpg]

* 2008/10/16: released v1.1
  * Show solution route (toggle via 'R' key)
    * Using this on a level disallows recording a new best score on that level.
  * Auto-forward (toggle via 'Q' key) so you don't have to hold down 'W'.
    * Note that SHIFT still toggles slow & fast modes, independently of 'Q'.
  * Some other refactoring and tweaks.

* 2008/10/16: created Wiki page FeatureWishlist.
   Please take a look and give your feedback on most desirable features.

 * 2008/07/20: first major release (v1.0), with the features listed below
  
## Platform
In the initial release, the project is developed for Windows / Visual C++.

## Features
* random 3D maze generation, with sparseness constraint
* textured 3D maze rendering
* keyboard-controlled navigation ("flying") around and through maze
* collision detection prevents flying through maze walls
* code demonstrates use of quaternions for rotation
* entrance and exit marked
* display FPS and help text
* maze solution timer and high score list
* nice skybox

## Planned features
* port to Linux
* make into screensaver (for windows and xscreensaver)
* autopilot to fly through maze
* objects in maze
* skyboxes made from Stellarium landscapes
* provide run-time control of settings
* many other ideas; see [http://maze3dflyer.googlecode.com/svn/trunk/ideas-todos.txt ideas-todos.txt]

## Screenshot
[http://maze3dflyer.googlecode.com/svn/trunk/screenshots/maze3d-0.9-scrshot2.jpg http://maze3dflyer.googlecode.com/svn/trunk/screenshots/maze3d-0.9-scrthum2.jpg]


## Instructions

### To install from a zip archive:

Extract the contents (executable and Data folder) somewhere so that the maze3dflyer.exe and the Data folder are siblings.

### To use:

Run the executable (maze3dflyer.exe) from the folder that is the parent of Data.

### Goal:

Find your way through the maze from the entrance (green ring) to the exit (red).

### Controls:

Esc: exit
H: toggle display of help text
WASD: move
Arrow keys: turn
Mouse: steer (if mouse grab is on)
Q: toggle auto-forward (instead of holding down 'W')
Home/End: go to maze entrance/exit

Shift: toggle higher speed
M or mouse-click: toggle mouse grab*
R: toggle display of solution route (disables recording new best scores)
  Length of dashes increases toward exit.
T: toggle display of framerate
L: toggle display of best score list (arrow shows current maze config)
U: toggle status bar display
G: toggle maze outline*
F1: toggle full-screen mode

Space: stop and snap camera position/orientation to grid
F7: toggle "developer" mode*
N: generate new maze of same dimensions
C: toggle collision checking (allow passing through walls or not)*
J: 'solve' current level instantly
K: sKip current level (fast)
P: toggle autopilot (not yet implemented)*
+ and - adjust key sensitivity
[ and ] adjust mouse sensitivity

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


### Command line options:

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
