pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function copydata(table_id)
    local query
    db_query(query)
end

function thread_init(thread_id)
    local i
    local index_name
    local path
    set_vars()
    if (oltp_secondary) then
        index_name = "KEY xid"
    else
        index_name = "PRIMARY KEY"
    end
   print("thread prepare"..thread_id)
   for i=thread_id+1, oltp_tables_count, num_threads  do
        query = [[create table sbtest]] .. i .. [[(
        id INTEGER UNSIGNED NOT NULL ]] ..
        ((oltp_auto_inc and "AUTO_INCREMENT") or "") .. [[,
        k INTEGER UNSIGNED DEFAULT '0' NOT NULL,
        c CHAR(120) DEFAULT '' NOT NULL,
        pad CHAR(60) DEFAULT '' NOT NULL,
        ]] .. index_name .. [[ (id)
        ) /*! ENGINE = ]] .. mysql_table_engine ..
        " MAX_ROWS = " .. myisam_max_rows .. " */"
        db_query(query) 
        os.execute("rm -rf sbtest" .. i .. '.dat')
        os.execute('./gendata ' .. oltp_table_size .. ' > sbtest'..i..'.dat &')
        os.execute('mv ' .. 'sbtest'..i..'.dat /home/mysql/work/rds/var/sbtest')
        os.execute('/home/mysql/work/rds/bin/mysql --defaults-file=/home/mysql/work/rds/etc/user.root.cnf '..
        '-e '.."'use sbtest;load data infile \"" .. 'sbtest'..i..'.dat' ..'\" replace into table ' .. "sbtest" ..i.. " fields terminated by \",\"'")
    end
end

function event(thread_id)
    --os.exit()
end
