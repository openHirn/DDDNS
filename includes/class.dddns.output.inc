<?php

/**
 * @file
 * Base class for templating.
 */

require_once dirname(__FILE__) . '/class.dddns.extension.inc';

abstract class DDDNSoutput extends DDDNSextension {

  // Internal output related header k=> storage;
  protected $headers;

  // Limit output fields. Not all fields are desired and/or secure
  // for "public" output.
  protected $outputFields = array(
    'action', 'host', 'user', 'remoteIP', 'updateIP', 'updateIPs', 'deleteIP',
    'deleteIPs', 'nameserver', 'previousIP', 'previousIPs', 'currentIP',
    'currentIPs', 'success', 'needUpdate', 'timestamp',
  );

  function __construct($config, $headers = array()) {

    parent::__construct($config);

    $this->headers = $headers;

  }

  /**
   * Public getter for $this->headers.
   *
   * @return DDDNSoutput
   *   This object, for chaining.
   */
  public function getHeaders() {
    return $this->headers;
  }

  /**
   * Sends the contents of $this->headers to PHP's header().
   *
   * @return DDDNSoutput
   *   This object, for chaining.
   */
  public function sendHeaders() {
    foreach ($this->headers as $k => $v) {
      $args = array(sprintf('%s: %s', $k, $v));

      // Special treatment for status code to enforce an integer response
      // to any standard compliant browser.
      if (preg_match ('#^X-PHP-Response-Code$#i', $k)) {
        $args[] = TRUE;
        $args[] = $v;
      }
      call_user_func_array('header', $args);
    }
    return $this;
  }

  /**
   * Set/update HTTP headers.
   *
   * @param array $headers
   *   Array with headers to be set or overridden.
   *   Use an empty string value to remove a header.
   *
   * @return DDDNSoutput
   *   This object, for chaining.
   */
  public function setHeaders($headers = array()) {
    $this->headers = array_merge($this->headers, $headers);
    return $this;
  }

  /**
   * @param DDDNSresult $resultObject
   *   DDDNS Result object.
   * @param bool $send
   *   (Optional) Whether to send the output directly to stdout.
   *   Defaults to FALSE.
   *
   */
  abstract public function output(DDDNSresult $resultObject, $send = FALSE);

}
