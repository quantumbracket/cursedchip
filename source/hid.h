#pragma once


// screen resolution
#define WIDTH 64
#define HEIGHT 32

#define BLANK_PIXEL ' '
char FILL_PIXEL = '@';
#define KEY_STICKINESS 8






class ScreenHandler{
    public:
    bool screen_buf[WIDTH][HEIGHT]={};
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
            for(int x=0;x < WIDTH;x++){
                for(int y=0;y<HEIGHT;y++){
                    move(y,x);
                    addch((this->screen_buf[x][y]) ? FILL_PIXEL : BLANK_PIXEL);
                }
            }
        }

	void ClearScreen(){
		for(int x=0;x < WIDTH;x++){
                	for(int y=0;y < HEIGHT;y++){
                		this->WritePixel(x,y,false);
                	}
                }
	}
        

        bool ReadPixel(uint8_t x,uint8_t y){
            if(x < WIDTH && y < HEIGHT){
                return this->screen_buf[x][y];
            }else{
                return true;
            }
        }

        void WritePixel(uint8_t x,uint8_t y,bool val){
            if(x < WIDTH && y < HEIGHT){
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

