#!/bin/sh


TOP=$(cd `dirname $0`;pwd)


function usage(){
	cat <<EOT
This shell scripts will monitor system status and call gnuplot to make charts
Options:
	-h
	-t <run time>
	-s <sample period>
	-P <mysql port>
	-u <mysql user>
	-p <mysql password>
EOT
}

main(){
	while getopts "ht:s:P:u:p:m:" opt;do
		case $opt in
		h)
			usage
			exit 1
			;;
		t)
			duration=$OPTARG
			;;
		s)
			sample=$OPTARG
			;;
		P)
			port=$OPTARG
			;;
		u)
			user=$OPTARG
			;;
		p)
			passwd=$OPTARG
			;;
		m)
			mark=$OPTARG
			;;
		*)
			usage 
			exit 1
			;;
		esac
	done

	if [ "$port" = "" ];then
		usage
		exit 1
	fi

	host="127.0.0.1"
	
	if [ "$duration" =  "" ];then
		duration=100
	fi

	if [ "$sample" = ""  ];then
		sample=5
	fi
	
	if [ "$user" = "" ];then
		user="root"
	fi
	if [ "$mark" = "" ];then
		time_tag=$(date +%m%d_%H_%M_%S)
		mark=${port}_$time_tag
	fi

	#echo "TOP=$TOP sample=$sample host=$host port=$port user=$user password=$password"

	sh $TOP/perf/perf.sh start $sample $host $port $user "$passwd" 		

	sleep $duration

	sh $TOP/perf/perf.sh stop

	cd $TOP/plt	

	sh make_charts.sh -s $sample -i $TOP/result -o $TOP/result	

	cd $TOP

	rm -rf ${port}_${host}.tar.gz	

	tar -czf $mark.tar.gz ./result		
}

main $@
