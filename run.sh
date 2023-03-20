clear
make -C ./shell
echo ""

while getopts ":i" option; do
   case $option in
      i) # display Help
         cd tests; ./../shell/bin/mshell < ./suites/4/input/4.in
         exit;;
   esac
done

./shell/bin/mshell