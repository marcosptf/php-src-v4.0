<?php
//
// +----------------------------------------------------------------------+
// | PHP version 4.0                                                      |
// +----------------------------------------------------------------------+
// | Copyright (c) 1997-2001 The PHP Group                                |
// +----------------------------------------------------------------------+
// | This source file is subject to version 2.02 of the PHP license,      |
// | that is bundled with this package in the file LICENSE, and is        |
// | available at through the world-wide-web at                           |
// | http://www.php.net/license/2_02.txt.                                 |
// | If you did not receive a copy of the PHP license and are unable to   |
// | obtain it through the world-wide-web, please send a note to          |
// | license@php.net so we can mail you a copy immediately.               |
// +----------------------------------------------------------------------+
// | Authors: Sterling Hughes <sterling@php.net>                          |
// +----------------------------------------------------------------------+
//
// $Id: mssql.php,v 1.24.2.1 2001/08/28 11:41:00 ssb Exp $
//
// Database independent query interface definition for PHP's Microsoft SQL Server
// extension.
//

require_once 'DB/common.php';

class DB_mssql extends DB_common
{
    var $connection;
    var $phptype, $dbsyntax;
    var $prepare_tokens = array();
    var $prepare_types = array();
    var $transaction_opcount = 0;
    var $autocommit = true;

    function DB_mssql()
    {
        $this->DB_common();
        $this->phptype = 'mssql';
        $this->dbsyntax = 'mssql';
        $this->features = array(
            'prepare' => false,
            'pconnect' => true,
            'transactions' => true
        );
        // XXX Add here error codes ie: 'S100E' => DB_ERROR_SYNTAX
        $this->errorcode_map = array(

        );
    }

    function connect($dsninfo, $persistent = false)
    {
        $this->dsn = $dsninfo;
        $user = $dsninfo['username'];
        $pw = $dsninfo['password'];
        $dbhost = $dsninfo['hostspec'] ? $dsninfo['hostspec'] : 'localhost';
        $connect_function = $persistent ? 'mssql_pconnect' : 'mssql_connect';
        if ($dbhost && $user && $pw) {
            $conn = @$connect_function($dbhost, $user, $pw);
        } elseif ($dbhost && $user) {
            $conn = @$connect_function($dbhost, $user);
        } else {
            $conn = @$connect_function($dbhost);
        }
        if (!$conn) {
            return $this->raiseError(DB_ERROR_CONNECT_FAILED, null, null,
                                         null, mssql_get_last_message());
        }
        if ($dsninfo['database']) {
            if (!@mssql_select_db($dsninfo['database'], $conn)) {
                return $this->raiseError(DB_ERROR_NODBSELECTED, null, null,
                                         null, mssql_get_last_message());
            }
        }
        $this->connection = $conn;
        return DB_OK;
    }

    function disconnect()
    {
        $ret = @mssql_close($this->connection);
        $this->connection = null;
        return $ret;
    }

    function simpleQuery($query)
    {
        $ismanip = DB::isManip($query);
        $this->last_query = $query;
        $query = $this->modifyQuery($query);
        if (!$this->autocommit && $ismanip) {
            if ($this->transaction_opcount == 0) {
                $result = @mssql_query('BEGIN TRAN', $this->connection);
                if (!$result) {
                    return $this->mssqlRaiseError();
                }
            }
            $this->transaction_opcount++;
        }
        $result = @mssql_query($query, $this->connection);
        if (!$result) {
            return $this->mssqlRaiseError();
        }
        // Determine which queries that should return data, and which
        // should return an error code only.
        return $ismanip ? DB_OK : $result;
    }

    function &fetchRow($result, $fetchmode = DB_FETCHMODE_DEFAULT, $rownum=null)
    {
        if ($fetchmode == DB_FETCHMODE_DEFAULT) {
            $fetchmode = $this->fetchmode;
        }
        $res = $this->fetchInto ($result, $arr, $fetchmode, $rownum);
        if ($res !== DB_OK) {
            return $res;
        }
        return $arr;
    }

    function fetchInto($result, &$ar, $fetchmode, $rownum=null)
    {
        if ($rownum !== null) {
            if (!@mssql_data_seek($result, $rownum)) {
                return null;
            }
        }
        if ($fetchmode & DB_FETCHMODE_ASSOC) {
            $ar = @mssql_fetch_array($result);
        } else {
            $ar = @mssql_fetch_row($result);
        }
        if (!$ar) {
            if ($msg = mssql_get_last_message()) {
                return $this->raiseError($msg);
            } else {
                return null;
            }
        }
        return DB_OK;
    }

    function freeResult($result)
    {
        if (is_resource($result)) {
            return @mssql_free_result($result);
        }
        if (!isset($this->prepare_tokens[$result])) {
            return false;
        }
        unset($this->prepare_tokens[$result]);
        unset($this->prepare_types[$result]);
        return true;
    }

    function numCols($result)
    {
        $cols = @mssql_num_fields($result);
        if (!$cols) {
            return $this->mssqlRaiseError();
        }
        return $cols;
    }

    function numRows($result)
    {
        $rows = @mssql_num_rows($result);
        if (!$rows) {
            return $this->mssqlRaiseError();
        }
        return $rows;
    }

    /**
     * Enable/disable automatic commits
     */
    function autoCommit($onoff = false)
    {
        // XXX if $this->transaction_opcount > 0, we should probably
        // issue a warning here.
        $this->autocommit = $onoff ? true : false;
        return DB_OK;
    }

    // }}}
    // {{{ commit()

    /**
     * Commit the current transaction.
     */
    function commit()
    {
        if ($this->transaction_opcount > 0) {
            $result = @mssql_query('COMMIT TRAN', $this->connection);
            $this->transaction_opcount = 0;
            if (!$result) {
                return $this->mssqlRaiseError();
            }
        }
        return DB_OK;
    }

    // }}}
    // {{{ rollback()

    /**
     * Roll back (undo) the current transaction.
     */
    function rollback()
    {
        if ($this->transaction_opcount > 0) {
            $result = @mssql_query('ROLLBACK TRAN', $this->connection);
            $this->transaction_opcount = 0;
            if (!$result) {
                return $this->mssqlRaiseError();
            }
        }
        return DB_OK;
    }

    // }}}
    // {{{ affectedRows()

    /**
     * Gets the number of rows affected by the last query.
     * if the last query was a select, returns 0.
     *
     * @return number of rows affected by the last query or DB_ERROR
     */
    function affectedRows()
    {
        if (DB::isManip($this->last_query)) {
            $res = @mssql_query('select @@rowcount', $this->connection);
            if (!$res) {
                return $this->mssqlRaiseError();
            }
            $ar = @mssql_fetch_row($res);
            if (!$ar) {
                $result = 0;
            } else {
                @mssql_free_result($res);
                $result = $ar[0];
            }
        } else {
            $result = 0;
        }
        return $result;
    }
    // {{{ nextId()

    /**
     * Get the next value in a sequence.  We emulate sequences
     * for MSSQL.  Will create the sequence if it does not exist.
     *
     * @access public
     *
     * @param $seq_name the name of the sequence
     *
     * @param $ondemand whether to create the sequence table on demand
     * (default is true)
     *
     * @return a sequence integer, or a DB error
     */
    function nextId($seq_name, $ondemand = true)
    {
        $sqn = preg_replace('/[^a-z0-9_]/i', '_', $seq_name);
        $repeat = 0;
        do {
            $this->pushErrorHandling(PEAR_ERROR_RETURN);
            $result = $this->query("INSERT INTO ${sqn}_seq (vapor) VALUES (0)");
            $this->popErrorHandling();
            if ($ondemand && DB::isError($result) &&
                ($result->getCode() == DB_ERROR || $result->getCode() == DB_ERROR_NOSUCHTABLE))
            {
                $repeat = 1;
                $result = $this->createSequence($seq_name);
                if (DB::isError($result)) {
                    return $result;
                }
            } else {
                $result = $this->query("SELECT @@IDENTITY FROM ${sqn}_seq");
                $repeat = 0;
            }
        } while ($repeat);
        if (DB::isError($result)) {
            return $this->raiseError($result);
        }
        $result = $result->fetchRow(DB_FETCHMODE_ORDERED);
        return $result[0];
    }

    // }}}
    // {{{ createSequence()

    function createSequence($seq_name)
    {
        $sqn = preg_replace('/[^a-z0-9_]/i', '_', $seq_name);
        return $this->query("CREATE TABLE ${sqn}_seq".
                            '([id] [int] IDENTITY (1, 1) NOT NULL ,' .
                            '[vapor] [int] NULL)');
    }
    // }}}
    // {{{ dropSequence()

    function dropSequence($seq_name)
    {
        $sqn = preg_replace('/[^a-z0-9_]/i', '_', $seq_name);
        return $this->query("DROP TABLE ${sqn}_seq");
    }
    // }}}

    function errorCode()
    {
        $this->pushErrorHandling(PEAR_ERROR_RETURN);
        $error_code = $this->getOne('select @@ERROR as ErrorCode');
        $this->popErrorHandling();
        // XXX Debug
        if (!isset($this->errorcode_map[$error_code])) {
            trigger_error("Error code: '$error_code' is not defined, please add it to \$this->errorcode_map", E_USER_WARNING);
        }
        return $error_code;
    }

    function mssqlRaiseError($code = null)
    {
        if ($code !== null) {
            $code = $this->errorCode();
            if (DB::isError($code)) {
                return $this->raiseError($code);
            }
        }
        return $this->raiseError($code, null, null, null,
                        mssql_get_last_message());
    }
}

?>