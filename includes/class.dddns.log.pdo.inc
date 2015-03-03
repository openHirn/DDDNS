<?php

/**
 * @file
 * PDO database logging interface.
 */

// @todo make sure this is not a distinct mysql driver.

require_once dirname(__FILE__) . '/class.dddns.log.inc';

class DDDNSlogPdo extends DDDNSlog {

  protected $dbh;

  protected $schema = array(
    // Never provided from an incoming DDDNSresult.
    // Must be constructed by the extension, if necessary.
    // 'id'       // bigint unsigned auto_increment
    //
    // Read only (should be returned, but will not be provided):
    // 'timestamp' => PDO::PARAM_STR, // varchar(256) not null ; RFC 1123
    'nameserver' => PDO::PARAM_STR, // varchar(256) not null ; RFC 1123
    'host' => PDO::PARAM_STR,       // dto.
    'remoteHost' => PDO::PARAM_STR, // dto.
    'action' => PDO::PARAM_STR,     // varchar(128) not null
    'user' => PDO::PARAM_STR,       // varchar(128) null default null
    'remoteIP' => PDO::PARAM_STR,   // varchar(64) not null ; ipv6 in full length / RFC 2460
    'updateIPs' => PDO::PARAM_STR,  // varchar(2048) null default null ; ~30 ipV6, comma separated
    'deleteIPs' => PDO::PARAM_STR,  // dto.
    'currentIPs' => PDO::PARAM_STR, // dto.
    'previousIPs' => PDO::PARAM_STR,// dto.
    'clientNote' => PDO::PARAM_STR, // varchar(512) null default null ; free text
    'clientKey' => PDO::PARAM_STR,  // dto.
    'clientExtra' => PDO::PARAM_STR,// dto.
    'needUpdate' => PDO::PARAM_BOOL,// bool null default null
    'success' => PDO::PARAM_BOOL,   // dto.
  );

  /**
   * @param $config
   *   The "log" section of a global config.
   */
  function __construct($config) {

    parent::__construct($config['pdo']);

    // @todo validate DB schema or create | update DB.

    try {
        $this->dbh = new PDO($this->config['dsn'], $this->config['user'], $this->config['pass']);
    } catch (PDOException $e) {
        throw new DDDNSexception(sprintf('PDO error: "%s".', $e->getMessage()));
    }

  }

  /**
   * Create/update 'dddns' table schema.
   *
   * @return bool
   *   TRUE on success.
   */
  public function install() {

  //empty resultset -> table missing -> add
  $query = 'CREATE TABLE IF NOT EXISTS `dddns` ('.
      '`id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,'.
      '`timestamp` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,'.
      'PRIMARY KEY (`id`) )';

  $statement = $this->dbh->prepare($query);
  if (!$statement->execute()) {
    echo $query."\n";
    throw new DDDNSexception('Problem checking if dddns table is present!', DDDNS::HTTP_STATUS_INTERNAL_SERVER_ERROR);
  }

  //Add auto filled columns
  $autoKeys = array("id" => "bigint(20) unsigned NOT NULL AUTO_INCREMENT PRIMARY KEY",
            "timestamp" => "timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP");

    // Check all columns are present.
    foreach (array_keys($this->schemaKeys) as $k) {

      $query = sprintf('SELECT * FROM INFORMATION_SCHEMA.COLUMNS '.
              'WHERE TABLE_NAME=\'dddns\' AND column_name=\'%s\'',
              $k);
      $statement = $this->dbh->prepare($query);
      $statement->execute();
      $result = $statement->fetch(PDO::FETCH_ASSOC);
      if (empty($result)) {
        // Empty resultset -> column missing -> add.
        $query = 'ALTER TABLE dddns ADD '.$k.' '.$this->schemaKeys[$k];
        $statement = $this->dbh->prepare($query);
        if (!$statement->execute()) {
          //echo $query."\n";
          throw new DDDNSexception('Could not update dddns schema!', DDDNS::HTTP_STATUS_INTERNAL_SERVER_ERROR);
        }
      }
    }

  }

  /**
   * Pass a result data record to the attached log interface.
   *
   * @param DDDNSresult $result
   */
  public function write(DDDNSresult $result) {

    $data = $result->get();

    $dbData = array();

    // Transform result data into db schema.
    // TD: added array_kes(..) due to extended "schemaKeys" structure
    foreach (array_keys($this->schemaKeys) as $k) {
      if (isset($data[$k])) {
        $dbData[$k] = $data[$k];
      }
    }

    $query = sprintf(
      'INSERT INTO dddns (%s) VALUES (%s)',
      implode(', ', array_keys($dbData)),
      implode(', ', preg_replace('#^#', ':', array_keys($dbData)))
    );

    $q = $this->dbh->prepare($query);
    foreach (array_keys($dbData) as $k) {
      // Binds the corresponding data value AND the proper PDO::PARAM_*
      // parameter type (string, bool etc.) from the schema array.
      $q->bindParam(":$k", $dbData[$k], $this->schema[$k]);
    }

    if (!$q->execute()) {
      throw new DDDNSexception('Writing to attached PDO reporting database failed.');
    }

  }

  /**
   * Return the most recent add/update entry.
   *
   * @param string $host
   *   The host to look up.
   * @param array $actions
   *   (Optional.)
   * @param string $cKey
   *   Optional filter for "clientKey" field.
   *
   * @return DDDNSresult
   *   The most recent entry for $host.
   */
  public function check($host, $actions = array('add', 'update'), $cKey = NULL) {

    // Sanitize and prepare for query.
    // (Replaces all non-lowercase-alphas from any element, adds quotes
    // and implodes.)
    $actions = implode(', ', (array) preg_replace(array('#[^a-z]#i', '#(^|$)#'), array('', '"'), $actions));

    // The following duplicate smell cancer is brought to you by PDO
    // anti-simplicity intelligence force.
    if (isset($cKey)) {
      $q = $this->dbh->prepare(sprintf('SELECT * FROM dddns WHERE action IN (%s) AND host=:host AND (needUpdate!=1 OR success=1) AND clientKey LIKE :cKey ORDER BY timestamp DESC limit 1', $actions));
      $q->bindParam(':cKey', $cKey);
    }
    else {
      $q = $this->dbh->prepare(sprintf('SELECT * FROM dddns WHERE action IN (%s) AND host=:host AND (needUpdate!=1 OR success=1) ORDER BY timestamp DESC limit 1', $actions));
    }
    $q->bindParam(':host', $host);

    try {
      $t = $q->execute();
      if ($t) {
        if ($dbResult = $q->fetch(PDO::FETCH_ASSOC)) {
          return new DDDNSresult($dbResult);
        }
        // No record found!
        return FALSE;
      }
    }
    catch (Exception $e) {
      throw new DDDNSexception(sprintf('Database error: "%s"', $e->getMessage()));
    }

  }

}
