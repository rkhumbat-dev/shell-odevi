echo "Test1"
ls | wc -l
echo "Test2"
echo hello > a.txt
cat < a.txt | sort > sorted.txt
cat sorted.txt
echo "Test3"
ls -l | grep hw1 | wc -l
echo "Test4"
sleep 5 &
echo "Test5"
ls olmayan 2> err.txt
echo "err.txt:"
cat err.txt



