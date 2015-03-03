<?php

require_once dirname(__FILE__) . '/class.dddns.extension.inc';

/**
 * DNS abstration interface base class.
 *
 * Detects the appropriate DNS interface class and tries to load it.
 */

abstract class DDDNSdns extends DDDNSextension {

  /**
   * System interface to add a DNS record.
   *
   * @param string $host
   *   The host to be updated.
   * @param string $value
   *   The value to set (most likely an IP).
   * @param string $ttl
   *   The TTL to set.
   * @param string $type
   *   The record type to set. Defaults to "A".
   *
   * @return mixed
   *   May return a boolean indicating success or failure.
   *   Should return NULL (nothing) otherwise ("ambiguous").
   */
  abstract public function addRecord($host, $value, $ttl, $type = 'A');

  /**
   * System interface to delete a DNS record.
   *
   * @param string $host
   *   The host to be updated.
   * @param string $value
   *   The value to delete. Defaults to an empty string ("ANY").
   * @param string $type
   *   The record type to set. Defaults to "A".
   *
   * @return mixed
   *   May return a boolean indicating success or failure of a deletion.
   *   Should return NULL otherwise ("ambiguous").
   */
  abstract public function deleteRecord($host, $value = '', $type = 'A');

  /**
   * NS lookup system interface.
   *
   * Issues an nslookup request and parses the response into a structured form,
   * with one k=>v details array per existing record.
   * Note that dealing with multiple records MUST be supported here.
   *
   * @param string $host
   *   Host name to be checked.
   * @param string $recordType
   *   What kind of record to check. Defaults to IPv4 address ("A").
   *   If set to an empty string, all found entries are returned.
   *
   * @return array
   *   Returns the current record data like so:
   *   array(
   *     0 => array(
   *       'record' => overall record line as returned by dig;
   *                   run dig to see an example.
   *       'ttl' => integer: TTL in seconds,
   *       'type' => string: type (e. g. "A"),
   *       'value' => string: value (e. g. "127.0.0.1"),
   *     ),
   *     [1, .. => further sets]
   *   );
   */
  abstract public function lookup($host, $type = 'A');

}


