#!/usr/bin/php
<?php

/**
 * @file
 * Parses all Git log messages between 2 release tags and automatically
 * generates initial HTML for the release notes. This script must be
 * run inside the root directory of a local Git repo of the project
 * you want to generate release notes for.  Assumes "git" is in your
 * PATH. The author of the CVS version was Derek Wright. Josh The Geek
 * ported the script to Git (for http://drupal.org/node/1002410).
 *
 * Usage:
 * git-release-notes.php [previous-release-tag] [current-release-tag]
 *
 * TODO:
 * - Configurable path to git binary
 * - Lookup issues on d.o to group changes by issue type (bug, feature)
 * - Should strip out leading dashes: "- something"
 * - Should remove the word "Patch " before patch #s so they are
 *   formatted consistently.
 *
 * @author Derek Wright (http://drupal.org/user/46549)
 * @author Josh The Geek (http://drupal.org/user/926382)
 *
 */

if (count($argv) < 3) {
  usage("You must specify the release tags to compare");
}
$prev_tag = $argv[1];
$cur_tag = $argv[2];

// This line allows you keep one copy of this script at a given location.
// Setup a shell alias to this file and then just call the alias from the dir
// whose notes need generating.
chdir(getcwd());

if (!is_dir(".git")) {
  usage("This script must be run from the root directory of your Git project.");
}

$rval = '';
exec('git show -s --format=%h ' . $prev_tag . '^{commit}', $prev, $rval);
if ($rval) {
  echo "ERROR: $prev_tag is not a tag.";
  exit(1);
}
$rval = '';
exec('git show -s --format=%h ' . $cur_tag . '^{commit}', $cur, $rval);
if ($rval) {
  echo "ERROR: $cur_tag is not a tag.";
  exit(1);
}

$changes = get_changes($prev, $cur);
print "<p>Changes since $prev_tag:</p>\n";
print_changes($changes);


function usage($msg = NULL) {
  global $argv;
  if (!empty($msg)) {
    print "ERROR: $msg\n";
  }
  print <<<EOF
Usage: $argv[0] [previous_release_tag] [current_release_tag]
For example:
$argv[0] 6.x-1.0 6.x-1.1

EOF;
  exit(empty($msg) ? 0 : 1);
}

// Based loosely on cvs.module cvs_process_log()
function get_changes($prev, $cur) {
  $changes = array();
  $rval = '';
  $logs = array();
  exec("git log -s --format=format:%B $prev..$cur", $logs, $rval);
  if ($rval) {
    print "ERROR: 'git log' returned failure: $rval";
    print implode("\n", $logs);
    exit(1);
  }
  while (($line = next($logs)) !== false) {
      if (strpos($line,"\n") !== false) {
        // Skip blank lines that are left behind in the messages.
        continue;
      }
      $changes[] = $line;
    }
  }
  return $changes;
}

function print_changes($changes) {
  print "<ul>\n";
  foreach ($changes as $num => $msg) {
    print '<li>' . preg_replace('/#(\d+)/', '<a href="/node/$1">#$1</a>', $obj->msg) . "</li>\n";
  }
  print "</ul>\n";
}