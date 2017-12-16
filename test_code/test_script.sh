ulimit -n 12000

./sleeping

retValue=$?

echo $retValue

if [ $retValue -ne 10 ];
then
    echo "Un-Equal"
fi
