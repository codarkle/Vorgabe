import ctypes
from wrappers import *

class Test_Cp:
    # successful mkdir operation on a fresh filesystem
    # * valid path
    # * valid name
    # Expected outcome:
    # * return 0
    # * first direct block of root node points to inode[1]
    # * inode[1] is set as a directory, name is set
    # * parent of new directory is set to root dir inode number
    def test_mkdir_cp_easy(self):
        fs = setup(5)
        retval = libc.fs_mkdir(ctypes.byref(fs), ctypes.c_char_p(bytes("/testDirectory","UTF-8")))
        assert retval == 0
        assert fs.inodes[1].name.decode("utf-8") =="testDirectory","UTF-8" 
        assert fs.inodes[1].n_type == 2 # meaning it is marked as directory
        assert fs.inodes[0].direct_blocks[0] == 1 #fs.inodes[0] is the root node. its first direct block should point to the 1st inode (where the new dir is located)
        assert fs.inodes[1].parent == 0
        
        retval = libc.fs_cp(ctypes.byref(fs), ctypes.c_char_p(bytes("/testDirectory","UTF-8")), ctypes.c_char_p(bytes("/testLocation","UTF-8")))
        
        assert retval == 0
        assert fs.inodes[2].name.decode("utf-8") =="testLocation","UTF-8" 
        assert fs.inodes[2].n_type == 2 # meaning it is marked as directory
        assert fs.inodes[0].direct_blocks[1] == 2 #fs.inodes[0] is the root node. its first direct block should point to the 1st inode (where the new dir is located)
        assert fs.inodes[2].parent == 0
        
    def test_mkfile_cp_easy(self):
        fs = setup(5)
        retval = libc.fs_mkfile(ctypes.byref(fs), ctypes.c_char_p(bytes("/testFile","UTF-8")))
        assert retval == 0
        assert fs.inodes[1].n_type == 1 # meaning it is marked as regular file
        assert fs.inodes[1].name.decode("utf-8") =="testFile" 
        assert fs.inodes[0].direct_blocks[0] == 1 #fs.inodes[0] is the root node. its first direct block should point to the 1st inode (where the new file is located)
        assert fs.inodes[1].parent == 0
        
        retval == libc.fs_cp(ctypes.byref(fs), ctypes.c_char_p(bytes("/testFile","UTF-8")), ctypes.c_char_p(bytes("/abc","UTF-8")));
        assert retval == 0
        assert fs.inodes[2].n_type == 1 # meaning it is marked as regular file
        assert fs.inodes[2].name.decode("utf-8") =="abc" 
        assert fs.inodes[0].direct_blocks[1] == 2 #fs.inodes[0] is the root node. its first direct block should point to the 1st inode (where the new file is located)
        assert fs.inodes[2].parent == 0
    
    def test_mkdir_cp_nested(self):
        fs = setup(5)
        retval = libc.fs_mkdir(ctypes.byref(fs), ctypes.c_char_p(bytes("/testDirectory","UTF-8")))
        assert retval == 0
        assert fs.inodes[1].name.decode("utf-8") =="testDirectory","UTF-8" 
        assert fs.inodes[1].n_type == 2 # meaning it is marked as directory
        assert fs.inodes[0].direct_blocks[0] == 1 #fs.inodes[0] is the root node. its first direct block should point to the 1st inode (where the new dir is located)
        assert fs.inodes[1].parent == 0
        
        retval = libc.fs_mkdir(ctypes.byref(fs), ctypes.c_char_p(bytes("/testDirectory/tt","UTF-8")))
        assert retval == 0
        assert fs.inodes[2].name.decode("utf-8") =="tt","UTF-8" 
        assert fs.inodes[2].n_type == 2 # meaning it is marked as directory
        assert fs.inodes[1].direct_blocks[0] == 2 #fs.inodes[0] is the root node. its first direct block should point to the 1st inode (where the new dir is located)
        assert fs.inodes[2].parent == 1
        
        retval = libc.fs_cp(ctypes.byref(fs), ctypes.c_char_p(bytes("/testDirectory","UTF-8")), ctypes.c_char_p(bytes("/testLocation","UTF-8")))
        
        assert retval == 0
        assert fs.inodes[3].name.decode("utf-8") =="testLocation","UTF-8" 
        assert fs.inodes[3].n_type == 2 # meaning it is marked as directory
        assert fs.inodes[0].direct_blocks[1] == 3 #fs.inodes[0] is the root node. its first direct block should point to the 1st inode (where the new dir is located)
        assert fs.inodes[3].parent == 0
        
        assert fs.inodes[4].name.decode("utf-8") =="tt","UTF-8" 
        assert fs.inodes[4].n_type == 2 # meaning it is marked as directory
        assert fs.inodes[3].direct_blocks[0] == 4 #fs.inodes[0] is the root node. its first direct block should point to the 1st inode (where the new dir is located)
        assert fs.inodes[4].parent == 3
