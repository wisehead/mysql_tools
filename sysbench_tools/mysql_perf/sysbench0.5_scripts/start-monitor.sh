
CASENAME=$1
DATA_COLLECT_FOR_THIS_CASE=$2

echo "start nmon monitor for case:$CASENAME"
kill `ps -ef | grep nmon | grep ${CASENAME} | awk '{print $2}'` >> /dev/null 2>&1
#NMON=/home/mabo06/bin/nmon
NMON="nmon"
TMP_FILENAME=${DATA_COLLECT_FOR_THIS_CASE}/${CASENAME}-`date "+%Y%m%d-%H%M%S"`
${NMON} -F ${TMP_FILENAME} -t -r ${CASENAME} -s 2 -c 3600
