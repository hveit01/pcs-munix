:
#ident  @(#) /usr/sys/kernel.do
#       shellscript to help testing a new kernel 05-Sep-86

PATH=$PATH:/etc
echo
echo "T E S T I N G  of a new kernel: "
echo
echo "If you have a  W O R K I N G  /unix save it!"
echo
echo 'Do you want to save your working /unix in /oldunix (y,n)? \c'
read YN
case $YN in
    y|Y) echo "mv /unix /oldunix"
	 mv /unix /oldunix;;
      *) ;;
esac

echo
echo 'Are you in Single User Mode on the console (y,n)? \c'
read YN
case $YN in
    y|Y)
	 echo
	 echo "mv /newunix /unix"
	 mv /newunix /unix
	 echo "Autoboot of /unix will start now ! "
	 echo
	 sync
	 sleep 5
	 uadmin 2 1
	 ;;
      *) echo
	 echo "mv /newunix /unix"
	 mv /newunix /unix
	 echo '\nThe system will now be brought into Single User Mode'
	 echo
	 cd /
	 shutdown
	 ;;
esac
