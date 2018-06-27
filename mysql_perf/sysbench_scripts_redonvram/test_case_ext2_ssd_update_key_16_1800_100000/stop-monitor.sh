
CASENAME=$1
DATA_COLLECT_FOR_THIS_CASE=$2

TMP_FILENAME=`ls ${DATA_COLLECT_FOR_THIS_CASE}/${CASENAME}-*`
TMP_FILENAME=`basename ${TMP_FILENAME}`
FILENAME=${TMP_FILENAME}_`date "+%Y%m%d-%H%M%S"`.nmon

echo "stop nmon monitor for case:${CASENAME}. output file:${FILENAME}"
kill `ps -ef | grep nmon | grep ${CASENAME} | awk '{print $2}'` 
cd ${DATA_COLLECT_FOR_THIS_CASE} && mv ${TMP_FILENAME} ${FILENAME}


