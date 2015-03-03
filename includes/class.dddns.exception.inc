<?php

/**
 * Extends standard exceptions by HTTP status code tracker.
 */

class DDDNSexception extends Exception {

  private $statusCode;

  /**
   * @param string $message
   *   (Standard Exception message).
   * @param int $statusCode
   *   HTTP status code. Defaults to 500 (most likely with exceptions).
   */
  public function __construct($message, $statusCode = 500) {
    parent::__construct($message);
    $this->statusCode = $statusCode;
  }

  public function getStatusCode() {
    return $this->statusCode;
  }

}
