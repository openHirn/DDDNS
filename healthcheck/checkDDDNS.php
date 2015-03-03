<?php

/**
 * @file
 * DDDNS health check wrapper for CLI (bash, crontab) use.
 *
 * Although a matter of course, remember that this script will not make
 * any sense unless it runs outside the dynamic IP LAN you want to
 * monitor (but ideally on your DDDNS or any other independent server).
 *
 * From the command line, you need to provide these arguments:
 *
 * @param string host
 *   The host to check, e.g. "dyn.example.com". (Required.)
 *
 * @param string mail
 *   One or more mail recipients to be informed. Separate multiple
 *   recipients by semicolons. (Required.)
 *
 * @param int threshold
 *   A positive integer defining the lookback limit
 *   in seconds. ("Check whether an update was performed
 *   within the last n seconds.") (Required.)
 *
 * @param string disable
 *   If set to "true", delete all records for the given host.
 */

$dddnsParams = array('show' => 'check');

foreach (array(1, 2, 3, 4) as $i) {
  $arg = @$argv[$i];
  switch ($i) {
    case 1:
      if (empty($arg)) {
        echo 'Error: No host name given.';
        exit(1);
      }
      $dddnsParams['host'] = $arg;
      break;
    case 2:
      if (empty($arg)) {
        echo 'Error: No mail recipient given.';
        exit(2);
      }
      $dddnsParams['mailto'] = $arg;
      break;
    case 3:
      if (empty($arg)) {
        echo 'Error: No threshold given.';
        exit(3);
      }
      $dddnsParams['lookback'] = $arg;
      break;
    case 4:
      if ($arg == 'true') {
        $dddnsParams['cleanup'] = $arg;
      }
      break;
  }
}

require_once dirname(__FILE__) . '/../includes/class.dddns.inc';

$dddns = new DDDNS($dddnsParams);

$result = $dddns->execute()->getResult()->get();

if (FALSE === $result['success']) {
  printf('Health check for %s failed.', $result['host']);
  exit(9);
}

printf(
  'The health check for %s succeeded. Last update was at %s from %s by %s.',
  $result['host'],
  $result['timestamp'],
  $result['remoteIP'],
  $result['user']
);
