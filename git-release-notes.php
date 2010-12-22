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
 * - Make this a Drush extension
 *
 * @author Derek Wright (http://drupal.org/user/46549)
 * @author Josh The Geek (http://drupal.org/user/926382)
 *
 */

if (count($argv) < 3) {
  usage("You must specify the release tags to compare.");
}
$prev_tag = $argv[1];
$cur_tag = $argv[2];
global $git;
if (!isset($argv[3]) || empty($argv[3])) {
  $git = 'git';
}
else {
  $git = $argv[3];
}

// This line allows you keep one copy of this script at a given location.
// Setup a shell alias to this file and then just call the alias from the dir
// whose notes need generating.
chdir(getcwd());

if (!is_dir(".git")) {
  usage("This script must be run from the root directory of your Git project.");
}

$rval = '';
exec("$git show -s --format=%h " . $prev_tag . '^{commit}', $prev, $rval);
if ($rval) {
  echo "ERROR: $prev_tag is not a tag.";
  exit(1);
}
$rval = '';
exec("$git show -s --format=%h " . $cur_tag . '^{commit}', $cur, $rval);
if ($rval) {
  echo "ERROR: $cur_tag is not a tag.";
  exit(1);
}

$changes = get_changes($prev[0], $cur[0], $git);
print "<p>Changes since $prev_tag:</p>\n";
print_changes($changes);


function usage($msg = NULL) {
  global $argv;
  if (!empty($msg)) {
    print "ERROR: $msg\n";
  }
  print <<<EOF
Usage: $argv[0] previous_release_tag current_release_tag [path/to/git/binary]
For example:
$argv[0] 6.x-1.0 6.x-1.1 /usr/local/git/bin/git

EOF;
  exit(empty($msg) ? 0 : 1);
}

function get_page_data($issue) {
  $url = "http://drupal.org/node/" . $issue;
  // cURL
  $curl = curl_init($url); // Initiate transfer
  curl_setopt($curl, CURLOPT_RETURNTRANSFER, true); // Don't write to stdout
  $raw = curl_exec($curl); // Get it and set the raw data
  curl_close($curl); // Close it to free memory
  // Get type
  $init = strstr($raw, '  <div class="node-content">
    <div class="project-issue"><div id="project-summary-container" class="clear-block"><div id="project-issue-summary-table" class="summary"><table>
');
  $tfront = strstr($init,'<tr class="even"><td>Category:</td><td>'); // Find what's after the start tag
  $type = strstr($tfront,'</td> </tr>
 <tr class="odd"><td>Priority:</td><td>',true); // And filter it to get the count
  return $type;
}

function get_issue_type($line) {
  preg_match('/#(\d+)/', $line, $matches);
  if (!isset($matches[1]) || empty($matches[1])) {
    return $return = 'Other changes:';
  }
  $issue = trim($matches[1], ' .a..zA..Z#:/');
  if (!is_numeric($issue)) {
    echo "ERROR: '$issue' is not a valid issue number.";
  }
  $type = get_page_data($issue);
  if ($type == '<tr class="even"><td>Category:</td><td>feature request') {
    $return = 'New features:';
  }
  else if ($type == '<tr class="even"><td>Category:</td><td>bug report') {
    $return = 'Bug fixes:';
  }
  else {
    $return = 'Other changes:';
  }
  return $return;
}

function get_changes($prev, $cur, $git) {
  $changes = array();
  $rval = '';
  $logs = array();
  exec("$git log -s --format=format:%B $prev..$cur", $logs, $rval);
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
      $changes[get_issue_type($line)][] = $line;
    }
  return $changes;
}

function print_changes($changes) {
  print "<ul>\n";
  foreach ($changes as $type => $issues) {
    echo "<li>$type<ul>\n";
    foreach ($issues as $number => $line) {
      $print = '<li>' . preg_replace('/^Patch /', '', preg_replace('/^- /', '', preg_replace('/#(\d+)/', '<a href="/node/$1">#$1</a>', $line))) . "</li>\n";
      if ($print != "<li></li>\n") {
        echo $print;
      }
    }
    echo "</ul></li>\n";
  }
  print "</ul>\n";
}
