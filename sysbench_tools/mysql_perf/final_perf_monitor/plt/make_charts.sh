#!/bin/bash

#
#   2012 0503  wangjianying@baidu.com
#

TOP=$(cd `dirname $0`;pwd)

usage() {
    cat <<EOF
This shell script will loop all files with suffix 'plt'and call gnuplot to make charts
Options: 
       -h 
       -s <sample-interval>
       -i <input-directory>
       -o <output-directory>
EOF
}

parseArgs() {
    while getopts "hs:i:o:m:" opt; do
    case $opt in
        h)
        usage
        exit 1
        ;;
        s)
        sample=$OPTARG
        ;;
        i)
        inputdir=$OPTARG
        ;;
        o)
        outputdir=$OPTARG
    esac    
    done
}

main() {
    for file in `ls $cur/*.plt`
    do  
        cmd="sed -i "s/sample=.*/sample=$sample/" $file"
        $cmd

        #$TOP/gnuplot $file >$TOP/chart.log 2>&1
        gnuplot $file >$TOP/chart.log 2>&1
        echo -n draw $file|sed 's/\..*//' >>$TOP/chart.log 2>&1
        echo " finished!" >>$TOP/chart.log 2>&1
    done
    rm -f $cur/*.out

    if [ ! -d $outputdir ];then
    	mkdir -p $outputdir
    fi

    if [ $cur != $outputdir ]; then
        mv -f $cur/*.png  $outputdir/
    fi    
}

pre_process() {
    ln -s $inputdir/*.out $cur 
    sh cpu_adaptor.sh
}

sample=10
cur=$(cd $(dirname $0);pwd) 
inputdir=$(cd $(dirname $0);pwd) 
outputdir=$(cd "$(dirname $0)";pwd) 
parseArgs $@
pre_process

main
