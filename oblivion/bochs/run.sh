cd ../../sartoris; make; cd ../oblivion; make; cd bochs; echo c > comm_test; "/cygdrive/c/Program Files (x86)/Bochs-2.4.5/bochsdbg.exe" -rc comm_test; rm -f comm_test