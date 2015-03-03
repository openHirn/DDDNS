<?php

/**
 * @file
 * PowerDNS extension for DDDNS.
 *
 * This extension replaces the generic DDDNS DNS interface in favor of
 * directly writing to a PowerDNS' mysql DB backend.
 *
 * Replace the 'ddns' section of your configuration array like follows:
 *
 * 'ddns' => array(
 *
 *  // Replace this section in your global config:
 *
 *  // Change DNS backend:
 *  'type' => 'pdns',
 *
 *	// DNS TTL in seconds. After this period, a DNS client is supposed to refresh
 *	// their data. Thus find yourself a balance between server destruction
 *	// and a time frame you can live with from remote.
 *	'ttl' => 60,
 *
 *	// The nameserver to perform the NSUPDATE against (i. e., the DDNS).
 *	'nameserver' => 'localhost',
 *
 *  	// Mandatory: Path to the dig binary. You must provide it in any case.
 *   // Dig is available in the dnsutils (Debian) package.
 *   'dig_path' => '/usr/bin/dig',
 *
 *   // Mandatory: Credentials for PDNS database backend.
 *   'db' => array(
 *     'dsn' => 'mysql:dbname=<DATABASE_NAME>;host=127.0.0.1',
 *     'user' => '<YOUR_DB_USER>',
 *     'pass' => '<YOUR_DB_PASSWORD>',
 *   ),
 * ),
 */

require_once dirname(__FILE__) . '/class.dddns.dns.inc';

class DDDNSdnsPdns extends DDDNSdns {

  private $dbh;

  /**
   * Constructor.
   *
   * @param array $config
   *   The "ddns" section of a DDDNS config array.
   *
   * @todo: Consider config, DNSDB config etc.
   */
  function __construct($config) {

    parent::__construct($config);

    //open database connection
    try {
        $this->dbh = new PDO($config['db']['dsn'], $config['db']['user'], $this->config['db']['pass']);
    } catch (PDOException $e) {
        throw new DDDNSexception($e->getMessage(), DDDNS::HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }

  }

  /**
   * Interface to system call "checkUpdateDBStruct".
   *  Check database structure is updated for ddns-pdns client.
   *    -No doubles of name and type during inserts in records table
   *      -> create index
   *
   * Check database structure is updated for ddns-pdns client.
   *  -No doubles of name and type during inserts in records table
   */
  private function checkUpdateDBStruct() {
    $sqlStr = '';
    $index = 'unique_nametype_index';

    try {
      //Read installed indexes for table records. If IX $index
      // is not installed, create it.
      $sqlStr = 'SHOW INDEX FROM records where Key_name = :index';
      $statement = $this->dbh->prepare($sqlStr);

      // @todo @security Double check sanitation.
      if (FALSE == $statement->bindParam(':index', $index, PDO::PARAM_STR)) {
        throw new DDDNSexception('BindParam \'Key_name\' failed!');
      }
      $statement->execute();
      $result = $statement->fetch(PDO::FETCH_ASSOC);

      //Create index if missing
      if (empty($result)) {
        $sqlStr = 'Create UNIQUE INDEX '.$index.' ON records (name, type)';
        $statement = $this->dbh->prepare($sqlStr);
        $statement->execute();
      }
    } catch (PDOException $e) {
        throw new DDDNSexception($e->getMessage(), DDDNS::HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }

  }

  /**
   * Interface to system call "updateDB".
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
   *
   * @return bool
   *   Operational success flag.
   */
  private function updateDB($sqlStr, $host, $value, $ttl, $type) {

    //Check if DB contains the needed index..
    $this->checkUpdateDBStruct();

    try {
      //Main domain name -> cut off chars of sub domain
      $domain = substr($host, strpos($host, '.')+1);
      //echo "Main Domain: ".$domain."\n";

      //Read domain_id of host entry to update
      $statement = $this->dbh->prepare('SELECT id FROM domains WHERE name = :host');

      // @todo @security Double check sanitation.
      if (TRUE != $statement->bindParam(':host', $domain, PDO::PARAM_STR))
        throw new DDDNSexception('BindParam \'host\' failed!');
      $statement->execute();
      $result = $statement->fetch(PDO::FETCH_ASSOC);
      if (empty($result))
        throw new DDDNSexception('Unknown domain <'.domain.'>! Update refused', DDDNS::HTTP_STATUS_INTERNAL_SERVER_ERROR);

      $prepSQL = $this->dbh->prepare($sqlStr);

      //Just bind Params contained in query string
      if ( FALSE != strpos($sqlStr, ':content') )
        $prepSQL->bindParam(':content', $value, PDO::PARAM_STR);
      if ( FALSE != strpos($sqlStr, ':host') )
        $prepSQL->bindParam(':host', $host, PDO::PARAM_STR);
      if ( FALSE != strpos($sqlStr, ':type') )
        $prepSQL->bindParam(':type', $type, PDO::PARAM_STR);
      if ( FALSE != strpos($sqlStr, ':ttl') )
        $prepSQL->bindParam(':ttl', $ttl, PDO::PARAM_STR);
      if ( FALSE != strpos($sqlStr, ':domain_id') )
        $prepSQL->bindParam(':domain_id', $result['id'], PDO::PARAM_INT);

      if ($prepSQL->execute()) {
        return TRUE;
      } else {
        throw new DDDNSexception('Update of host '.$value.' failed!', DDDNS::HTTP_STATUS_INTERNAL_SERVER_ERROR);
      }

    } catch (PDOException $e) {
        throw new DDDNSexception($e->getMessage(), DDDNS::HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }
    //usually never reached
    return FALSE;
  }

  public function addRecord($host, $value, $ttl, $type = 'A') {
    $sqlStr = 'REPLACE INTO records '.
              '(domain_id, name, type, content, ttl, prio, change_date, ordername, auth) '.
              'VALUES '.
              '(:domain_id,:host,:type,:content,:ttl,0,DATE_FORMAT(NOW(), \'%Y%m%d%H%i%S\'),NULL,1)';
    return $this->updateDB($sqlStr, $host, $value, $ttl, $type);
  }

  public function deleteRecord($host, $value = '', $type = 'A') {
    $sqlStr = "DELETE from records WHERE name = :host";
    $sqlStr .= (empty($value) ? '' : " and content = :content");
    return $this->updateDB($sqlStr, $host, $value, '', $type);
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

