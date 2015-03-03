<?php

/**
 * @file
 * DDDNS web frontend.
 */

require_once dirname(__FILE__) . '/../includes/class.dddns.inc';

$dddns = new DDDNS();

try {
  $dddns->execute()->sendOutput();
} catch (DDDNSexception $e) {
  if ($c = $e->getStatusCode()) {
    header(sprintf('X-PHP-Response-Code: %s', $c), TRUE, $c);
  }
  printf('An unexpected error has occured: %s (HTTP code %s).', $e->getMessage(), $c);
}
