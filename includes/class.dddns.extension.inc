<?php

/**
 * @file
 * Base class for any extensions.
 */
abstract class DDDNSextension {

  protected $config;

  /**
   * @param array $config
   *   The section of a DDDNS config array related to the extension.
   *   E. g. for a DNS interface extension, the 'ddns' content of
   *   the general config.
   */
  function __construct($config) {

    $this->config = $config;

  }

}
