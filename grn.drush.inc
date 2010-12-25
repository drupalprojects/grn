<?php

/**
 * @file
 *   Git Release Notes: Drush Extension
 *
 *   This command will generate release notes for your project on
 *   Drupal.org between to Git tags.
 *
 *   You can copy this file to any of the following
 *     1. A .drush folder in your HOME folder.
 *     2. Anywhere in a folder tree below an active module on your site.
 *     3. /usr/share/drush/commands (configurable)
 *     4. In an arbitrary folder specified with the --include option.
 */

/**
 * Implementation of hook_drush_command().
 *
 * In this hook, you specify which commands your
 * drush module makes available, what it does and
 * description.
 *
 * Notice how this structure closely resembles how
 * you define menu hooks.
 *
 * @See drush_parse_command() for a list of recognized keys.
 *
 * @return
 *   An associative array describing your command(s).
 */
function grn_drush_command() {
  $items['generate-notes'] = array(
    'description' => 'Generate release notes with all changes between two Git tags.',
    'arguments' => array(
      'first-tag' => 'The starting point of the list. This should be a valid Git tag name.',
      'second-tag' => 'The ending point of the list. This should be a valid Git tag name.',
    ),
    'options' => array(
      '--git' => 'Path to the Git binary, if not in your path.',
    ),
    'examples' => array(
      'drush notes 6.x-1.0 6.x-1.1' => 'Generate release notes with all changes between version 6.x-1.0 and 6.x-1.1',
      'drush notes 6.x-1.0 6.x-1.1 --git=/usr/local/git/bin/git' => 'Generate release notes with all changes between version 6.x-1.0 and 6.x-1.1 and the Git binary in /usr/local/git/bin/git.',
    ),
    'aliases' => array(/*'grn', */'notes'),
//    'deprecated-aliases' => array('release-notes'),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUSH, // No bootstrap at all.
  );
}

/**
 * Implementation of hook_drush_help().
 *
 * This function is called whenever a drush user calls
 * 'drush help <name-of-your-command>'
 *
 * @param
 *   A string with the help section (prepend with 'drush:')
 *
 * @return
 *   A string with the help text for your command.
 */
function grn_drush_help($section) {
  switch ($section) {
    case 'drush:generate-notes':
      return dt("Generate release notes for a project with all changes between Git two tags. You must be in the root directory of a Git repo.");
  }
}

/*
 * Implementation of drush_hook_COMMAND_validate().
 */
function drush_grn_grn_generate_validate() {
  if ($name['name'] !== 'root') {
    return drush_set_error('MAKE_IT_YOUSELF', dt('What? Make your own grn.'));
  }
}

/**
 * Example drush command callback. This is where the action takes place.
 *
 * The function name should be same as command name but with dashes turned to
 * underscores and 'drush_commandfile_' prepended, where 'commandfile' is
 * taken from the file 'commandfile.drush.inc', which in this case is 'grn'.
 * Note also that a simplification step is also done in instances where
 * the commandfile name is the same as the beginning of the command name,
 * "drush_example_example_foo" is simplified to just "drush_example_foo".
 * To also implement a hook that is called before your command, implement
 * "drush_hook_pre_example_foo".  For a list of all available hooks for a
 * given command, run drush in --debug mode.
 *
 * If for some reason you do not want your hook function to be named
 * after your command, you may define a 'callback' item in your command
 * object that specifies the exact name of the function that should be
 * called.  However, the specified callback function must still begin
 * with "drush_commandfile_" (e.g. 'callback' => "drush_example_foo_execute").
 * All hook functions are still called (e.g. drush_example_pre_foo_execute,
 * and so on.)
 *
 * In this function, all of Drupal's API is (usually) available, including
 * any functions you have added in your own modules/themes.
 *
 */
function drush_grn_generate_notes($tag1n, $tag2n) {
  $git = 'git';
  $additional = '.';
  if (drush_get_option('git')) {
    $git = drush_get_option('git');
    $additional = ' with Git at !git.';
  }
  $msg = dt('Generating release notes between !tag1 and !tag2' . $additional, array('!tag1' => $tag1, '!tag2' => $tag2, '!git' => $git));
  drush_print($msg);
  drush_print(file_get_contents(dirname(__FILE__) . '/grn.txt'));
}
