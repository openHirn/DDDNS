<?php

/**
 * @file
 * DDDNS helper class for standardized data storage.
 */

require_once dirname(__FILE__) . '/class.dddns.exception.inc';

/**
 * Result standardization and abstracion class.
 */
class DDDNSresult {

  // Effective result array (to be filtered by $this->format[$key]).
  private $result;

  /**
   * @param array $defaults
   *   A default result set, e.g. from a DB call.
   */
  function __construct($defaults = array()) {
    $this->result = $defaults;
  }

  /**
   * Public setter for $this->result.
   *
   * @param $id
   * @param $data
   *
   * @return DDDNSresult
   *   $this for chaining.
   */
  public function set($id, $data) {

    if (empty($id) || is_array($id)) {
      throw new DDDNSexception(sprintf('Invalid result id "%s".', $id));
    }
    $this->result[$id] = $data;

    return $this;

  }

  /**
   * Unset one or more values.
   *
   * @param array|string $keys
   *
   * @return DDDNSresult
   *   $this for chaining.
   */
  public function delete($keys) {

    if (!empty($keys)) {
      foreach ((array) $keys as $key) {
        unset($this->result[$key]);
      }
    }

    return $this;

  }

  /**
   * Store relevant data from results.
   *
   * @param string $id
   *   Arbitrary key name.
   * @param array $data
   *   @todo: Replace with DDDNSdnsData object type.
   *
   * @return DDDNSresult
   *   $this for chaining.
   */
  public function setDNS($id, $data) {
    return $this->setIP("{$id}IP", $data['IP']);
  }

  /**
   * Store one ore more IPs in a standardized way.
   *
   * @param $id
   * @param string|array $data
   *
   * @return DDDNSresult
   *   $this for chaining.
   */
  public function setIP($id, $data) {
    $this->set($id, $data);
    return $this->set("{$id}s", implode(', ', (array) $data));
  }


  /**
   * Getter for $this->result (or parts of it).
   *
   * @param string|array $keys
   *   Single key or array of keys to be returned from $this->result.
   *
   * @return mixed
   *   - If $keys is empty, all of $this->result (an array in any case).
   *   - If $keys is a string, the $this->result value for $key as stored.
   *   - If $keys is an array, all values represented in $keys in an array.
   */
  public function get($keys = NULL) {

    if (is_array($keys)) {
      $keys = array_fill_keys($keys, TRUE);
      return array_intersect_key($this->result, $keys);
    }
    elseif (!empty($keys)) {
      return $this->result[$keys];
    }
    return $this->result;

  }

}