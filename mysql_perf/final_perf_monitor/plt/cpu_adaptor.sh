#!/bin/sh

dbug() {
    echo "" > /dev/null
}

get_cpu_enum() {
    CPU_ENUM=(`grep "^[0-9]" mpstat.out | grep -v "CPU" | grep -v "all" | awk '{print $3}' | sort -n | uniq`)
}

 set_cpu_info() {
    local col_index=1

    N_S_FORMAT="%s "
    N_ARR_FORMAT=
    N_USING_FORMAT=
    
    for ((i=0; i<${#CPU_ENUM[@]}; i++))
    do
        if [ -n "${N_USING_FORMAT}" ];then
            N_USING_FORMAT="${N_USING_FORMAT} , ''  "
        fi

        N_S_FORMAT="${N_S_FORMAT} %s"
        N_ARR_FORMAT="${N_ARR_FORMAT} ,arr[i,$i]"
        let col_index+=1
        N_USING_FORMAT="${N_USING_FORMAT} using (funx(\$1)):${col_index} lw 2 title \"cpu${CPU_ENUM[$i]}\""
    done
}

generate_common_head_part() {
    local png_name=$1
    local ylbl=$2
    local png_name_prefix=`echo $png_name `

cat >common.plt.head << EOF 
    set terminal png truecolor
    set output "${png_name_prefix}.png"
    set autoscale
    set grid
    set title "${png_name}"
    set xlabel "ITime(sec)"
    set ylabel "$ylbl"
    set style data lines
    sample=10
    funx(x)=(x * sample)
EOF
}


generate_plt_script() {
    local name="$1"

    local using_content_str="${N_USING_FORMAT}"
    local arr_content_str="${N_ARR_FORMAT}"
    local n_mode=${#CPU_ENUM[@]}
    local s_content_str="${N_S_FORMAT}"
    local ylbl="{%user+%sys}"

    generate_common_head_part $name $ylbl
    
cat >${name}.plt.tail <<EOF
    plot "< egrep ^[0-9] mpstat.out | grep -v CPU | grep -v all | awk 'BEGIN{i=0} { arr[(i - i%${n_mode})/${n_mode}, i % ${n_mode}] = \$4 + \$6; i++;} \\
    END{ num = NR/${n_mode}; for(i=0; i<num; i++) \\
    printf(\"${s_content_str}\\\n\\", \\
    i${arr_content_str});}' " \\
    ${using_content_str}
EOF

    cat common.plt.head > ${name}.plt.new
    cat ${name}.plt.tail >> ${name}.plt.new
    rm -rf common.plt.head ${name}.plt.tail
    mv  ${name}.plt.new ${name}.plt 
}


get_cpu_enum
set_cpu_info


generate_plt_script "CPU_usage_across_cores" 
   
