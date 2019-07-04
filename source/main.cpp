#include<iostream>
#include<ncurses.h>
#include<unistd.h>
#include<fstream>
#include<string.h>
#include<sys/time.h>


#include "hid.h"
#include "runtime.h"
#include "interpreter.h"


/* TODO:
 * anyting that runs at 60hz should run in a separate thread
 */



typedef struct{
    ssize_t size;
    uint8_t *data;
} LOADEDROM;



LOADEDROM* ROMLoad(const char* filename){
    std::streampos size;
    uint16_t romsize;
    LOADEDROM* res=new LOADEDROM();
    char* rdata;
    std::ifstream romstream (filename,std::ios::in | std::ios::binary | std::ios::ate);
    size=romstream.tellg();
    romsize = (size) & 0xDFF;
    rdata=new char[romsize];
    romstream.seekg(0,std::ios::beg);
    romstream.read(rdata,romsize);
    romstream.close();
    res->data=(uint8_t*) rdata;
    res->size=romsize;

    return res;

}





void parse_args(const char *args[],int arglen,const char* progname){
    int sm=0;
    if(!(!strcmp(args[0],"--help") || !strcmp(args[0],"-h"))){
        sm=1;
    }
    for(int x=sm;x<arglen;x++){
        if(!strcmp(args[x],"--pixelchar") || !strcmp(args[x],"-p")){
            if(x == (arglen-1)){
                std::printf("%s requires argument\n",args[x]);
                exit(-1);
            }else{
                FILL_PIXEL=args[x+1][0];
                x++;
                continue;
            }
        }


        if(!strcmp(args[x],"--clock") || !strcmp(args[x],"-c")){
            if(x == (arglen-1)){
                std::printf("%s requires argument\n",args[x]);
                exit(-1);
            }else{
                CPU_SPEED_HZ=strtoul(args[x+1],NULL,10);
                USECS_PER_CPU=(long)(1e6/CPU_SPEED_HZ);
                if(CPU_SPEED_HZ > 2000 || CPU_SPEED_HZ < 300){
                    std::printf("error: cpu speed cannot be above 2000Hz or below 300Hz\n");
                    exit(-1);
                }
                x++;
                continue;
            }
        }

        if(!strcmp(args[x],"--help") || !strcmp(args[x],"-h")){
            std::printf("Ussage: %s <romfile> [flags]\n",progname);
            std::printf("Flags:\n");
            std::printf("\t--pixelchar || -p:\tset char that will represent pixels on screen(default: @)\n");
            std::printf("\t--clock || -c:\t\tset cpu frequency(default: 500)\n");
            std::printf("\t--help || -h:\t\tshow this\n");
            exit(0);
        }

        std::printf("unknown flag: %s\n",args[x]);
        exit(1);

    }
}






int main(int argc,const char* argv[]){
    if(argc < 2){
        std::printf("Ussage: %s <romfile> [flags]\n",argv[0]); 
        std::printf("use -h or --help to get a list of flags\n");
        return 1;
    }
    

    parse_args(&argv[1],argc-1,argv[0]);


    LOADEDROM* rom_contents=ROMLoad(argv[1]);

    
    ScreenHandler scr;
    KeyboardHandler kb;
    Runtime rt(rom_contents->data,rom_contents->size);

    Interpreter emu(&scr,&kb,&rt);

    
    ERRORCODE err= emu.Run();
    scr.~ScreenHandler();
    uint16_t curpc = rt.registers.PC;
    emu.~Interpreter();

    std::printf("error code: %hhi %s\nopcode: %04hx\nPC = 0x%hx\n",err.error_num,err2string(err.error_num),err.opcode,curpc);
    return 0;
    
}
