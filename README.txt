Git Release Notes
=================

This is a Git port of the cvs-release-notes PHP script. The original script lives in the Drupal.org CVS repository at http://drupalcode.org/viewvc/drupal/contributions/tricks/cvs-release-notes/ .
The Git version will do the same thing, but use Git. For more information, see http://drupal.org/node/1002410 .
If you have any requests or bug reports, please go to http://drupal.org/node/1002410 and add a comment. In the GitHub issue queue, your issue will most likely go unnoticed.

This script is now a Drush extension, so you must place the file 'grn.drush.inc' in ~/.drush . For more information about installing Drush commands, see the Drush documentation at http://drush.ws .

Usage
-----

* The drush command is release-notes, alias rn.
* Options:
1. --git=PathToGit If included, git-release-notes will use the git binary specified. Otherwise, it will use Git in your path.
* Parameters:
1. tag1 The tag marking the starting point of the history output.
2. tag2 The tag marking the starting point of the history output. (See Note #1)
* Notes:
1. tag2 can also be a branch (for example drush rn 6.x-1.0 6.x-1.x). You would use this if you had not created the tag yet. This can be a pointer, for example drush rn 6.x-1.0 origin/6.x-1.x if you don't have the branch locally checked out.
2. tag1 and tag2 can be commit SHA1s.
* Warnings
1. Be careful that you either have git in your path or specify the binary with --git, the script has not been tested without a functioning git installation.
