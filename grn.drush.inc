<?php
/**
 * @file
 * Generates HTML release notes based on commit logs in a Git repository.
 *
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
 */

/**
 * Implements hook_drush_help().
 */
function grn_drush_help($section) {
  switch ($section) {
    case 'drush:release-notes':
      return dt("Generate release notes from between two Git tags.");
  }
}

/**
 * Implements hook_drush_command().
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
      'reverse' => 'Display the commits from old to new instead of the default Git behavior that is new to old.',
      'changelog' => 'Display the commits in the format for CHANGELOG.txt as expected by drupal.org.',
    ),
    'examples' => array(
      'drush release-notes 6.x-1.0 6.x-1.1' => 'Generate release notes from all commits between 6.x-1.0 and 6.x-1.1',
      'drush release-notes 6.x-1.0 6.x-1.x' => 'Use a branch for tag2 (6.x-1.x)',
      'drush rn 6.x-1.0 6.x-1.1 --git=/usr/local/git/bin/git' => 'Use git in /usr/local/git/bin/git, and using alias',
      'drush rn 6.x-1.0 origin/6.x-1.x' => 'If you don\'t have the branch locally, you might need to use "[remote-name]/[branch-name]"',
      'drush rn 6.x-1.0 6.x-1.x --baseurl="http://community.openatrium.com/node/"' => 'You can specify the changelog to direct issues to other issue trackers.',
      'drush rn 6.x-1.0 6.x-1.x --reverse' => 'Generate release notes from the commits between the two tags in reverse order',
      'drush rn 6.x-1.0 6.x-1.x --changelog' => 'Generate release notes from the commits between the two tags in the format for CHANGELOG.txt as expected by drupal.org.'
    ),
    'aliases' => array('rn', 'relnotes'),
    'deprecated-aliases' => array('grn'), // I keep typing it, but not intuitive for others
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap needed
  );
  return $items;
}

/**
 * Implements drush_hook_COMMAND().
 */
function drush_grn_release_notes($tag1n = NULL, $tag2n = NULL) {
  $git = drush_get_option('git', 'git');
  drush_shell_exec('chdir ' . drush_cwd()); // Make sure we are in the working directory started.
  if (!is_dir(".git")) {
    drush_log("This must be run from the root directory of your Git project.");
  }

  // Fill in calculated tags if both are not given.
  if (!isset($tag2n)) {
    // Get all the defined tags in this repository and sort them.
    drush_shell_exec('%s name-rev HEAD', $git);
    $branch = substr(array_shift(drush_shell_exec_output()), 5, -1);
    drush_shell_exec('%s tag -l %s*', $git, $branch);
    $tags = drush_shell_exec_output();
    usort($tags, 'version_compare');

    if (!isset($tag1n) && count($tags)) {
      // If no tags are provided, use the two most recent ones.
      $tag2n = array_pop($tags);

      $tag1n = count($tags) ? array_pop($tags) : $tag2n;
    }
    else {
      // If only one tag is given, it is considered to be <end> and <start> is
      // taken to be one tag before it.

      $key = array_search($tag1n, $tags);
      if (is_numeric($key)) {
        if ($key > 0) {
          // Rearrange our tags: the given tag is in fact tag 2.
          $tag2n = $tag1n;
          // The <start> tag is one before the given <end> tag.
          $tag1n = $tags[$key - 1];
        }
        else {
          return drush_set_error('DRUSH_INVALID_TAG', dt('!tag is the first in the branch.', array('!tag' => $tag1n)));
        }
      }
      else {
        return drush_set_error('DRUSH_INVALID_TAG', dt('!tag is not a valid Git tag.', array('!tag' => $tag1n)));
      }
    }
  }

  // '^' is the escape character on Windows (like '\' on *nix) - has to be
  // contained in the escaped shell argument string ("%s").
  if (!drush_shell_exec('%s show -s --pretty=format:%%H %s', $git, $tag1n . '^{commit}')) {
    return drush_set_error('DRUSH_INVALID_TAG', dt('!tag is not a valid Git tag.', array('!tag' => $tag1n)));
  }
  $tag1 = drush_shell_exec_output();
  // '^' is the escape character on Windows (like '\' on *nix) - has to be
  // contained in the escaped shell argument string ("%s").
  if (!drush_shell_exec('%s show -s --pretty=format:%%H %s', $git, $tag2n . '^{commit}')) {
    return drush_set_error('DRUSH_INVALID_TAG', dt('!tag is not a valid Git tag.', array('!tag' => $tag2n)), 'error');
  }
  $tag2 = drush_shell_exec_output();
  $changes = _drush_grn_get_changes($tag1[0], $tag2[0], $git);
  $items = _drush_grn_get_items_array($changes);
  if (drush_get_option('changelog', FALSE)) {
    $formatted_items = _drush_grn_format_changelog($items, $tag2n);
  }
  else {
    $formatted_items = _drush_grn_format_changes($items, $tag1n, $tag1[0], $tag2[0], $git);
  }
  if (drush_get_context('DRUSH_PIPE')) {
    drush_print_pipe(array_keys($formatted_items['raw']));
  }
  else {
    drush_print($formatted_items['rendered']);
  }

  return TRUE;
}

// Other functions

/**
 * Generates the output.
 */
function _drush_grn_format_changes($items, $prev_tag, $tag1, $tag2, $git) {
  $rendered = "<p>Changes since $prev_tag";
  if (drush_get_option('commit-count')) {
    $rendered .= ' (' . trim(drush_get_option('commit-count')) . ' commits)';
  }
  $rendered .= ":</p>\n";

  if (!empty($items)) {
    $rendered .= "<ul>\n  <li>" . implode("</li>\n  <li>", $items) . "</li>\n</ul>";
  }

  return array('rendered' => $rendered, 'raw' => $items);
}

function _drush_grn_format_changelog($items, $tag) {
  drush_shell_exec("more *.info | grep name");
  $name = "";
  $info = drush_shell_exec_output();
  $info = explode("=", $info[0]);
  if (isset($info[1])) {
    $name = trim($info[1]);
  }
  drush_shell_exec("git show -s --pretty=format:%ad --date=short $tag");
  $date = drush_shell_exec_output();
  $changelog = $name . " " . $tag . ", " . $date[0] . "\n";
  $changelog .= str_pad("", strlen($changelog), "-");
  $changelog .= "\n";
  foreach ($items as $raw => $html) {
    $changelog .= '- ';
    $line = ucfirst(trim((strpos($raw, "#") === 0) ? substr(strstr($raw, ':'), 1) : $raw));
    $changelog .= substr($line, -1) == "." ? $line : $line . ".";
    $changelog .= "\n";
  }
  return array('raw' => $items, 'rendered' => $changelog);
}

/**
 * Gets the changes and returns them in an array.
 */
function _drush_grn_get_changes($tag1, $tag2, $git) {
  $changes = array();

  $reverse = drush_get_option('reverse', FALSE) ? '--reverse' : '';
  if (!drush_shell_exec("%s log -s --pretty=format:%%s %s %s..%s", $git, $reverse, $tag1, $tag2)) {
    return drush_set_error('DRUSH_GIT_LOG_ERROR', 'git log returned an error.');
  }
  $output = drush_shell_exec_output();
  $changes[] = $output[0]; // Otherwise, next() would skip first
  while (($line = next($output)) !== FALSE) {
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

function _drush_grn_get_items_array($issues) {
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
  drush_command_invoke_all_ref('release_notes_output_alter', $items);
  return $items;
}
