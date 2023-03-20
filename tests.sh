clear

do_I_make=true
do_I_clean=true
do_I_run_suite=false
do_I_run_test=false

suite=0
test=0
iterations=1

while getopts ":mcs:t:n:" option; do
   case $option in
      m) # do I make
         do_I_make=false;;
      c) # do I clean and prepare
         do_I_clean=false;;
      s) # run suite
         do_I_run_suite=true
         suite="${OPTARG}";;
      t) # run single test
         do_I_run_test=true
         test="${OPTARG}";;
      n) # number of iterations
         iterations="${OPTARG}";;
   esac
done


if $do_I_make ; then
    make -C "./shell"
    echo ""
fi

if $do_I_clean ; then
    (cd ./tests  ; ./clean.sh)
    echo ""
    (cd ./tests  ; ./prepare.sh)
    echo ""
fi

result=""

for i in $(seq $iterations); do
   # Run all tests
   if [[ ${do_I_run_suite} == false && ${do_I_run_test} == false ]]
   then
      result+=`cd ./tests ; ./run_all.sh ./../shell/bin/mshell | tee /dev/tty`
   fi

   # Run suite
   if [[ ${do_I_run_suite} == true && ${do_I_run_test} == false ]]
   then
      result+=`cd ./tests ; ./run_suite.sh ./../shell/bin/mshell ${suite} | tee /dev/tty`
   fi

   # Run single test
   if [[ ${do_I_run_suite} == true && ${do_I_run_test} == true ]]
   then
      result+=`cd ./tests ; ./run_one.sh ./../shell/bin/mshell ${suite} ${test} | tee /dev/tty`
   fi
done

echo ""
passed=$(echo ${result} | grep -o 'OK' | wc -l)
failed=$(echo ${result} | grep -o 'FAIL' | wc -l)
echo "Passed: ${passed}"
echo "Failed: ${failed}"

# TODO: implement own version of all tests / suite tests