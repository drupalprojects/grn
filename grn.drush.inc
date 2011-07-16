<?php
/**
 * @file
 * Parses all Git log messages between 2 release tags and automatically
 * generates initial HTML for the release notes. This script must be
 * run inside the root directory of a local Git repo of the project
 * you want to generate release notes for.  Assumes "git" is in your
 * PATH. Otherwise you should set the --git option. The author of the
 * CVS version was dww. Josh The Geek ported the script to Git
 * (for http://drupal.org/node/1002410).
 *
 * Usage:
 * drush release-notes [previous-release-tag] [current-release-tag]
 *
 * This is the Drush version of this script.
 *
 * @author dww (http://drupal.org/user/46549)
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
      'baseurl' => 'Set the base url for all issue links. Defaults to /node/ for Drupal.org usage. Issue number will be appended to path or replace "%s".',
    ),
    'examples' => array(
      'drush release-notes 6.x-1.0 6.x-1.1' => 'Generate release notes from all commits between 6.x-1.0 and 6.x-1.1',
      'drush release-notes 6.x-1.0 6.x-1.x' => 'Use a branch for tag2 (6.x-1.x)',
      'drush rn 6.x-1.0 6.x-1.1 --git=/usr/local/git/bin/git' => 'Use git in /usr/local/git/bin/git, and using alias',
      'drush rn 6.x-1.0 origin/6.x-1.x' => 'If you don\'t have the branch locally, you might need to use "[remote-name]/[branch-name]"',
      'drush rn 6.x-1.0 6.x-1.x --baseurl="http://community.openatrium.com/node/"' => 'You can specify the changelog to direct issues to other issue trackers.',
    ),
    'aliases' => array('rn', 'relnotes'),
    'deprecated-aliases' => array('grn'), // I keep typing it, but not intuitive for others
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap needed
  );
  return $items;
}

/**
 * Implementation of drush_hook_COMMAND().
 */
function drush_grn_release_notes($tag1n, $tag2n) {
  $git = 'git';
  if (drush_get_option('git')) {
    $git = drush_get_option('git');
  }
  chdir(getcwd()); // Make sure we're in the working directory started.
  if (!is_dir(".git")) {
    drush_log("This must be run from the root directory of your Git project.");
  }
  if (!drush_shell_exec('%s show -s --pretty=format:%%H %s^{commit}', $git, $tag1n)) {
    return drush_set_error('DRUSH_INVALID_TAG', dt('!tag is not a valid Git tag.', array('!tag' => $tag1n)));
  }
  $tag1 = drush_shell_exec_output();
  if (!drush_shell_exec('%s show -s --pretty=format:%%H %s^{commit}', $git, $tag2n)) {
    return drush_set_error('DRUSH_INVALID_TAG', dt('!tag is not a valid Git tag.', array('!tag' => $tag2n)), 'error');
  }
  $tag2 = drush_shell_exec_output();
  $changes = _drush_grn_get_changes($tag1[0], $tag2[0], $git);

  $output = _drush_grn_format_changes($changes, $tag1n, $tag1[0], $tag2[0], $git);
  if (drush_get_context('DRUSH_PIPE')) {
    drush_print_pipe(array_keys($items['raw']));
  }
  else {
    drush_print($items['rendered']);
  }

  return true;
}

// Other functions

/**
 * Generate the output
 */
function _drush_grn_format_changes($issues, $prev_tag, $tag1, $tag2, $git) {
  $rendered = "<p>Changes since $prev_tag";
  if (drush_get_option('commit-count')) {
    $rendered .= ' (' . trim(drush_get_option('commit-count')) . ' commits)';
  }
  $rendered .= ":</p>\n";

  $baseurl = drush_get_option('baseurl', '/node/');
  if (strpos($baseurl, '%s') == FALSE) {
    $baseurl .= '%s';
  }

  $items = array();
  foreach ($issues as $number => $line) {
    // Clean up commit log.
    $item = preg_replace('/^(Patch |- |Issue ){0,3}/', '', $line);
    // Add issue links.
    $items[$item] = preg_replace('/#(\d+)/S', '<a href="' . str_replace('%s', '$1', $baseurl) . '">#$1</a>', $item);
  }

  if (!empty($items)) {
    $rendered .= "<ul>\n  <li>" . implode("</li>\n  <li>", $items) . "</li>\n</ul>";
  }

  return array('rendered' => $rendered, 'raw' => $items);
}

/**
 * Get the changes, and return them in an array sorted by issue type, if available
 */
function _drush_grn_get_changes($tag1, $tag2, $git) {
  $changes = array();
  if (!drush_shell_exec("%s log -s --pretty=format:%%s %s..%s", $git, $tag1, $tag2)) {
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
