<?php

/**
 * @file
 * Generic DNS extension for DDDNS.
 *
 * This is the default DNS source interface. It handles standard calls
 * using dnsutils package components (nsupdate, dig).
 * Unless you have a non-standard BIND9 environment or additional
 * requirements, there is no need for a replacement.
 *
 * The following additional paramaters may be added to the global
 * configuration array:
 *
 * array(
 *   'ddns' => array(
 *
 *     // Add this inside your existing configuration, if necessary:
 *
 *     // Optional: Alternative path to the nsupdate binary, if not in /usr/bin.
 *     'update_path' => '/path/to/nsupdate',
 *     // Optional: Alternative path to the dig binary, if not in /usr/bin.
 *     'dig_path' => '/path/to/dig',
 *     // Optional path to an RNDC key for the update (only fill in if required).
 *     // Mind file permissions, bind must be allowed to read it.
 *     'keyfile_path' => '',
 * ),
 */

require_once dirname(__FILE__) . '/class.dddns.dns.inc';

class DDDNSdnsGeneric extends DDDNSdns {

  /**
   * Generic "nsupdate" system call.
   *
   * Performs a system nsupdate "add" or "delete" request with the
   * provided data against the configured nameserver.
   *
   * @param string $action
   *   The action to perform. In a DDNS scenario, this is "add" and "delete".
   * @param string $host
   *   The host to be updated.
   * @param string $value
   *   The value to set (in DDNS case this is an IP or nothing for "delete").
   * @param string $ttl
   *   The TTL to set, mandatory for "add" and forbidden for "delete".
   * @param string $type
   *   The record type to set. For DDNS, most likely an "A" entry.
   */
  protected function nsupdate($action, $host, $value = '', $ttl = '', $type = 'A') {

    // Prepare optional RNDC section for popen statement.
    $rndc = empty($this->config['keyfile_path']) ? '' : sprintf('-k %s', $this->config['keyfile_path']);

    // Prepare popen statement. Suppress any console output in order
    // not to pollute our HTTP response.
    $command = sprintf('%s %s -d 2> /dev/null', $this->config['update_path'], $rndc);

    if ($fh = popen($command, 'w')) {
      fwrite($fh, sprintf("server %s.\n", $this->config['nameserver']));
      fwrite($fh, sprintf(
        "update %s %s %s %s %s\n",
        $action,
        $host,
        $ttl,
        $type,
        $value
      ));
      fwrite($fh, "send\n");
      pclose($fh);
    }

  }

  public function addRecord($host, $value, $ttl, $type = 'A') {
    return $this->nsupdate('add', $host, $value, $ttl, $type);
  }

  public function deleteRecord($host, $value = '', $type = 'A') {
    return $this->nsupdate('delete', $host, $value, '', $type);
  }

  /**
   * Interface to BIND system "dig".
   *
   * Issues an nslookup request to bind's "dig" tool and parses the response
   * in a structured form, with one k-v details array per existing record.
   * Note that dealing with multiple records MUST be supported here.
   *
   * @param string $host
   *   Host name to be checked.
   * @param string $recordType
   *   What kind of record to check. Defaults to IPv4 address ("A").
   *   If set to an empty value, all found entries are returned.
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
  public function lookup($host, $type = 'A') {

    $ret = array();

    if (!empty($host)) {
      $result = shell_exec(sprintf('%s @%s %s', $this->config['dig_path'], $this->config['nameserver'], $host));

      // First, check if there is something in the answer section at all.
      if (preg_match('#^;; ANSWER SECTION:\\s*?\\n(?<answer>.*?)\\r?\\n\\r?\\n#ms', $result, $raw)) {
        if (
          preg_match_all(
            sprintf('#^(?<record>%s\\.\\s+(?<ttl>[0-9]+)\\s+IN\\s+(?<type>[^\\s]+)\\s+(?<value>[^\\s]+))\\s*?(?:\\n|$)#m', $host),
            $raw['answer'],
            $match,
            PREG_SET_ORDER
          )
        ) {
          $ret = $match;
        }
      }
    }

    return $ret;
  }

}
