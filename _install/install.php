<?php

/**
 * @file
 * DB installer/updater tasks for DDDNS.
 */

require_once dirname(__FILE__) . '/../includes/class.dddns.inc';

$dddns = new DDDNS(array('host' => 'localhost'));

var_dump($dddns->install()->getResult());
