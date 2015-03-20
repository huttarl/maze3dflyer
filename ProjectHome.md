# maze3dflyer is: #
An OpenGL graphics demo/game. It generates a random 3D maze with some configurable properties, and displays the maze using textured 3D graphics.
You can "fly" through and around the maze using standard movement controls.

# News #
  * 2009/10/30: released v1.5.4 (minor feature release)
    * added pictures on walls (just Mona Lisa for now)
  * 2009/10/29: released v1.5.3 (minor feature release)
    * fixed [issue #4](https://code.google.com/p/maze3dflyer/issues/detail?id=#4), problem with going forward and sideways at the same time
  * 2009/10/23: released v1.5.2 (minor feature release)
    * added keys to adjust key/mouse sensitivity
  * 2009/10/23: released v1.5.1 (bug fix release)
    * Full screen mode toggle now works (textures reload).
  * 2009/06/12: released v1.5
    * Show maze as it's being generated (fun to watch). Blue = the queue of cells from which the maze is to expand. Red = cells that are marked "forbidden" for growth because they are too close to other passages.
    * Rather than just finding the route from entrance to exit, you now need to collect all the white balls on the way. More challenging.
    * See-through grates give some visibility in/out of maze.
    * Many other tweaks.
    * Let me know [what you think!](Comments.md)
![![](http://maze3dflyer.googlecode.com/svn/trunk/screenshots/maze3d-1.5-animGen-th.jpg)](http://maze3dflyer.googlecode.com/svn/trunk/screenshots/maze3d-1.5-animGen.jpg) ![![](http://maze3dflyer.googlecode.com/svn/trunk/screenshots/maze3d-1.5-prizes-th.jpg)](http://maze3dflyer.googlecode.com/svn/trunk/screenshots/maze3d-1.5-prizes.jpg)


  * 2008/10/16: released v1.1
    * Show solution route (toggle via 'R' key)
      * Using this on a level disallows recording a new best score on that level.
    * Auto-forward (toggle via 'Q' key) so you don't have to hold down 'W'.
      * Note that SHIFT still toggles slow & fast modes, independently of 'Q'.
    * Some other refactoring and tweaks.

  * 2008/10/16: created Wiki page FeatureWishlist.
> > Please take a look and give your feedback on most desirable features.

  * 2008/07/20: first major release (v1.0), with the features listed below

# Platform #
In the initial release, the project is developed for Windows / Visual C++.
A Linux port is planned.

# Features #
  * random 3D maze generation, with sparseness constraint
  * textured 3D maze rendering
  * keyboard-controlled navigation ("flying") around and through maze
  * collision detection prevents flying through maze walls
  * code demonstrates use of quaternions for rotation
  * entrance and exit marked
  * display FPS and help text
  * maze solution timer and high score list
  * nice skybox

# Planned features #
  * port to Linux
  * make into screensaver (for windows and xscreensaver)
  * autopilot to fly through maze
  * objects in maze
  * skyboxes made from Stellarium landscapes
  * provide run-time control of settings
  * many other ideas; see [ideas-todos.txt](http://maze3dflyer.googlecode.com/svn/trunk/ideas-todos.txt)

# Screenshot #
![![](http://maze3dflyer.googlecode.com/svn/trunk/screenshots/maze3d-0.9-scrthum2.jpg)](http://maze3dflyer.googlecode.com/svn/trunk/screenshots/maze3d-0.9-scrshot2.jpg)