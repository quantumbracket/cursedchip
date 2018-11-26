#include<iostream>
#include<ncurses.h>
#include<unistd.h>
#include<fstream>
#include<string.h>
#include<sys/time.h>




//general options
#define LENGHT 64
#define HEIGHT 32

#define BLANK_PIXEL ' '
char FILL_PIXEL = '@';
#define KEY_STICKINESS 8


#define TIMER_SPEED_HZ 60
#define USECS_PER_TIMER 16667


long CPU_SPEED_HZ = 500;
long USECS_PER_CPU = 2000; // 1e6/CPU_SPEED_HZ



#define step_err(stepfunc,errvar) errvar=stepfunc();if(errvar.error_num < 1) return errvar



typedef struct{
    ssize_t size;
    char *data;
} LOADEDROM;

typedef struct{
    uint16_t opcode;
    uint8_t _unused;
    int8_t error_num;
} ERRORCODE;

//error codes
#define CLEAN_EXIT 0 //exit with no error
#define EXEC_CONTINUE 1    //no error

#define ERR_OUT_OF_MEM -1
#define ERR_INVALID_OP -2
#define ERR_STACK_UNDERFLOW -3
#define ERR_STACK_OVERFLOW -4


//print error codes
const char* ERR_CODES_HR[]={"ERR_STACK_OVERFLOW","ERR_STACK_UNDERFLOW","ERR_INVALID_OP","ERR_OUT_OF_MEM","CLEAN_EXIT","EXEC_CONTINUE (suddenly stoped?)"};

#define err2string(errcode) ERR_CODES_HR[(int)(4+errcode)]





//converters
#define pack_error(errnum,erropcode) ERRORCODE {.opcode=erropcode,.error_num=errnum}


#define GetINSTR(x) (uint8_t)(x >> 0xC)
#define GetNNN(x) (uint16_t)(x & 0xfff)
#define GetN(x) (uint8_t)(x & 0xf)
#define GetX(x) (uint8_t)((x >> 8) & 0xf)
#define GetY(x) (uint8_t)((x >> 4) & 0xf)
#define GetKK(x) (uint8_t)(x & 0xff)

#define conv_timer(x) x > 0 ? x : 0




unsigned char CHIP8_FONTSET[80] =
{ 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};



long getMicrotime(){
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}

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
    res->data=rdata;
    res->size=romsize;

    return res;

}


class ScreenHandler{
    public:
    bool screen_buf[LENGHT][HEIGHT]={};
        ScreenHandler(){
            initscr();
            noecho();
            nonl();
            cbreak();
            curs_set(0);
        }

        ~ScreenHandler(){
            endwin();
        }

        void UpdateScreen(){
            uint8_t spr=0xF0;
            for(int x=0;x < LENGHT;x++){
                for(int y=0;y<HEIGHT;y++){
                    move(y,x);
                    addch((this->screen_buf[x][y]) ? FILL_PIXEL : BLANK_PIXEL);
                }
            }
        }
        

        bool ReadPixel(uint8_t x,uint8_t y){
            if(x < LENGHT && y < HEIGHT){
                return this->screen_buf[x][y];
            }else{
                return true;
            }
        }

        void WritePixel(uint8_t x,uint8_t y,bool val){
            if(x < LENGHT && y < HEIGHT){
                this->screen_buf[x][y] = val;
            }
        }


};


class KeyboardHandler {
    private:
        char keys[16]={0};
        uint8_t GetMapKey(int key){
            switch(key){
                case 49: //1
                    return 1;
                case 50: //2
                    return 2;
                case 51: //3
                    return 3;
                case 52: //4
                    return 0xC;
                case 113: //Q
                    return 4;
                case 119: //W
                    return 5;
                case 101: //E
                    return 6;
                case 114: //R
                    return 0xD;
                case 97: //A
                    return 7;
                case 115: //S
                    return 8;
                case 100: //D
                    return 9;
                case 102: //F
                    return 0xE;
                case 122: //Z
                    return 0xA;
                case 120: //X
                    return 0x0;
                case 99: //C
                    return 0xB;
                case 118: //V
                    return 0xF;
                default:
                    return 0xff;
                
            }
        }
    public:
        bool isKeyPressed(int key){
            int rkey=key % 16;
            return this->keys[rkey];
        }

        uint8_t GetKey(bool tmout=true){
            if(tmout) timeout(0); else timeout(-1);
            uint8_t k=this->GetMapKey(getch());
            if(k != 0xff){
                this->keys[k] = KEY_STICKINESS;
            }

            return k;
        }

        void Every60HZ(){
            for(int x=0;x < 16;x++){
                if(this->keys[x] != 0) this->keys[x]--;
            }


        }
};



class Chip8{

        void Every60HZ(){
            this->keyboard->Every60HZ();
            this->ST=conv_timer(this->ST-1);
            if(this->ST > 0){
                std::cout << '\a';
                std::cout.flush();
            }
            this->DT=conv_timer(this->DT-1);
        }

        ERRORCODE Step(){
            this->keyboard->GetKey();
            if(this->PC+1 > 0xfff) return pack_error(ERR_OUT_OF_MEM,0);

            uint16_t opcode=(this->mem[this->PC] << 8) | (this->mem[this->PC+1]);
            switch(GetINSTR(opcode)){
                case 0x0:{
                    switch(GetKK(opcode)){
                        case 0xE0:{//clear screen(cls)
                            for(int x=0;x < LENGHT;x++){
                                for(int y=0;y < HEIGHT;y++){
                                    this->screen->WritePixel(x,y,false);
                                }
                            }
                    
                            this->screen->UpdateScreen();
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }
                        case 0xFD:{//exit(exit)
                            return pack_error(CLEAN_EXIT,opcode);
                        }
                        case 0xEE:{//return(ret)
                            if(this->SP <= 0) return pack_error(ERR_STACK_UNDERFLOW,opcode);
                            this->PC=this->stack[this->SP--];
                            return pack_error(EXEC_CONTINUE,opcode);
                        }
                        default:{
                            return pack_error(ERR_INVALID_OP,opcode);
                        }


                    }
                }

                case 0x1:{//jump(jp NNN)
                    this->PC=GetNNN(opcode);
                    return pack_error(EXEC_CONTINUE,opcode);
                }
                
                case 0x2:{//call (call NNN)
                    if(this->SP >= 0x10) return pack_error(ERR_STACK_OVERFLOW,opcode);
                    this->stack[++this->SP]=this->PC+2;
                    this->PC=GetNNN(opcode);
                    return pack_error(EXEC_CONTINUE,opcode);
                }
                
                case 0x3:{//skip if equal(se Vx,KK)
                    if(this->V[GetX(opcode)] == GetKK(opcode)){
                        this->PC+=2;
                    }
                    this->PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0x4:{//skip if not equal(sne Vx,KK)
                    if(this->V[GetX(opcode)] != GetKK(opcode)){
                        this->PC+=2;
                    }
                    this->PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0x5:{//skip if equal(se Vx,Vy)
                    if(this->V[GetX(opcode)] == this->V[GetY(opcode)]){
                        this->PC+=2;
                    }
                    this->PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }


                case 0x6:{//load (ld Vx,kk)
                    this->V[GetX(opcode)] = GetKK(opcode);
                    this->PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0x7:{//add (add Vx,kk)
                    this->V[GetX(opcode)] += GetKK(opcode);
                    this->PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0x8:{
                    switch(GetN(opcode)){
                        case 0x0:{//load (ld Vx,Vy)
                            this->V[GetX(opcode)] = this->V[GetY(opcode)];
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x1:{//or (or Vx,Vy)
                            this->V[GetX(opcode)] |= this->V[GetY(opcode)];
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x2:{//and (and Vx,Vy)
                            this->V[GetX(opcode)] &= this->V[GetY(opcode)];
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x3:{//xor (xor Vx,Vy)
                            this->V[GetX(opcode)] ^= this->V[GetY(opcode)];
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x4:{//add (add Vx,Vy)(Vf = 1 on carry)
                            uint8_t regX=GetX(opcode);
                            uint8_t regY=GetY(opcode);
                            this->V[0xf] = (this->V[regY] > (0xff-this->V[regX])) ? 1 : 0;
                            this->V[regX] += this->V[regY];
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x5:{//substract (sub Vx,Vy)(Vf = 0 on borrow)
                            uint8_t regX=GetX(opcode);
                            uint8_t regY=GetY(opcode);
                            this->V[0xf] = (this->V[regX] > this->V[regY]) ? 1 : 0;
                            this->V[regX] -= this->V[regY];
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x6:{//divide by 2| shift right by 1 (shr Vx)(Vf = 1 if Vx lsb's = 1)
                            uint8_t regX=GetX(opcode);
                            this->V[0xf] = this->V[regX] & 1;
                            this->V[regX] >>= 1;
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x7:{//substract 'reverse' (subn Vx,Vy)(Vf = 0 on borrow)
                            uint8_t regX=GetX(opcode);
                            uint8_t regY=GetY(opcode);
                            this->V[0xf] = (this->V[regY] > this->V[regX]) ? 1 : 0;
                            this->V[regX] = this->V[regY] - this->V[regX];
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0xE:{//multiply by 2 | shift left by 1 (shl Vx)(Vf = 1 if Vx msb's = 1)
                            uint8_t regX=GetX(opcode);
                            this->V[0xf] = (this->V[regX] >> 7);
                            this->V[regX] <<= 1;
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        default:{
                            return pack_error(ERR_INVALID_OP,opcode);
                        }
                

                    }
                }
                
                case 0x9:{//skip if not equal (sne Vx,Vy)
                    if(this->V[GetX(opcode)] != this->V[GetY(opcode)]){
                        this->PC+=2;
                    }
                    this->PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0xA:{//load addr in I register (ld I,NNN)
                    this->I=GetNNN(opcode);
                    this->PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0xB:{//jump V0+addr (jp V0,NNN)
                    this->PC=this->V[0]+GetNNN(opcode);
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0xC:{//random number and KK (rnd Vx,kk)
                    this->V[GetX(opcode)] = (rand() % 0x100) & GetKK(opcode);
                    this->PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0xD:{//draw sprite at [I] in the position Vx and Vy on the screen with the sprite lenght of N (drw Vx,Vy,N)(Vf = 1 if collision)
                    uint8_t regX=this->V[GetX(opcode)];
                    uint8_t regY=this->V[GetY(opcode)];
                    uint8_t sprite_len=GetN(opcode);
                    this->V[0xf]=0;
                    for(int y = 0;y < sprite_len;y++){
                        uint16_t offset=this->I+y;
                        if(offset > 0xfff) return pack_error(ERR_OUT_OF_MEM,opcode);
                        uint8_t cur_line=this->mem[offset];
                        for(int x=0;x < 8;x++){
                            uint8_t xpos=(regX+x);
                            uint8_t ypos=(regY+y);
                            if(cur_line & (0x80 >> x)){
                                bool scr_pixel=this->screen->ReadPixel(xpos,ypos);
                                if(scr_pixel) this->V[0xf]=1;
                                this->screen->WritePixel(xpos,ypos,(scr_pixel != true));
                            }

                        }
                    }
                    this->screen->UpdateScreen();
                    
                    this->PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0xE:{
                    switch(GetKK(opcode)){
                        case 0x9E:{//skip if key is pressed (skp Vx)
                            if(this->keyboard->isKeyPressed(this->V[GetX(opcode)])){
                                this->PC+=2;
                            }
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0xA1:{//skip if key not pressed (sknp Vx)
                            if(!this->keyboard->isKeyPressed(this->V[GetX(opcode)])){
                                this->PC+=2;
                            }
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        default:{
                            return pack_error(ERR_INVALID_OP,opcode);
                        }

                    }
                }

                case 0xF:{
                    switch(GetKK(opcode)){
                        case 0x7:{//load DT register in Vx (ld Vx,DT)
                            this->V[GetX(opcode)] = this->DT;
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0xA:{//wait for key press and store value in Vx (ld Vx,K)
                            uint8_t cur_key=0xff;
                            while(cur_key == 0xff){
                                cur_key=this->keyboard->GetKey(false);
                            }
                            this->V[GetX(opcode)] = cur_key;
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x15:{//set DT (ld DT,Vx)
                            this->DT=this->V[GetX(opcode)];
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x18:{//set ST (ld ST,Vx)
                            this->ST=this->V[GetX(opcode)];
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x1E:{//add I with Vx (add I,Vx)
                            uint16_t bVal=this->I;
                            this->I = (this->I + this->V[GetX(opcode)]) % 0xfff;
                            this->V[0xf]=(this->I < bVal) ? 1 : 0;
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x29:{//sets I to the next sprite(I=(VX*5)) (ld F,Vx)
                            this->I = (this->V[GetX(opcode)] * 5) % 0xfff;
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x33:{//makes vx in human format(decimal) and stores it in I,I+1 and I+2 (ld B,Vx)
                            uint8_t regX=this->V[GetX(opcode)];
                            uint16_t Ival=this->I;
                            this->mem[Ival]=(uint8_t)(regX / 100 % 10); 
                            this->mem[Ival+1]=(uint8_t)(regX / 10 % 10);
                            this->mem[Ival+2]=(uint8_t)(regX % 10);
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x55:{//load registers from V0 to Vx at memory address I (ld [I], Vx)
                            uint8_t copy_size=GetX(opcode);
                            for(int x=0;x<=copy_size;x++){
                                this->mem[this->I+x] = this->V[x];
                            }
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }


                        case 0x65:{//read registers from V0 to Vx from memory address I (ld Vx,[I])
                            uint8_t copy_size=GetX(opcode);
                            for(int x=0;x<=copy_size;x++){
                                this->V[x] = this->mem[this->I+x];
                            }
                            this->PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        default:{
                            return pack_error(ERR_INVALID_OP,opcode);
                        }

                        


                    }
                }
                default:{ return pack_error(ERR_INVALID_OP,opcode); }    
            }
        }
    public:
        uint8_t mem[0xfff]={0};
        uint8_t V[0x10]={0};
        uint16_t I=0;
        uint16_t PC=0x200;
        uint8_t SP=0;
        uint16_t stack[0x10]={0};
        uint8_t DT=0;
        uint8_t ST=0;
        ScreenHandler* screen;
        KeyboardHandler* keyboard;
        Chip8(ScreenHandler* ss,KeyboardHandler* kk,ssize_t romsize,char* romdata){
            for(int x=0;x < 80;x++){
                this->mem[x]=CHIP8_FONTSET[x];
            }

            for(int x=0;x < romsize;x++){
                this->mem[0x200+x] = romdata[x];
            }
            this->screen=ss;
            this->keyboard=kk;
            srand(time(NULL));
        }

        ERRORCODE Run(){
            ERRORCODE e_status;
            long before=getMicrotime();
            for(;;){
                long now=getMicrotime();
                long ticks_left=(now-before)/USECS_PER_TIMER;
                for(long ticks=0;ticks<ticks_left;ticks++){
                    this->Every60HZ();
                    before=now;
                }
                step_err(this->Step,e_status);
                
                usleep(USECS_PER_CPU);
            }
        }

        ~Chip8(){
            this->screen->~ScreenHandler();
        }



};




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

    Chip8 emu(&scr,&kb,rom_contents->size,rom_contents->data);

    
    ERRORCODE err= emu.Run();
    uint16_t curpc = emu.PC;
    emu.~Chip8();

    std::printf("error code: %hhi %s\nopcode: %04hx\nPC = 0x%hx\n",err.error_num,err2string(err.error_num),err.opcode,curpc);
    return 0;
    
}
