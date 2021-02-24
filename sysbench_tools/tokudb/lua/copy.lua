pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function copydata(table_id)
    local query

    query = [[create table sbtest]] .. table_id .. [[(
    id serial not null,
    k interger,
    c char(120) default '' not null,
    pad char(60) default '' not null,
    primary key(id)
    )]]

    db_query(query)
end

function thread_init(thread_id)
   set_vars()
   print("thread prepare"..thread_id)
   for i=1, oltp_tables_count, num_threads do
       copydata(i)
   end
end

function event(thread_id)
    os.exit()
end
