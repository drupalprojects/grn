#!/usr/bin/php
<?php

// $Id: cvs-release-notes.php,v 1.7 2008/02/13 22:15:33 weitzman Exp $

/**
 * @file
 * Parses all CVS log messages between 2 release tags and automatically
 * generates initial HTML for the release notes. This script must be
 * run inside the root directory of a local CVS workspace of the project
 * you want to generate release notes for.  Assumes "cvs" is in your
 * PATH, and that the workspace has already been checked out with the
 * appropriate CVSROOT.
 *
 * Usage:
 * cvs-release-notes.php [previous-release-tag] [current-release-tag]
 *
 * TODO:
 * - Option to include patch committer if "by" isn't included in message
 * - Pretty formatting of previous release version (instead of the tag)
 * - Lookup issues on d.o to group changes by issue type (bug, feature)
 * - Should strip out leading dashes: "- something"
 * - Should remove the word "Patch " before patch #s so they are
 *   formatted consistently.
 *
 * @author Derek Wright (http://drupal.org/user/46549)
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

exec('git show -s --format=%h ' . $prev_tag . '^{commit}', $prev, $rval);
if ($rval) {
  echo "ERROR: $prev_tag is not a tag.";
  exit(1);
}
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
  // Sort changes chronologically
  usort($changes, 'log_date_cmp');
  print "<ul>\n";
  foreach ($changes as $k => $obj) {
    print '<li>' . preg_replace('/#(\d+)/', '<a href="/node/$1">#$1</a>', $obj->comment) . "</li>\n";
  }
  print "</ul>\n";
}

function cvs_explode($text, $delim = ':') {
  $parts = explode($delim, $text, 2);
  return trim($parts[1]);
}

function log_date_cmp($a, $b) {
  if ($a->date == $b->date) {
    return 0;
  }
  return ($a->date < $b->date) ? -1 : 1;
}

