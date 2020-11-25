# PTRACE-Debugger


##### Linux only debugger using PTRACE
This is a small work to make a custom debugger in linux. 
I saw some resources for this , but nothing completed or in easily working conditions. 

Run the debugger like ./debugger 
when prompted for executable please provide full path.

#### To Compile
```
g++ -m32 debugger.cpp -o debugger
```

##### To Run
```
./ debugger <FULL PATH TO BINARY> 

```

##### Commands Available
```
######### Commands ##############
next              : to step 
break 0x12345678  : to set break point at 0x12345678
continue          : continue or c may be used
exit              : exit the program
infobreak         : List break points
examined          : Video DWORD at location 



########################################

```
