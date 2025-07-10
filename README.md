# Note. updated "ha2_new.c" is better to test.
ren ha2.c ha2_old.c
ren ha2.c_new ha2.c

# test 
make test

# export image by SysProgFiles.fs
make
./build/ha2 -l SysProgFile.fs
list /
list /pictures
export /pictures/success.jpg ext_success.jpg
exit

# create MyProgFiles.fs  
./build/ha2 -c MyFiles.fs 50

# create directory and files
mkdir /docs
mkdir /pics
mkfile /readme.txt
list /

# write(append) and read
writef /readme.txt Good
readf /readme.txt
writef /readme.txt Luck!
readf /readme.txt

# copy and remove
cp /readme.txt /docs/readme2.txt
list /docs
rm /readme.txt
list /
mkdir /docs/dir1

# image import, copy, export
import /docs/dir1/imp_photo.jpg ext_success.jpg
cp /docs /pics/docs_backup
list /pics/docs_backup
export /pics/docs_backup/dir1/img_photo.jpg same_photo.jpg

# directory remove
rm /docs

# dump in device
dump
exit

# load it from device
./build/ha2 -l MyFiles.fs
list /pics

# test wrong inputs
mkdir /pics
mkfile /wrongdir/wrongfile
list /pics/wrongdir
cp /wrongdir /wrongdir2
readf /pics
writef /wrongdir/wrongfile randomtext
rm /wrongdir
export /wrongdir/wrongfile anyfile
import /wrongdir/wrongfile anyfile
exit

# (operations.c => submission.zip)
make pack
