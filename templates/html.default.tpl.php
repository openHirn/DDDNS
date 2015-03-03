<?php

/**
 * @file
 * Default template for the DDDNS web frontend.
 *
 * This template is used for the frontend output.
 * It is parsed with the following variables:
 *
 * string $action
 *   The effective action resulting from configuration and request.
 * array $currentIP
 *   Array containing all current IP addresses.
 * string $currentIPs
 *   Comma separated list of all current IP addresses.
 * array $deleteIP
 *   Effective IP(s) provided for the add/delete action (if necessary)
 * string $deleteIPs
 *   Effective IP(s) (serialized, comma separated)
 * string $host
 *   Effective host name as configured/requested.
 * array $updateIP
 *   Effective IP(s) provided for the add/update action (if necessary)
 * string $updateIPs
 *   Effective IP(s) (serialized, comma separated)
 * bool $needUpdate
 *   Whether an update was required (only for "update" action).
 * array $previousIP
 *   Array containing all IP addresses which existed before
 *   this request was performed.
 * string $previousIPs
 *   Comma separated list of all previous IP addresses.
 * string $remoteIP
 *   IP that the request came from.
 * bool $success
 *   Whether the requested action succeeded.
 * string $user
 *   User who performed the action.
 */

$user = empty($user) ? 'n/a' : $user;
$successText = isset($success) ? ($success ? "Successful" : "Failed") : 'no action';
$updateText = isset($needUpdate) ? ($needUpdate ? "Yes" : "No") : 'n/a';
$successClass = str_replace(' ', '-', strtolower($successText));

?><html class="dddns">
  <head>
  <style type="text/css">
    table, th, td {
      border: 2px solid black;
      border-collapse: collapse;
    }
    th, td {
      border-width: 1px;
      padding: .5em;
      text-align: left;
    }
    .successful {color: #090;}
    .failed {color: #900;}
    .no-action {color: #009;}
    .success td {
      font-weight: bold;
    }
  </style>
  <body>
    <h1>DDDNS: <?php print "$nameserver"; ?></h1>
    <h2><?php print "$action $host"; ?></h2>
    <div class="status">
      <table>
        <tr class="previous">
          <th>Previous IP(s)</th>
          <td><?php print $previousIPs; ?></td>
        </tr>
        <tr class="current">
          <th>Current IP(s)</th>
          <td><?php print $currentIPs; ?></td>
        </tr>
        <tr class="remote">
          <th>Remote IP</th>
          <td><?php print $remoteIP; ?></td>
        </tr>
        <?php if ($action == 'update'): ?>
          <tr class="effective">
            <th>IP(s) for update</th>
            <td><?php print $updateIPs; ?></td>
          </tr>
        <?php endif; ?>
        <tr class="user">
          <th>User</th>
          <td><?php print $user; ?></td>
        </tr>
        <tr class="need-update">
          <th>Update required?</th>
          <td><?php print $updateText; ?></td>
        </tr>
        <tr class="success">
          <th>Result</th>
          <td class="<?php print $successClass; ?>"><?php print $successText; ?></td>
        </tr>
      </table>
    </div>
  </body>
</html>
