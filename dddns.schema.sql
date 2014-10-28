-- phpMyAdmin SQL Dump
-- version 3.4.10.1deb1
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Erstellungszeit: 19. Jul 2013 um 23:34
-- Server Version: 5.5.31
-- PHP-Version: 5.3.10-1ubuntu3.6

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Datenbank: `dddns`
--

-- --------------------------------------------------------

--
-- Tabellenstruktur für Tabelle `dddns`
--

CREATE TABLE IF NOT EXISTS `dddns` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `ip` tinytext NOT NULL,
  `time` bigint(20) unsigned NOT NULL,
  `dns_success` tinyint(4) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=636 ;

-- --------------------------------------------------------

--
-- Tabellenstruktur für Tabelle `dddns_vars`
--

CREATE TABLE IF NOT EXISTS `dddns_vars` (
  `name` varchar(256) NOT NULL,
  `value` text,
  PRIMARY KEY (`name`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Stellvertreter-Struktur des Views `last_update`
--
CREATE TABLE IF NOT EXISTS `last_update` (
`ip` tinytext
,`FROM_UNIXTIME(time)` datetime
,`dns_success` tinyint(4)
);
-- --------------------------------------------------------

--
-- Struktur des Views `last_update`
--
DROP TABLE IF EXISTS `last_update`;

CREATE ALGORITHM=UNDEFINED DEFINER=`meister`@`%` SQL SECURITY DEFINER VIEW `last_update` AS select `ip` AS `ip`,from_unixtime(`time`) AS `FROM_UNIXTIME(time)`,`dns_success` AS `dns_success` from `dddns` order by `time` desc;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
