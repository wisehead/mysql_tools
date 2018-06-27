pathtest = string.match(test, "(.*/)") or ""

dofile(pathtest .. "common.lua")

function uuid()        
    local seed = {'0','1','2','3','4','5','6','7','8','9',"a","b","c","d","e","f"}
    local tb={}  
    for i=1,32 do
        table.insert(tb, seed[math.random(1,16)])
    end
    local sid = table.concat(tb)
    return string.format('%s%s%s%s%s',
        string.sub(sid, 1, 8), 
        string.sub(sid, 9, 12),
        string.sub(sid, 13, 16),
        string.sub(sid, 17, 20),
        string.sub(sid, 21, 32) 
    )
end
function thread_init(thread_id)
    set_vars()
    db_connect()

end

function event(thread_id)
   local rs
   local i
   local table_name
   local range_start
   local c_val
   local pad_val
   local query
   local xid
   table_name = "sbtest".. sb_rand_uniform(1, oltp_tables_count)
   c_val = sb_rand_str([[
###########-###########-###########-###########-###########-###########-###########-###########-###########-###########]])
   pad_val = sb_rand_str([[
###########-###########-###########-###########-###########]])
   xid = sb_rand(1, 9000000000000000)..sb_rand_uniform(1, 9000000000000000)..table_name..sb_rand(8000000000000000, 9000000000000000)..sb_rand_uniform(1, 9000000000000000)..sb_rand_uniq(1, 9000000000000000)
   db_query("xa start ".."'"..xid.."'")  
   rs = db_query("INSERT INTO " .. table_name ..  " (id, k, c, pad) VALUES " .. string.format("(NULL, %d, '%s', '%s')", sb_rand(1, oltp_table_size) , c_val, pad_val))
   db_query("xa end ".."'"..xid.."'")
   db_query("xa prepare ".."'"..xid.."'")
   db_query("xa commit ".."'"..xid.."'")
end

