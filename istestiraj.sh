rm test*.*
for test in test*
do
	echo $test
	./haskell_to_llvm < $test > $test.ll &&
		clang -S $test.ll &&
		clang -o main main.c $test.s -lm &&
		./main
	echo "=============================================================================="
done
