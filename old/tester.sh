
# for value in {1..30}
# do
# echo $value
# ./a.out 0
# done

for value in {1..30}
do
echo $value
./a.out 1 100
done
cat ./"particle_perf.txt"