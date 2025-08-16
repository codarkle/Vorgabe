These situations are also considered.
- (fs_mkdir, fs_mkfile, fs_cp, fs_import, fs_writef) should be returned memory overflow(inode & block size limit) error.(value:-2)
- fs_mkdir should be returned -2 when the directory already exists.
- fs_rm should be returned -1 when the path is "/" because root shouldn't be removed.
- In fs_import, when there is already exist file which has data blocks, those data blocks should be cleared. (because append:[after i import picturefile, that could be broken] so I think it should be overwritten.)
What do you think?