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
   xid = sb_rand(1, 9000000000000000)..sb_rand_uniform(1, 9000000000000000)..table_name..sb_rand(1, 1000000000000000)..sb_rand_uniform(1, 9000000000000000)..sb_rand_uniq(1, 9000000000000000)
   db_query("xa start ".."'"..xid.."'") 
   for i=1, oltp_point_selects do
    --[[
       [db_query("xa start ".."'"..xid.."'")
       ]]
    --[[
       [rs = db_query("SELECT c FROM ".. table_name .." WHERE id=" .. sb_rand(1, oltp_table_size))
       [db_query("xa end ".."'"..xid.."'")       
       [db_query("xa prepare ".."'"..xid.."'")       
       [db_query("xa commit ".."'"..xid.."'")       
       ]]
       rs = db_query("SELECT c FROM ".. table_name .." WHERE id=" .. sb_rand(1, oltp_table_size))
   end
   db_query("xa end ".."'"..xid.."'")
   db_query("xa prepare ".."'"..xid.."'")
   db_query("xa commit ".."'"..xid.."'")

--[[
   [   xid = sb_rand(1, 9000000000000000)..sb_rand_uniform(1, 9000000000000000)..table_name..sb_rand(1000000000000000, 2000000000000000)..sb_rand_uniform(1, 9000000000000000)..sb_rand_uniq(1, 9000000000000000)
   [   db_query("xa start ".."'"..xid.."'")
   [   for i=1, oltp_simple_ranges do
   [       range_start = sb_rand(1, oltp_table_size)
   [       rs = db_query("SELECT c FROM ".. table_name .." WHERE id BETWEEN " .. range_start .. " AND " .. range_start .. "+" .. oltp_range_size - 1)
   [   end
   [   db_query("xa end ".."'"..xid.."'")
   [   db_query("xa prepare ".."'"..xid.."'")
   [   db_query("xa commit ".."'"..xid.."'")
   [
   [   xid = sb_rand(1, 9000000000000000)..sb_rand_uniform(1, 9000000000000000)..table_name..sb_rand(2000000000000000, 3000000000000000)..sb_rand_uniform(1, 9000000000000000)..sb_rand_uniq(1, 9000000000000000)
   [   db_query("xa start ".."'"..xid.."'") 
   [   for i=1, oltp_sum_ranges do
   [       range_start = sb_rand(1, oltp_table_size)
   [       rs = db_query("SELECT SUM(K) FROM ".. table_name .." WHERE id BETWEEN " .. range_start .. " AND " .. range_start .. "+" .. oltp_range_size - 1)
   [   end
   [   db_query("xa end ".."'"..xid.."'")
   [   db_query("xa prepare ".."'"..xid.."'")
   [   db_query("xa commit ".."'"..xid.."'")
   [
   [   xid = sb_rand(1, 9000000000000000)..sb_rand_uniform(1, 9000000000000000)..table_name..sb_rand(3000000000000000, 4000000000000000)..sb_rand_uniform(1, 9000000000000000)..sb_rand_uniq(1, 9000000000000000)
   [   db_query("xa start ".."'"..xid.."'") 
   [   for i=1, oltp_order_ranges do
   [       range_start = sb_rand(1, oltp_table_size)
   [       rs = db_query("SELECT c FROM ".. table_name .." WHERE id BETWEEN " .. range_start .. " AND " .. range_start .. "+" .. oltp_range_size - 1 .. " ORDER BY c")
   [   end
   [   db_query("xa end ".."'"..xid.."'")
   [   db_query("xa prepare ".."'"..xid.."'")
   [   db_query("xa commit ".."'"..xid.."'")
   ]]

   xid = sb_rand(1, 9000000000000000)..sb_rand_uniform(1, 9000000000000000)..table_name..sb_rand(4000000000000000, 5000000000000000)..sb_rand_uniform(1, 9000000000000000)..sb_rand_uniq(1, 9000000000000000)
   db_query("xa start ".."'"..xid.."'") 
   for i=1, oltp_distinct_ranges do
       range_start = sb_rand(1, oltp_table_size)
       rs = db_query("SELECT DISTINCT c FROM ".. table_name .." WHERE id BETWEEN " .. range_start .. " AND " .. range_start .. "+" .. oltp_range_size - 1 .. " ORDER BY c")
   end
   db_query("xa end ".."'"..xid.."'")
   db_query("xa prepare ".."'"..xid.."'")
   db_query("xa commit ".."'"..xid.."'")

   xid = sb_rand(1, 9000000000000000)..sb_rand_uniform(1, 9000000000000000)..table_name..sb_rand(5000000000000000, 6000000000000000)..sb_rand_uniform(1, 9000000000000000)..sb_rand_uniq(1, 9000000000000000)
   db_query("xa start ".."'"..xid.."'") 
   for i=1, oltp_index_updates do
       rs = db_query("UPDATE " .. table_name .. " SET k=k+1 WHERE id=" .. sb_rand(1, oltp_table_size))
   end
   db_query("xa end ".."'"..xid.."'")
   db_query("xa prepare ".."'"..xid.."'")
   db_query("xa commit ".."'"..xid.."'")

   xid = sb_rand(1, 9000000000000000)..sb_rand_uniform(1, 9000000000000000)..table_name..sb_rand(6000000000000000, 7000000000000000)..sb_rand_uniform(1, 9000000000000000)..sb_rand_uniq(1, 9000000000000000)
   db_query("xa start ".."'"..xid.."'")  
   for i=1, oltp_non_index_updates do
       c_val = sb_rand_str("###########-###########-###########-###########-###########-###########-###########-###########-###########-###########")
       query = "UPDATE " .. table_name .. " SET c='" .. c_val .. "' WHERE id=" .. sb_rand(1, oltp_table_size)
       rs = db_query(query)
       if rs then
           print(query)
       end
   end
   db_query("xa end ".."'"..xid.."'")
   db_query("xa prepare ".."'"..xid.."'")
   db_query("xa commit ".."'"..xid.."'")
   

   xid = sb_rand(1, 9000000000000000)..sb_rand_uniform(1, 9000000000000000)..table_name..sb_rand(7000000000000000, 8000000000000000)..sb_rand_uniform(1, 9000000000000000)..sb_rand_uniq(1, 9000000000000000)
   db_query("xa start ".."'"..xid.."'")  
   i = sb_rand(1, oltp_table_size)
   rs = db_query("DELETE FROM " .. table_name .. " WHERE id=" .. i)
   db_query("xa end ".."'"..xid.."'")
   db_query("xa prepare ".."'"..xid.."'")
   db_query("xa commit ".."'"..xid.."'")

   c_val = sb_rand_str([[
###########-###########-###########-###########-###########-###########-###########-###########-###########-###########]])
   pad_val = sb_rand_str([[
###########-###########-###########-###########-###########]])
   
   xid = sb_rand(1, 9000000000000000)..sb_rand_uniform(1, 9000000000000000)..table_name..sb_rand(8000000000000000, 9000000000000000)..sb_rand_uniform(1, 9000000000000000)..sb_rand_uniq(1, 9000000000000000)
   db_query("xa start ".."'"..xid.."'")  
   rs = db_query("replace INTO " .. table_name ..  " (id, k, c, pad) VALUES " .. string.format("(%d, %d, '%s', '%s')",i, sb_rand(1, oltp_table_size) , c_val, pad_val))
   db_query("xa end ".."'"..xid.."'")
   db_query("xa prepare ".."'"..xid.."'")
   db_query("xa commit ".."'"..xid.."'")
end

