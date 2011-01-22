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
 * drush release-notes [previous-release-tag] [current-release-tag]
 *
 * This is the Drush version of this script.
 *
 * @author Derek Wright (http://drupal.org/user/46549)
 * @author Josh The Geek (http://drupal.org/user/926382)
 *
 */

/**
 * Ipmlementation of hook_drush_help().
 */
function grn_drush_help($section) {
  switch ($section) {
    case 'drush:release-notes':
      return dt("Generate release notes from between two Git tags.");
  }
}

/**
 * Implementation of hook_drush_command().
 */
function grn_drush_command() {
  $items = array();
  
  // The 'release-notes' command
  $items['release-notes'] = array(
    'description' => 'Generate release notes using all commits between two tags',
    'arguments' => array(
      'tag 1' => 'The previous tag, the starting point for the log.',
      'tag 2' => 'The current tag, the ending point for the log. This can be a branch, too, see example 2',
    ),
    'options' => array(
      'git' => 'Path to the git binary, defaults to "git"',
    ),
    'examples' => array(
      'drush release-notes 6.x-1.0 6.x-1.1' => 'Generate release notes from all commits between 6.x-1.0 and 6.x-1.1',
      'drush release-notes 6.x-1.0 6.x-1.x' => 'Generate release notes from all commits between 6.x-1.0 and 6.x-1.x (a branch)',
      'drush rn 6.x-1.0 6.x-1.1 --git=/usr/local/git/bin/git' => 'Generate release notes from all commits between 6.x-1.0 and 6.x-1.1, with git in /usr/local/git/bin/git, and using alias',
      'drush rn 6.x-1.0 origin/6.x-1.x' => 'If you don\'t have the branch locally, you might need to use "[remote-name]/[branch-name]"',
    ),
    'aliases' => array('rn'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );
  return $items;
}

/**
 * Implementation of drush_hook_COMMAND().
 */
function drush_grn_release_notes($tag1n, $tag2n) {
  $git = 'git';
  $additional = '.';
  if (drush_get_option('git')) {
    $git = drush_get_option('git');
    $additional = ' with Git at !git.';
  }
  //drush_log(dt('Generating release notes between !tag1 and !tag2' . $additional, array('!tag1' => $tag1n, '!tag2' => $tag2n, '!git' => $git)));
  chdir(getcwd());
  if (!is_dir(".git")) {
    drush_log("This must be run from the root directory of your Git project.");
  }
  if (!drush_shell_exec('%s show -s --format=%%H %s^{commit}', $git, $tag1n)) {
    drush_log(dt('!tag is not a valid Git tag.', array('!tag' => $tag1n)), 'error');
  }
  $tag1 = drush_shell_exec_output();
  if (!drush_shell_exec('%s show -s --format=%%H %s^{commit}', $git, $tag2n)) {
    drush_log(dt('!tag is not a valid Git tag.', array('!tag' => $tag2n)), 'error');
  }
  $tag2 = drush_shell_exec_output();
  $changes = _drush_grn_get_changes($tag1[0], $tag2[0], $git);
  _drush_grn_print_changes($changes, $tag1n);
}

// Other functions

function _drush_grn_print_changes($changes, $prev_tag) {
  $return = "<p>Changes since $prev_tag:</p>\n";
  $return .= "<ul>\n";
  foreach ($changes as $type => $issues) {
    $return .= "<li>$type<ul>\n";
    foreach ($issues as $number => $line) {
      $print = '<li>' . preg_replace('/^Patch /', '', preg_replace('/^- /', '', preg_replace('/#(\d+)/', '<a href="/node/$1">#$1</a>', $line))) . "</li>\n";
      if ($print != "<li></li>\n") {
        $return .= $print;
      }
    }
    $return .= "</ul></li>\n";
  }
  $return .= "</ul>\n";
  return $return;
}

function _drush_grn_get_changes($tag1, $tag2, $git) {
  $changes = array();
  $rval = '';
  $logs = array();
  if (!drush_shell_exec("%s log -s --format=format:%%B %s..%s", $git, $tag1, $tag2)) {
    drush_log('git log returned an error.', 'error');
  }
  $output = drush_shell_exec_output();
  $changes[_drush_grn_get_issue_type($output[0])][] = $logs[0];
  while (($line = next($output)) !== false) {
      if ($line == '') {
        // Skip blank lines that are left behind in the messages.
        continue;
      }
      $changes[_drush_grn_get_issue_type($line)][] = $line;
    }
  return $changes;
}

function _drush_grn_get_issue_type($line) {
  preg_match('/#(\d+)/', $line, $matches);
  if (!isset($matches[1]) || empty($matches[1])) {
    return $return = 'Other changes:';
  }
  $issue = trim($matches[1], ' .a..zA..Z#:/');
  if (!is_numeric($issue)) {
    echo "ERROR: '$issue' is not a valid issue number.";
  }
  $type = _drush_grn_get_page_data($issue);
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

function _drush_grn_get_page_data($issue) {
  $raw = file_get_contents("http://drupal.org/node/" . $issue);
  // Get type
  $init = strstr($raw, '  <div class="node-content">
    <div class="project-issue"><div id="project-summary-container" class="clear-block"><div id="project-issue-summary-table" class="summary"><table>
');
  $tfront = strstr($init,'<tr class="even"><td>Category:</td><td>'); // Find what's after the start tag
  $type = strstr($tfront,'</td> </tr>
 <tr class="odd"><td>Priority:</td><td>',true); // And filter it to get the count
  return $type;
}
