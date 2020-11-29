#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>      // std::istringstream

#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/user.h>   
#include <sys/reg.h>
#include <sys/personality.h>


#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <syscall.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/reg.h>
#include <sys/user.h>
#include <unistd.h>
#include <errno.h>
#include <vector>
#include <map>  


/* 
break point probably should be here 0x565554c0
 0x5655552c
 0x56555527

Linux only debugger using PTRACE

Run the debugger like ./debugger 
when prompted for executable please provide full path.

######### Commands ##############
next              : to step 
break 0x12345678  : to set break point at 0x12345678
continue          : continue or c may be used
exit              : exit the program
infobreak         : List break points
examined          : Video DWORD at location
info registers    : show registers  



########################################
*/

//std::vector< unsigned int  > BreakPoints; 
std::map < unsigned int, unsigned int > BreakPoints;


void InfoRegisters( pid_t child )
{
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, child, 0, &regs);

/*
eax            0x0                 0x0
ecx            0x0                 0x0
edx            0x0                 0x0
ebx            0x0                 0x0
esp            0xffffd080          0xffffd080
ebp            0x0                 0x0
esi            0x0                 0x0
edi            0x0                 0x0
eip            0xf7fd6c70          0xf7fd6c70
eflags         0x200               [ IF ]
cs             0x23                0x23
ss             0x2b                0x2b
ds             0x2b                0x2b
es             0x2b                0x2b
fs             0x0                 0x0
gs             0x0                 0x0

*/
printf("######### Listing  Registers #########\n");

    printf(" EAX:  0x%08x\n ECX:  0x%08x\n EDX:  0x%08x\n EBX:  0x%08x\n ESP:  0x%08x\n EBP:  0x%08x\n ESI:  0x%08x\n EDI:  0x%08x\n EIP:  0x%08x\n"
    , regs.eax,regs.ecx,regs.edx,regs.ebx,regs.esp,regs.ebp,regs.esi,regs.edi,regs.eip ); // exterend here is you want more registers.

    printf("######### Finished  Registers #########\n");

}





void CheckDWORDMem(pid_t child, unsigned int addr )
{
    //unsigned addr = 0x8048096;
    unsigned data = ptrace(PTRACE_PEEKTEXT, child, (void*)addr, 0);
    printf("[+] Checking BP Location 0x%08x: 0x%08x\n", addr, data);


}


void ListBreakPoints()
{
  // Printing values in the map
  std::cout << std::endl << "######### Listing  BreakPoints #########" << std::endl;  
  for( std::map< unsigned int, unsigned int >::iterator iter=BreakPoints.begin(); iter!=BreakPoints.end(); ++iter)  
  {  
    //std::cout << (*iter).first << ": " << (*iter).second << std::endl;  
    printf("[+] Listing BPs 0x%08x: 0x%08x\n", iter->first, iter->second);
  }  

  std::cout << std::endl << "######### Finished BreakPoints #########\n\n" << std::endl;  

}


void SetBreakPoint(pid_t child, unsigned int addr)
{
    /* Write the trap instruction 'int 3' into the address */
    unsigned data = ptrace(PTRACE_PEEKTEXT, child, (void*)addr, 0);
    unsigned data_with_trap = (data & 0xFFFFFF00) | 0xCC;
    ptrace(PTRACE_POKETEXT, child, (void*)addr, (void*)data_with_trap);
    printf("[+] Setting BP 0x%08x: 0x%08x\n", addr, data);

    BreakPoints.insert ( std::pair< unsigned int, unsigned int>(addr,data) );
}


void execute_debugee (pid_t child, const std::string& prog_name) 
{
    personality(ADDR_NO_RANDOMIZE); // Disable ALSR

    if (ptrace(PTRACE_TRACEME, 0, 0, 0) < 0) {
        std::cerr << "[!] Error in PTRACE_TRACEME\n";
        return;
    }
    execl(prog_name.c_str(), prog_name.c_str(), nullptr);
}


void Nexti(pid_t child)
{
    int wait_status;
    
    /* Make the child execute another instruction */
    if (ptrace(PTRACE_SINGLESTEP, child, 0, 0) < 0) {
        perror("[!] Error in PTRACE_SINGLESTEP");
        return;
    }
    wait(&wait_status);

}


void Continue ( pid_t child )
{
    int wait_status;
    struct user_regs_struct regs;


    ptrace(PTRACE_CONT, child, 0, 0);
    wait(&wait_status);
    if (WIFSTOPPED(wait_status)) {
        //printf("Child got a signal: %d\n", (WSTOPSIG(wait_status))  );
        if(  (WSTOPSIG(wait_status))  == 5 )
        {
            ptrace(PTRACE_GETREGS, child, 0, &regs);
            printf("[!] BreakPoint Hit at 0x%08x \n", regs.eip );


            // Restore data at breakpoint
            std::map<unsigned int , unsigned int>::iterator iter = BreakPoints.find(regs.eip - 1);
            if (iter != BreakPoints.end())
            {
            //std::cout << std::endl<< "[+] Value  => " << BreakPoints.find(regs.eip -= 1)->second << '\n';
            printf("[+] Address 0x%08x: Data 0x%08x\n", iter->first, iter->second);
            }

            unsigned int addr = BreakPoints.find(regs.eip - 1)->first;
            unsigned int data = BreakPoints.find(regs.eip - 1)->second;

            ptrace(PTRACE_POKETEXT, child, (void*)addr, (void*)data);
            regs.eip -= 1;
            ptrace(PTRACE_SETREGS, child, 0, &regs);

            // End restore data


        }
    }
    else {
        perror("[-] wait");
        return;
}

    printf("[+] Child stopped at EIP = 0x%08x\n", regs.eip);

}




std::string lastcommand;

void HandleCommand(  pid_t child , std::string command  )
{


// printf("COMMAND: %s\n" , command.c_str());
// A better would be comparion string
// std::basic_string_view::starts_with):

if (command == "exit"){exit(0);}
else if (command == "next" || command == "nexti"){Nexti( child ); } // handle next /nexti commands
else if (command.find("break") == 0 ) 
{   
    // All this does is get the last string which is hopefully the address
    // break 0x56555527
    std::istringstream iss(command);
    std::size_t  LastWord = command.find_last_of(' ');
    std::string  addr = command.substr(LastWord + 1); // should be 0x########

     // convert address which is in string to unsigned int. 
     unsigned int uiaddr;
     std::stringstream hexstring ;
     hexstring << std::hex << addr;
     hexstring >> uiaddr;

   //printf("--- Break Point  - 0x%08x \n", uiaddr);
    SetBreakPoint(child, uiaddr); 
}
else if (command == "continue"|| command == "c")
{
    Continue(child);

}
else if (command == "infobreak" || command == "info break")
{
    ListBreakPoints();
}
else if (command.find("examined") == 0)
{
    //examined  --supposed to check a DWORD space in memory
    // All this does is get the last string which is hopefully the address
    // examined 0x56555527
    std::istringstream iss(command);
    std::size_t  LastWord = command.find_last_of(' ');
    std::string  addr = command.substr(LastWord + 1); // should be 0x########

     // convert address which is in string to unsigned int. 
     unsigned int uiaddr;
     std::stringstream hexstring ;
     hexstring << std::hex << addr;
     hexstring >> uiaddr;

    CheckDWORDMem(child, uiaddr);
}

else if (command == "info registers" || command ==  "i r")
{
    InfoRegisters(child);


}

else if (command.empty()){
    //printf("command empty");
    lastcommand = command;
    Nexti( child );
}
else {
    printf("[-] Unknown command entered\n");
    command = "";
}


lastcommand  = command;
 return ;

}



int main()
{

  int wait_status;
  unsigned icounter = 0;
  std::string executable;
  std::string command;


  std::cout << "[?] Type your name of executable path: ";
  std::getline(std::cin, executable);


 pid_t child = fork();
    if (child == 0) {
        execute_debugee(child, "/home/krash/works/Anastasia/build-Anastasia-Desktop-Debug/plugins/garbage/hello");
    
    }
    else{
        // exit? 


        
    }

// set up

// launch exe

    printf("[+] Debugger Started\n");
    wait(&wait_status);
   


while(WIFSTOPPED(wait_status)){

    //printf("WAIT STATUS %d\n" , wait_status);
    icounter++;
    struct user_regs_struct regs;
    ptrace(PTRACE_GETREGS, child, 0, &regs);
    unsigned instr = ptrace(PTRACE_PEEKTEXT, child, regs.eip, 0);
    printf("[+] Icounter = %u.  EIP = 0x%08x.  instr = 0x%08x\n", icounter, regs.eip, instr);


    printf(">>> ");
    std::getline(std::cin, command);
    HandleCommand(child  , command);


    }


printf("The child executed %u instructions\n", icounter);


}