<?php
/**
 * @file
 */

/**
 * Page callback for the 'goauth/authenticate' path.
 *
 * This page callback does every kind of authorizations.
 */
function google_drive_sync_authenticate_page() {
  $code = isset($_GET['code']) ? $_GET['code'] : NULL;

  $handler = new GoogleDriveAuthHandler(
    variable_get('googledrive_client_id'),
    variable_get('googledrive_client_secret'),
    variable_get('googledrive_redirect_uri')
  );

  if (!isset($_SESSION['redeemed_codes'])) {
    $_SESSION['redeemed_codes'] = array();
  }
  $handler->Verify($code, $_SESSION['redeemed_codes']);

  $_SESSION['redeemed_codes'][] = $code;
  $_SESSION['google_drive_sync'] = TRUE;

  drupal_goto('<front>');
}

/**
 * Admin form.
 */
function google_drive_sync_admin_auth_form() {
  $form = array();

  $form['googledrive_client_id'] = array(
    '#title' => t('Client ID'),
    '#type' => 'textfield',
    '#default_value' => variable_get('googledrive_client_id'),
  );

  $form['googledrive_client_secret'] = array(
    '#title' => t('Client Secret'),
    '#type' => 'textfield',
    '#default_value' => variable_get('googledrive_client_secret'),
  );

  $form['googledrive_redirect_uri'] = array(
    '#title' => t('Redirect URI'),
    '#type' => 'textfield',
    '#default_value' => variable_get('googledrive_redirect_uri'),
  );

  $form['googledrive_auto_sync'] = array(
    '#title' => t('Automatic document synchronization from Google Drive'),
    '#type' => 'checkbox',
    '#default_value' => variable_get('googledrive_auto_sync', 0),
  );

  $form['googledrive_manual_sync'] = array(
    '#markup' => l(t('Syncrhonize documents'), 'googledrive/sync-all'),
  );

  return system_settings_form($form);
}

/**
 * Page callback for 'googledrive/sync'
 *
 * @param $user
 */
function google_drive_sync_synchronization_page($user = NULL) {
  if (isset($user) && isset($user->googledrive)) {
    google_drive_sync_callback($user->googledrive);
    drupal_set_message(t('@name\'s documents have been synchronized.', array('@name' => $user->name)));
    drupal_goto('user');
  }
  else {
    $users_credentials = google_drive_sync_get_users_credential();

    $batch = array(
      'title' => t('Synchronize Google Documents'),
      'init_message' => t('Synchronization is starting.'),
      'progress_message' => t('Processed @current out of @total.'),
      'error_message' => t('Example Batch has encountered an error.'),
      'operations' => array(),
      'finished' => 'google_drive_sync_finished_callback',
      'file' => drupal_get_path('module', 'google_drive_sync') . '/google_drive_sync.pages.inc',
    );

    foreach ($users_credentials as $credentials) {
      $batch['operations'][] = array('google_drive_sync_batch_callback', array($credentials));
    }

    batch_set($batch);
    batch_process('admin/config/people/googledrive');
  }
}

/**
 * @param $users_credentials
 * @param $context
 */
function google_drive_sync_batch_callback($credentials, &$context) {
  if (empty($context['sandbox'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['max'] = 1;
  }
  google_drive_sync_callback($credentials);
  $context['sandbox']['progress']++;
  $context['results'][] = $credentials;

  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

function google_drive_sync_finished_callback($success, $results, $operations) {
  // The 'success' parameter means no fatal PHP errors were detected. All
  // other error management should be handled using 'results'.
  if ($success) {
    $message = format_plural(count($results), 'One user\'s document is synchronized.', '@count users\'s documents are synchronized.');
  }
  else {
    $message = t('Finished with an error.');
  }
  drupal_set_message($message);
}
