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
      'commit-count' => 'If set, output will show the number of commits between the two tags',
    ),
    'examples' => array(
      'drush release-notes 6.x-1.0 6.x-1.1' => 'Generate release notes from all commits between 6.x-1.0 and 6.x-1.1',
      'drush release-notes 6.x-1.0 6.x-1.x' => 'Generate release notes from all commits between 6.x-1.0 and 6.x-1.x (a branch)',
      'drush rn 6.x-1.0 6.x-1.1 --git=/usr/local/git/bin/git' => 'Generate release notes from all commits between 6.x-1.0 and 6.x-1.1, with git in /usr/local/git/bin/git, and using alias',
      'drush rn 6.x-1.0 origin/6.x-1.x' => 'If you don\'t have the branch locally, you might need to use "[remote-name]/[branch-name]"',
    ),
    'aliases' => array('rn'),
    'deprecated-aliases' => array('grn'), // I keep typing it, but not intuitive for others
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
  }
  chdir(getcwd());
  if (!is_dir(".git")) {
    drush_log("This must be run from the root directory of your Git project.");
  }
  if (!drush_shell_exec('%s show -s --format=%%H %s^{commit}', $git, $tag1n)) {
    return drush_set_error('DRUSH_INVALID_TAG', dt('!tag is not a valid Git tag.', array('!tag' => $tag1n)));
  }
  $tag1 = drush_shell_exec_output();
  if (!drush_shell_exec('%s show -s --format=%%H %s^{commit}', $git, $tag2n)) {
    return drush_set_error('DRUSH_INVALID_TAG', dt('!tag is not a valid Git tag.', array('!tag' => $tag2n)), 'error');
  }
  $tag2 = drush_shell_exec_output();
  $changes = _drush_grn_get_changes($tag1[0], $tag2[0], $git);
  drush_print(_drush_grn_format_changes($changes, $tag1n, $tag1[0], $tag2[0], $git));
  return true;
}

// Other functions

/**
 * Generate the output
 */
function _drush_grn_format_changes($issues, $prev_tag, $tag1, $tag2, $git) {
  $return = "<p>Changes since $prev_tag";
  if (drush_get_option('commit-count')) {
    $return .= ' (' . trim(drush_get_option('commit-count')) . ' commits)';
  }
  $return .= ":</p>\n";
  $return .= "<ul>\n";
  foreach ($issues as $number => $line) {
    $print = '<li>' . preg_replace('/^(Patch |- |Issue ){0,3}/', '', preg_replace('/#(\d+)/', '<a href="/node/$1">#$1</a>', $line)) . "</li>\n";
    if ($print != "<li></li>\n") {
      $return .= $print;
    }
  }
  $return .= "</ul>\n";
  return $return;
}

/**
 * Get the changes, and return them in an array sorted by issue type, if available
 */
function _drush_grn_get_changes($tag1, $tag2, $git) {
  $changes = array();
  if (!drush_shell_exec("%s log -s --format=%%B %s..%s", $git, $tag1, $tag2)) {
    return drush_set_error('DRUSH_GIT_LOG_ERROR', 'git log returned an error.');
  }
  $output = drush_shell_exec_output();
  $changes[] = $output[0]; // Otherwise, next() would skip first
  while (($line = next($output)) !== false) {
    if (empty($line)) {
      // Skip blank lines that are left behind in the messages.
      continue;
    }
    $changes[] = $line;
  }
  if (drush_get_option('commit-count')) {
    drush_set_option('commit-count', count($changes));
  }
  return $changes;
}
