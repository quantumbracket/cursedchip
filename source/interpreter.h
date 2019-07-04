#pragma once


#include "runtime.h"

#define step_err(stepfunc,errvar) errvar=stepfunc();if(errvar.error_num < 1) return errvar

//converters

#define GetINSTR(x) (uint8_t)(x >> 0xC)
#define GetNNN(x) (uint16_t)(x & 0xfff)
#define GetN(x) (uint8_t)(x & 0xf)
#define GetX(x) (uint8_t)((x >> 8) & 0xf)
#define GetY(x) (uint8_t)((x >> 4) & 0xf)
#define GetKK(x) (uint8_t)(x & 0xff)

#define conv_timer(x) x > 0 ? x : 0

long getMicrotime(){
	struct timeval currentTime;
	gettimeofday(&currentTime, NULL);
	return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}


class Interpreter{

        void Every60HZ(){
            this->runtime->keyboard->Every60HZ();
            this->runtime->registers.ST=conv_timer(runtime->registers.ST-1);
            if(this->runtime->registers.ST > 0){
                std::cout << '\a';
                std::cout.flush();
            }
            this->runtime->registers.DT=conv_timer(runtime->registers.DT-1);
        }

        ERRORCODE Step(){
            this->runtime->keyboard->GetKey();
            if(this->runtime->registers.PC+1 > 0xfff) return pack_error(ERR_OUT_OF_MEM,0);

            uint16_t opcode=(this->runtime->mem[runtime->registers.PC] << 8) | (runtime->mem[runtime->registers.PC+1]);
            switch(GetINSTR(opcode)){
                case 0x0:{
                    switch(GetKK(opcode)){
                        case 0xE0:{//clear screen(cls)
                            this->runtime->screen->ClearScreen(); 
                            this->runtime->screen->UpdateScreen();
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }
                        case 0xFD:{//exit(exit)
                            return pack_error(CLEAN_EXIT,opcode);
                        }
                        case 0xEE:{//return(ret)
                            if(this->runtime->registers.SP <= 0) return pack_error(ERR_STACK_UNDERFLOW,opcode);
                            this->runtime->registers.PC=runtime->stack[runtime->registers.SP--];
                            return pack_error(EXEC_CONTINUE,opcode);
                        }
                        default:{
                            return pack_error(ERR_INVALID_OP,opcode);
                        }


                    }
                }

                case 0x1:{//jump(jp NNN)
                    this->runtime->registers.PC=GetNNN(opcode);
                    return pack_error(EXEC_CONTINUE,opcode);
                }
                
                case 0x2:{//call (call NNN)
                    if(this->runtime->registers.SP >= 0x10) return pack_error(ERR_STACK_OVERFLOW,opcode);
                    this->runtime->stack[++runtime->registers.SP]=runtime->registers.PC+2;
                    this->runtime->registers.PC=GetNNN(opcode);
                    return pack_error(EXEC_CONTINUE,opcode);
                }
                
                case 0x3:{//skip if equal(se Vx,KK)
                    if(this->runtime->registers.V[GetX(opcode)] == GetKK(opcode)){
                        this->runtime->registers.PC+=2;
                    }
                    this->runtime->registers.PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0x4:{//skip if not equal(sne Vx,KK)
                    if(this->runtime->registers.V[GetX(opcode)] != GetKK(opcode)){
                        this->runtime->registers.PC+=2;
                    }
                    this->runtime->registers.PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0x5:{//skip if equal(se Vx,Vy)
                    if(this->runtime->registers.V[GetX(opcode)] == this->runtime->registers.V[GetY(opcode)]){
                        this->runtime->registers.PC+=2;
                    }
                    this->runtime->registers.PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }


                case 0x6:{//load (ld Vx,kk)
                    this->runtime->registers.V[GetX(opcode)] = GetKK(opcode);
                    this->runtime->registers.PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0x7:{//add (add Vx,kk)
                    this->runtime->registers.V[GetX(opcode)] += GetKK(opcode);
                    this->runtime->registers.PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0x8:{
                    switch(GetN(opcode)){
                        case 0x0:{//load (ld Vx,Vy)
                            this->runtime->registers.V[GetX(opcode)] = this->runtime->registers.V[GetY(opcode)];
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x1:{//or (or Vx,Vy)
                            this->runtime->registers.V[GetX(opcode)] |= this->runtime->registers.V[GetY(opcode)];
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x2:{//and (and Vx,Vy)
                            this->runtime->registers.V[GetX(opcode)] &= this->runtime->registers.V[GetY(opcode)];
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x3:{//xor (xor Vx,Vy)
                            this->runtime->registers.V[GetX(opcode)] ^= this->runtime->registers.V[GetY(opcode)];
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x4:{//add (add Vx,Vy)(Vf = 1 on carry)
                            uint8_t regX=GetX(opcode);
                            uint8_t regY=GetY(opcode);
                            this->runtime->registers.V[0xf] = (this->runtime->registers.V[regY] > (0xff-this->runtime->registers.V[regX])) ? 1 : 0;
                            this->runtime->registers.V[regX] += this->runtime->registers.V[regY];
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x5:{//substract (sub Vx,Vy)(Vf = 0 on borrow)
                            uint8_t regX=GetX(opcode);
                            uint8_t regY=GetY(opcode);
                            this->runtime->registers.V[0xf] = (this->runtime->registers.V[regX] > this->runtime->registers.V[regY]) ? 1 : 0;
                            this->runtime->registers.V[regX] -= this->runtime->registers.V[regY];
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x6:{//divide by 2| shift right by 1 (shr Vx)(Vf = 1 if Vx lsb's = 1)
                            uint8_t regX=GetX(opcode);
                            this->runtime->registers.V[0xf] = this->runtime->registers.V[regX] & 1;
                            this->runtime->registers.V[regX] >>= 1;
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x7:{//substract 'reverse' (subn Vx,Vy)(Vf = 0 on borrow)
                            uint8_t regX=GetX(opcode);
                            uint8_t regY=GetY(opcode);
                            this->runtime->registers.V[0xf] = (this->runtime->registers.V[regY] > this->runtime->registers.V[regX]) ? 1 : 0;
                            this->runtime->registers.V[regX] = this->runtime->registers.V[regY] - this->runtime->registers.V[regX];
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0xE:{//multiply by 2 | shift left by 1 (shl Vx)(Vf = 1 if Vx msb's = 1)
                            uint8_t regX=GetX(opcode);
                            this->runtime->registers.V[0xf] = (this->runtime->registers.V[regX] >> 7);
                            this->runtime->registers.V[regX] <<= 1;
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        default:{
                            return pack_error(ERR_INVALID_OP,opcode);
                        }
                

                    }
                }
                
                case 0x9:{//skip if not equal (sne Vx,Vy)
                    if(this->runtime->registers.V[GetX(opcode)] != this->runtime->registers.V[GetY(opcode)]){
                        this->runtime->registers.PC+=2;
                    }
                    this->runtime->registers.PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0xA:{//load addr in I register (ld I,NNN)
                    this->runtime->registers.I=GetNNN(opcode);
                    this->runtime->registers.PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0xB:{//jump V0+addr (jp V0,NNN)
                    this->runtime->registers.PC=this->runtime->registers.V[0]+GetNNN(opcode);
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0xC:{//random number and KK (rnd Vx,kk)
                    this->runtime->registers.V[GetX(opcode)] = (rand() % 0x100) & GetKK(opcode);
                    this->runtime->registers.PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0xD:{//draw sprite at [I] in the position Vx and Vy on the screen with the sprite lenght of N (drw Vx,Vy,N)(Vf = 1 if collision)
                    uint8_t regX=this->runtime->registers.V[GetX(opcode)];
                    uint8_t regY=this->runtime->registers.V[GetY(opcode)];
                    uint8_t sprite_len=GetN(opcode);
                    this->runtime->registers.V[0xf]=0;
                    for(int y = 0;y < sprite_len;y++){
                        uint16_t offset=this->runtime->registers.I+y;
                        if(offset > 0xfff) return pack_error(ERR_OUT_OF_MEM,opcode);
                        uint8_t cur_line=this->runtime->mem[offset];
                        for(int x=0;x < 8;x++){
                            uint8_t xpos=(regX+x);
                            uint8_t ypos=(regY+y);
                            if(cur_line & (0x80 >> x)){
                                bool scr_pixel=this->runtime->screen->ReadPixel(xpos,ypos);
                                if(scr_pixel) this->runtime->registers.V[0xf]=1;
                                this->runtime->screen->WritePixel(xpos,ypos,(scr_pixel != true));
                            }

                        }
                    }
                    this->runtime->screen->UpdateScreen();
                    
                    this->runtime->registers.PC+=2;
                    return pack_error(EXEC_CONTINUE,opcode);
                }

                case 0xE:{
                    switch(GetKK(opcode)){
                        case 0x9E:{//skip if key is pressed (skp Vx)
                            if(this->runtime->keyboard->isKeyPressed(this->runtime->registers.V[GetX(opcode)])){
                                this->runtime->registers.PC+=2;
                            }
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0xA1:{//skip if key not pressed (sknp Vx)
                            if(!this->runtime->keyboard->isKeyPressed(this->runtime->registers.V[GetX(opcode)])){
                                this->runtime->registers.PC+=2;
                            }
                            this->runtime->registers.PC+=2;
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
                            this->runtime->registers.V[GetX(opcode)] = runtime->registers.DT;
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0xA:{//wait for key press and store value in Vx (ld Vx,K)
                            uint8_t cur_key=0xff;
                            while(cur_key == 0xff){
                                cur_key=this->runtime->keyboard->GetKey(false);
                            }
                            this->runtime->registers.V[GetX(opcode)] = cur_key;
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x15:{//set DT (ld DT,Vx)
                            this->runtime->registers.DT=this->runtime->registers.V[GetX(opcode)];
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x18:{//set ST (ld ST,Vx)
                            this->runtime->registers.ST=this->runtime->registers.V[GetX(opcode)];
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x1E:{//add I with Vx (add I,Vx)
                            uint16_t bVal=this->runtime->registers.I;
                            this->runtime->registers.I = (runtime->registers.I + this->runtime->registers.V[GetX(opcode)]) & 0xfff;
                            this->runtime->registers.V[0xf]=(runtime->registers.I < bVal) ? 1 : 0;
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x29:{//sets I to the next sprite(I=(VX*5)) (ld F,Vx)
                            this->runtime->registers.I = (this->runtime->registers.V[GetX(opcode)] * 5) & 0xfff;
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x33:{//makes vx in human format(decimal) and stores it in I,I+1 and I+2 (ld B,Vx)
                            uint8_t regX=this->runtime->registers.V[GetX(opcode)];
                            uint16_t Ival=this->runtime->registers.I;
                            this->runtime->mem[Ival]=(uint8_t)(regX / 100 % 10); 
                            this->runtime->mem[Ival+1]=(uint8_t)(regX / 10 % 10);
                            this->runtime->mem[Ival+2]=(uint8_t)(regX % 10);
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }

                        case 0x55:{//load registers from V0 to Vx at memory address I (ld [I], Vx)
                            uint8_t copy_size=GetX(opcode);
                            for(int x=0;x<=copy_size;x++){
                                this->runtime->mem[runtime->registers.I+x] = this->runtime->registers.V[x];
                            }
                            this->runtime->registers.PC+=2;
                            return pack_error(EXEC_CONTINUE,opcode);
                        }


                        case 0x65:{//read registers from V0 to Vx from memory address I (ld Vx,[I])
                            uint8_t copy_size=GetX(opcode);
                            for(int x=0;x<=copy_size;x++){
                                this->runtime->registers.V[x] = runtime->mem[runtime->registers.I+x];
                            }
                            this->runtime->registers.PC+=2;
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
	Runtime* runtime;

        Interpreter(ScreenHandler* ss,KeyboardHandler* kk, Runtime* rt){
	    this->runtime = rt;
            this->runtime->screen=ss;
            this->runtime->keyboard=kk;
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




};


