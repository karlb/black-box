/*  Black-box: guess where the crystals are !                                           
    Copyright (C) 2000-2008 Karl Bartel                                              
                                                                                
    This program is free software; you can redistribute it and/or modify        
    it under the terms of the GNU General Public License as published by        
    the Free Software Foundation; either version 2 of the License, or           
    (at your option) any later version.                                         
                                                                                
    This program is distributed in the hope that it will be useful,             
    but WITHOUT ANY WARRANTY; without even the implied warranty of              
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               
    GNU General Public License for more details.                                
                                                                                
    You should have received a copy of the GNU General Public License           
    along with this program; if not, write to the Free Software                 
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA   
                                                                                
    Karl Bartel                                                                 
    Cecilienstr. 14                                                             
    12307 Berlin                                                                
    GERMANY                                                                     
    karlb@gmx.net                                                               
*/                                                                              

// This ports the game to SFont...
#define Text( dest, d1, x, y, d2, text, d3) PutString( dest, x, y, text)

#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_timer.h>
#include <SDL_mixer.h>
#include <signal.h>
#include "SFont.h"

       int fullscreen=1,difficulty=4,trihigh=1,sound=1;   //settings

       SDL_Event event;
       SDL_Rect dstrect;
       SDL_Surface *screen,*box_pic,*edge1_pic,*edge2_pic,*think_pic;
       SDL_Surface *quit_pic,*real_pic,*box4_pic,*ok_pic,*new_pic,*edge3_pic;
       SDL_Surface *light_pic,*edge1red_pic,*edge2red_pic,*edge3red_pic;
       SDL_Surface *edge1rred_pic,*edge2rred_pic,*edge3rred_pic,*help_pic;
       SDL_Surface *next_pic,*prev_pic,*won_pic,*giveup_pic;
       SDL_Surface *demo_pic[8],*Font;
       Mix_Music *music;
       int mouse_x, mouse_y, level_num, won_shown, this_turn_demo, LevelGiven=0;
       char text[80];
       char xline[9];
       char yline[11];
       char think[12][9];
       char light[11][9];
       char real[12][9];

#define PATHNUM 5
char DATAPATH[200]=DATADIR;
const char PATH[PATHNUM][200]={DATADIR,".","data","../data",DATADIR};

typedef struct {
       int x;
       int y;
       int dir;
} output;


void ComplainAndExit(void)
{
        fprintf(stderr, "Problem: %s\n", SDL_GetError());
        exit(1);
}

void setpix( Sint32 X, Sint32 Y, Uint8 red, Uint8 green, Uint8 blue)  //sets one Pixel to the screen 
{
        Uint32   pixel;
        Uint8   *bits, bpp;

        pixel = SDL_MapRGB(screen->format, red , green , blue);

        if ( SDL_MUSTLOCK(screen) ) {
                if ( SDL_LockSurface(screen) < 0 )
                        return;
        }
        bpp = screen->format->BytesPerPixel;
        bits = ((Uint8 *)screen->pixels)+Y*screen->pitch+X*bpp;

        switch(bpp) {
                case 1:
                        *((Uint8 *)(bits)) = (Uint8)pixel;
                        break;
                case 2:
                        *((Uint16 *)(bits)) = (Uint16)pixel;
                        break;
                case 3: { /* Format/endian independent */
                        Uint8 r, g, b;

                        r = (pixel>>screen->format->Rshift)&0xFF;
                        g = (pixel>>screen->format->Gshift)&0xFF;
                        b = (pixel>>screen->format->Bshift)&0xFF;
                        *((bits)+screen->format->Rshift/8) = r; 
                        *((bits)+screen->format->Gshift/8) = g;
                        *((bits)+screen->format->Bshift/8) = b;
                        }
                        break;
                case 4:
                        *((Uint32 *)(bits)) = (Uint32)pixel;
                        break;
        }
        /* Update the display */
        if ( SDL_MUSTLOCK(screen) ) {
                SDL_UnlockSurface(screen);
        }
        SDL_UpdateRect(screen, X, Y, 1, 1);
        return;
}

SDL_Surface *LoadImage(char *datafile, int transparent)   // reads one GIF into the memory
{
  SDL_Surface *pic,*pic2;
  char filename[200];
  int i=0;

  sprintf(filename,"%s/gfx/%s",DATAPATH,datafile);
  pic = IMG_Load(filename);
  while ( pic == NULL ) {
    strcpy(DATAPATH,PATH[i]);
    sprintf(filename,"%s/gfx/%s",DATAPATH,datafile);
    pic = IMG_Load(filename);
    i++;
    
    if (i>PATHNUM)
    {
      fprintf(stderr,"Couldn't load %s: %s\n", filename, SDL_GetError());
      exit(2);
    }
  }
  if (transparent==3)
      return pic;
  pic2 = SDL_DisplayFormat(pic);
  SDL_FreeSurface(pic);
  if (transparent==1)
    SDL_SetColorKey(pic2,SDL_SRCCOLORKEY|SDL_RLEACCEL,SDL_MapRGB(pic2->format,0xFF,0xFF,0xFF));
  if (transparent==2)
    SDL_SetColorKey(pic2,SDL_SRCCOLORKEY|SDL_RLEACCEL,0);
  return (pic2);
}

void blit(int Xpos,int Ypos,SDL_Surface *image)  //blits one GIF or BMP from the memory to the screen
{
  dstrect.x = Xpos;
  dstrect.y = Ypos;
  dstrect.w = image->w;
  dstrect.h = image->h;
  if ( SDL_BlitSurface(image, NULL, screen, &dstrect) < 0 ) 
  {
    SDL_FreeSurface(image);
    ComplainAndExit();
  }
}

void blit_part(int Xpos,int Ypos,SDL_Surface *image, SDL_Rect srcrect)  //blits a part of one GIF or BMP from the memory to the screen
{
  dstrect.x = srcrect.x;
  dstrect.y = srcrect.y;
  dstrect.w = srcrect.w;
  dstrect.h = srcrect.h;
  if ( SDL_BlitSurface(image, &srcrect , screen, &dstrect) < 0 ) 
  {
    SDL_FreeSurface(image);
    ComplainAndExit();
  }
}

void init_SDL()  // sets the video mode
{
  if ( SDL_Init(SDL_INIT_VIDEO) < 0 ) {ComplainAndExit();}
  atexit(SDL_Quit);
  // Set the video mode (800x600 at 16-bit depth)  
  if (fullscreen)  
    { 
      screen = SDL_SetVideoMode(800, 600, 16, SDL_SWSURFACE| SDL_FULLSCREEN);
    }
  else { screen = SDL_SetVideoMode(800, 600, 16, SDL_SWSURFACE); }
  if ( screen == NULL ) {ComplainAndExit();}
  if ( SDL_Init(SDL_INIT_AUDIO) < 0 ) 
    {fprintf(stderr, "Couldn't initialize SDL: %s\n",SDL_GetError());exit(2);}
}

void DontReadKeys()
{
  SDL_EventState(SDL_KEYUP, SDL_IGNORE);                                  
  SDL_EventState(SDL_KEYDOWN, SDL_IGNORE);                                  
}

void load_images()
{
  Uint8 i;  

  for (i=0;i<=7;i++)
  {
    sprintf(text,"demo%d.gif",i+1);
    demo_pic[i]=LoadImage(text,0);
  }
  won_pic=LoadImage("won.png",3);
  giveup_pic=LoadImage("giveup.gif",0);
  next_pic=LoadImage("next.gif",0);
  prev_pic=LoadImage("prev.gif",0);
  box_pic=LoadImage("box.gif",0);
  edge1_pic=LoadImage("edge1.gif",1);
  edge2_pic=LoadImage("edge2.gif",1);
  edge3_pic=LoadImage("edge3.gif",1);
  edge1rred_pic=LoadImage("edge1red.gif",1);
  edge2rred_pic=LoadImage("edge2red.gif",1);
  edge3rred_pic=LoadImage("edge3red.gif",1);
  think_pic=LoadImage("think.gif",1);
  ok_pic=LoadImage("ok.gif",0);
  new_pic=LoadImage("new.gif",0);
  quit_pic=LoadImage("quit.gif",0);
  real_pic=LoadImage("real.gif",1);
  light_pic=LoadImage("light.gif",0);
  help_pic=LoadImage("help.gif",0);
  Font=LoadImage("font.png",3);

  SDL_SetAlpha(think_pic,(SDL_SRCALPHA),160);
  SDL_SetAlpha(real_pic,(SDL_SRCALPHA),210);
}

void init_blit()
{
  char text[100];
  SDL_Rect rect;

  blit(695,10,giveup_pic);
  blit(695,530,quit_pic);
  blit(695,80,new_pic);
  blit(695,480,help_pic);
  
  sprintf(text,"Level %d",level_num);
  rect.x=800-111;
  rect.y=300;
  rect.w=111;
  rect.h=Font->h;
  SDL_FillRect(screen, &rect, SDL_MapRGB(screen->format,0,0,0));
  PutString(screen,800-TextWidth(text),300,text);
  SDL_UpdateRect(screen,0,0,0,0);
}

void ReadCommandLine(char *argv[])
{
  int i;
  for ( i=1;argv[i];i++ ) 
  {
    if ((strcmp(argv[i],"--nofullscreen")==0)||(strcmp(argv[i],"-f")==0)) {fullscreen=0;} else
    if ((strcmp(argv[i],"--nosound")==0)||(strcmp(argv[i],"-s")==0)) {sound=0;} else
    if (((strcmp(argv[i],"--levelnum")==0)&&(argv[i+1]))||((strcmp(argv[i],"-l")==0)&&(argv[i+1])))
      {i++;level_num=atoi(argv[i])%100000;printf("starting with level #%d\n",level_num);LevelGiven=1;} else
    {
	if (!((strcmp(argv[i],"-h")==0)||(strcmp(argv[i],"--help")==0))) printf("Unknown parameter: \"%s\"\n", argv[i]);
	puts("\nCommand line options:\n");
	puts("  -f, --nofullscreen       start in windowed mode");
	puts("  -s, --nosound            start without sound");
	puts("  -l, --levelnum           start specified level (number has to follow)");
	puts("  -h, --help               this text\n");
	
	exit(0);
    }
  }
}

void free_memory()
{
  Uint8 i;  

  SDL_FreeSurface(box_pic);          
  SDL_FreeSurface(edge1_pic);     
  SDL_FreeSurface(edge2_pic);  
  SDL_FreeSurface(edge3_pic);     
  SDL_FreeSurface(edge1red_pic);     
  SDL_FreeSurface(edge2red_pic);  
  SDL_FreeSurface(edge3red_pic);     
  SDL_FreeSurface(think_pic);  
  SDL_FreeSurface(ok_pic);  
  SDL_FreeSurface(giveup_pic);  
  SDL_FreeSurface(new_pic);  
  SDL_FreeSurface(quit_pic);  
  SDL_FreeSurface(real_pic);  
  SDL_FreeSurface(light_pic);  
  SDL_FreeSurface(help_pic);  
}


void xshot(int y,int right,int erase) //x right
{
  int x,move;
  for (move=25;move<75;move++)
  {
    if (erase)
    {
      setpix(move+right*600,y,0,0,0);
      setpix(move+right*600,y+1,0,0,0);
      setpix(move+right*600,y-1,0,0,0);
    }
    else
    {
      setpix(move+right*600,y,255,0,0);
      setpix(move+right*600,y+1,255,0,0);
      setpix(move+right*600,y-1,255,0,0);
    }
    if (move % 2==0) SDL_Delay(1);
    SDL_PollEvent(&event);
  }
}

void xshot2(int y,int right,int erase) //x left
{
  int x,move;
  for (move=74;move>24;move--)
  {
    if (erase)
    {
      setpix(move+right*600,y,0,0,0);
      setpix(move+right*600,y+1,0,0,0);
      setpix(move+right*600,y-1,0,0,0);
    }
    else
    {
      setpix(move+right*600,y,255,0,0);
      setpix(move+right*600,y+1,255,0,0);
      setpix(move+right*600,y-1,255,0,0);
    }
    if (move % 2==0) SDL_Delay(1);
    SDL_PollEvent(&event);
  }
}

void yshot(int x,int down,int erase) // y down
{
  int y,move;
  for (move=24;move<74;move++)
  {
    if (erase)
    {
      setpix(x,move+down*500,0,0,0);
      setpix(x+1,move+down*500,0,0,0);
      setpix(x-1,move+down*500,0,0,0);
    }
    else 
    {
      setpix(x,move+down*500,255,0,0);
      setpix(x+1,move+down*500,255,0,0);
      setpix(x-1,move+down*500,255,0,0);
    }
    if (move % 2==0) SDL_Delay(1);
    SDL_PollEvent(&event);
  }
}

void yshot2(int x,int down,int erase)  //y up
{
  int y,move;
  for (move=74;move>24;move--)
  {
    if (erase)
    {
      setpix(x,move+down*500,0,0,0);
      setpix(x+1,move+down*500,0,0,0);
      setpix(x-1,move+down*500,0,0,0);
    }
    else 
    {
      setpix(x,move+down*500,255,0,0);
      setpix(x+1,move+down*500,255,0,0);
      setpix(x-1,move+down*500,255,0,0);
    }
    if (move % 2==0) SDL_Delay(1);
    SDL_PollEvent(&event);
  }
}

void select_shot(int x,int y,int dir,int erase)
{
  if ((dir==1)||(dir==3))
  {
    if (dir==1)
      xshot((y+2)*50,div(x,11).quot,erase);
    if (dir==3)
      xshot2((y+2)*50,div(x,11).quot,erase);
  }
  else
  {
    if (dir==0)
      yshot2((x+2)*50,div(y,9).quot,erase);
    if (dir==2)
      yshot((x+2)*50,div(y,9).quot,erase);
  }
}

output calc_out(char map[12][9],int x,int y,int dir)
{
  int end=0;
  output out;
  
  while (!end)
  {
    switch (dir)
    {
    case 0:
      if (y==0) {end=1;} else
      if (map[x-1][y-1]==1) {dir=1;} else
      if (map[x][y-1]==1) {dir=3;}
      else {y--;}
    break;
    case 1:
      if (x==12) {end=1;} else
      if (map[x][y]==1) {dir=0;} else
      if (map[x][y-1]==1) {dir=2;}
      else {x++;}
    break;
    case 2:
      if (y==9) {end=1;} else
      if (map[x][y]==1) {dir=3;} else
      if (map[x-1][y]==1) {dir=1;}
      else {y++;}
    break;
    case 3:
      if (x==0) {end=1;} else
      if (map[x-1][y]==1) {dir=0;} else
      if (map[x-1][y-1]==1) {dir=2;}
      else {x--;}
    break;
    }
    if ((x>12)||(y>9)||(x<0)||(y<0))
    {
      fprintf(stderr,"ERROR: calc_out: x or y out of range, quitting now.\n");
      free_memory();
      exit(1);
    }
  }
  out.x=x;
  out.y=y;
  out.dir=dir;

  return(out);
}

void blit_screen()
{
  int x,y;
  SDL_Rect rect;
  
  if (trihigh)
  {
    edge1red_pic=edge1rred_pic;
    edge2red_pic=edge2rred_pic;
    edge3red_pic=edge3rred_pic;
  }else
  {
    edge1red_pic=edge1_pic;
    edge2red_pic=edge2_pic;
    edge3red_pic=edge3_pic;
  }
  
  for (x=0;x<=10;x++){
    for (y=0;y<=8;y++){
      if (light[x][y]==0)
        {blit(x*50+75,y*50+75,box_pic);}
      else {blit(x*50+75,y*50+75,light_pic);}
    }
  }
  for (x=0;x<=10;x++){
    for (y=0;y<=8;y++){
      if (think[x][y])
        {blit(x*50+100,y*50+100,think_pic);}
    }
  }
  for (x=0;x<=10;x++){
    if ((calc_out(real,x,0,2).x==calc_out(think,x,0,2).x)
       &&(calc_out(real,x,0,2).y==calc_out(think,x,0,2).y)
       &&(calc_out(real,x,0,2).dir==calc_out(think,x,0,2).dir))
    {blit(x*50+75,0,edge1_pic);}
    else {blit(x*50+75,0,edge1red_pic);}
  }
  for (y=0;y<=8;y++){
    if ((calc_out(real,0,y,1).x==calc_out(think,0,y,1).x)
       &&(calc_out(real,0,y,1).y==calc_out(think,0,y,1).y)
       &&(calc_out(real,0,y,1).dir==calc_out(think,0,y,1).dir))
    {blit(0,y*50+75,edge2_pic);}
    else {blit(0,y*50+75,edge2red_pic);}
  }  
  for (x=0;x<=10;x++){
    if ((calc_out(real,x,9,0).x==calc_out(think,x,9,0).x)
       &&(calc_out(real,x,9,0).y==calc_out(think,x,9,0).y)
       &&(calc_out(real,x,9,0).dir==calc_out(think,x,9,0).dir))
    {blit(x*50+75,562,edge3_pic);}
    else {blit(x*50+75,562,edge3red_pic);}
  }
  if (fullscreen)
  {
    rect.x=25;
    rect.y=25;
    rect.w=675;
    rect.h=50;
    SDL_FillRect(screen,&rect,0);
    rect.w=50;
    rect.h=550;
    SDL_FillRect(screen,&rect,0);
    rect.x=25;
    rect.y=525;
    rect.w=675;
    rect.h=50;
    SDL_FillRect(screen,&rect,0);
  }
  SDL_UpdateRect(screen,0,0,0,0);
}

void hidden(int x,int y,int dir)  //0=up 1=right 2=down 3=left
{
  select_shot(x,y,dir,0);
  output out=calc_out(real,x,y,dir);

  if ((out.x==x)&&(out.y==y)&&((dir==out.dir+2)||(dir==out.dir-2)))
  {
    select_shot(x,y,dir,1);
    select_shot(x,y,out.dir,0);
    select_shot(x,y,out.dir,1);    
  }
  else
  {
    select_shot(out.x,out.y,out.dir,0);
    SDL_Delay(500);
    select_shot(x,y,dir,1);
    select_shot(out.x,out.y,out.dir,1);
  }
}

void show_real()
{
  int x,y;
  
  blit_screen();
  for (x=0;x<=10;x++){
    for (y=0;y<=8;y++){
      if (real[x][y])
        {blit(x*50+100,y*50+100,real_pic);}
    }
  }
  for (x=0;x<=10;x++){
    for (y=0;y<=8;y++){
      if (think[x][y])
        {blit(x*50+100,y*50+100,think_pic);}
    }
  }
  SDL_UpdateRect(screen,0,0,0,0);
}

void generate_field()
{
  int x,y;
  
  for (x=0;x<=10;x++){
    for (y=0;y<=8;y++){
      think[x][y]=0;
      light[x][y]=0;
    }
  }
  for (;;)
  {
    int crystal_num=0;
    srand(level_num);
    for (x=0;x<=9;x++){
      for (y=0;y<=7;y++){
        if (abrand(0,10)==0) {real[x][y]=1;}
          else {real[x][y]=0;}
      }
    }
    for (x=0;x<=9;x++){
      for (y=0;y<=7;y++){
        if (real[x][y]==1) {crystal_num++;}
      }
    }  
    if (LevelGiven) {
	LevelGiven=0;
	break;
    }
    if (((difficulty-1)*3<=crystal_num)&&(crystal_num<=difficulty*3)) {
        break;
    }
    level_num=(level_num+1)%100000;
//    printf("%d\n",crystal_num); 
  }
  sprintf(text,"Black-Box: Level #%d",level_num);
  for (x=0;x<=10;x++)
  {
    yline[x]=0;
  }
  for (y=0;y<=8;y++)
  {
    xline[y]=0;
  }
  SDL_WM_SetCaption(text,"Black-Box"); 
  won_shown=0;
}

void tex(int x, int y, char *text)
{
  Text(screen,&font,x,y,1150,text,-1);
}

void num(int x, int y, int numb)
{
  char text[30];  
                                                                     
  sprintf(text,"%d",numb);
  Text(screen,&font,x,y,1150,text,-1);
}

void demo()
{
  SDL_Rect rect;
  int end=0,end2=0,show=0;

  blit(695,10,ok_pic);
  while (!end)
  {
  rect.x=0;
  rect.y=0;
  rect.w=700;
  rect.h=600;
  SDL_FillRect(screen,&rect,0);
  blit(240,100,demo_pic[show]);  
  blit(220,310,prev_pic);  
  blit(400,310,next_pic);  
  XCenteredString(screen,400,"For more info visit http://www.linux-games.com");
  XCenteredString(screen,420,"or write a mail to Karl Bartel <karlb@gmx.net>");
  XCenteredString(screen,20,"BLACK-BOX HELP & OPTIONS");
  tex(150,500,"DIFFICULTY:");
  num(190,520,difficulty);
  tex(275,500,"HIGHLIGHT:");
  if (trihigh) tex(310,520,"YES");
  else tex(312,520,"No");
  tex(400,500,"SOUND:");
  if (sound) tex(410,520,"YES");
  else tex(412,520,"No");
  SDL_UpdateRect(screen,0,0,0,0);
  while (( SDL_WaitEvent(&event) >= 0 )&&(!end2)) {
    switch (event.type) {
      case SDL_MOUSEMOTION: {
        SDL_GetMouseState(&mouse_x, &mouse_y);
        }
        break;
      case SDL_MOUSEBUTTONDOWN:
        {end2=1;}
        break;
      case SDL_QUIT: {
        printf("Quit requested, quitting...\n");
        free_memory();
        exit(0);
      }
      break;
    }
  }end2=0;
//buttons
    if ((mouse_x>150)&&(mouse_y>500)&&(mouse_y<600)&&(mouse_x<250))
    {
      if (difficulty<6) difficulty++; else difficulty=1;
      level_num=(((unsigned)time(NULL)+SDL_GetTicks())%100000);
      generate_field();
    }
    if ((mouse_x>280)&&(mouse_y>500)&&(mouse_y<600)&&(mouse_x<400))
    {
      if (trihigh) trihigh=0; else trihigh=1;
    }
    if ((mouse_x>400)&&(mouse_y>500)&&(mouse_y<600)&&(mouse_x<500))
    {
      if (sound) {sound=0; Mix_HaltMusic();} 
          else {sound=1; Mix_PlayMusic(music,1);}
    }
    if ((mouse_x>220)&&(mouse_y>310)&&(mouse_y<400)&&(mouse_x<390))
    {
      if (show) show--;
    }
    if ((mouse_x>390)&&(mouse_y>310)&&(mouse_y<400)&&(mouse_x<620))
    {
      if (show<7) show++;
    }
    if ((mouse_x>700)&&(mouse_y<120)&&(mouse_y>75))
    {
      level_num=(((unsigned)time(NULL)+SDL_GetTicks())%100000);
      generate_field();
      end=1;
    }
    if ((mouse_x>700)&&(mouse_y>530))
    {
      printf("Quit pressed, quitting...\n");
      free_memory();
      exit(0);
    } 
    if ((mouse_x>700)&&(mouse_y<70))
    {
      end=1;
      blit_screen();
    }
  }
  this_turn_demo=1;
  SDL_FillRect(screen,&rect,0);
  blit(695,10,giveup_pic);
  init_blit();
  SDL_Delay(100);
}

int IsItComplete()
{
  int x,y,won=1;
  for (x=0;x<=11;x++)
  {
    if (calc_out(real,x,0,2).x!=calc_out(think,x,0,2).x) {won=0;}
    if (calc_out(real,x,0,2).y!=calc_out(think,x,0,2).y) {won=0;}
    if (calc_out(real,x,0,2).dir!=calc_out(think,x,0,2).dir) {won=0;}
  }
  for (x=0;x<=11;x++)
  {
    if (calc_out(real,x,9,0).x!=calc_out(think,x,9,0).x) {won=0;}
    if (calc_out(real,x,9,0).y!=calc_out(think,x,9,0).y) {won=0;}
    if (calc_out(real,x,9,0).dir!=calc_out(think,x,9,0).dir) {won=0;}
  }
  for (y=0;y<=9;y++)
  {
    if (calc_out(real,0,y,1).x!=calc_out(think,0,y,1).x) {won=0;}
    if (calc_out(real,0,y,1).y!=calc_out(think,0,y,1).y) {won=0;}
    if (calc_out(real,0,y,1).dir!=calc_out(think,0,y,1).dir) {won=0;}
  }
  return(won);  
}

void click()
{
  int x,y;
  
  switch(SDL_GetMouseState(&mouse_x, &mouse_y))
  {
//block highlighting
  case SDL_BUTTON(3):
    if ((light[div(mouse_x+25,50).quot-2][div(mouse_y+25,50).quot-2]==0)
       &&(div(mouse_x+25,50).quot>1)&&(div(mouse_y+25,50).quot>1)
       &&(div(mouse_x+25,50).quot<13)&&(div(mouse_y+25,50).quot<11))
    {
      light[div(mouse_x+25,50).quot-2][div(mouse_y+25,50).quot-2]=1;
    }else
    if ((light[div(mouse_x+25,50).quot-2][div(mouse_y+25,50).quot-2]==1)
       &&(div(mouse_x+25,50).quot>1)&&(div(mouse_y+25,50).quot>1)
       &&(div(mouse_x+25,50).quot<13)&&(div(mouse_y+25,50).quot<11))
    {
      light[div(mouse_x+25,50).quot-2][div(mouse_y+25,50).quot-2]=0;
    }
//line highlighting    
    if ((div(mouse_x+25,50).quot<2)
       &&(div(mouse_y+25,50).quot>1)&&(div(mouse_y+25,50).quot<11))
    {
      if (xline[div(mouse_y+25,50).quot-2]==0)
      {
        for (x=0;x<11;x++)
        {
          light[x][div(mouse_y+25,50).quot-2]=1;
        }
	xline[div(mouse_y+25,50).quot-2]=1;
      }
      else{
        for (x=0;x<11;x++)
        {
          light[x][div(mouse_y+25,50).quot-2]=0;
        }
	xline[div(mouse_y+25,50).quot-2]=0;
      }      
    }
    if (((div(mouse_y+25,50).quot<2)&&(div(mouse_x+25,50).quot>1)
       &&(div(mouse_x+25,50).quot<13))||((div(mouse_y+25,50).quot>10)
       &&(div(mouse_x+25,50).quot>1)&&(div(mouse_x+25,50).quot<13)))
    {
      if (yline[div(mouse_x+25,50).quot-2]==0)
      {
        for (y=0;y<9;y++)
        {
          light[div(mouse_x+25,50).quot-2][y]=1;
        }
	yline[div(mouse_x+25,50).quot-2]=1;
      }
      else{
        for (y=0;y<9;y++)
        {
          light[div(mouse_x+25,50).quot-2][y]=0;
        }
	yline[div(mouse_x+25,50).quot-2]=0;
      }      
    }
  break;
  case SDL_BUTTON(1):
//think
    if ((think[div(mouse_x,50).quot-2][div(mouse_y,50).quot-2]==0)
       &&(div(mouse_x,50).quot>1)&&(div(mouse_y,50).quot>1)
       &&(div(mouse_x,50).quot<12)&&(div(mouse_y,50).quot<10))
    {
      think[div(mouse_x,50).quot-2][div(mouse_y,50).quot-2]=1;
    } else
    if ((think[div(mouse_x,50).quot-2][div(mouse_y,50).quot-2]==1)
       &&(div(mouse_x,50).quot>1)&&(div(mouse_y,50).quot>1)
       &&(div(mouse_x,50).quot<12)&&(div(mouse_y,50).quot<10))
    {
      think[div(mouse_x,50).quot-2][div(mouse_y,50).quot-2]=0;
    }
//shot  
    if ((div(mouse_x+25,50).quot<2)
       &&(div(mouse_y+25,50).quot>1)&&(div(mouse_y+25,50).quot<11))
    {
      hidden(0,div(mouse_y+25,50).quot-2,1);
    }
    if ((div(mouse_y+25,50).quot<2)
       &&(div(mouse_x+25,50).quot>1)&&(div(mouse_x+25,50).quot<13))
    {
      hidden(div(mouse_x+25,50).quot-2,0,2);
    }
    if ((div(mouse_y+25,50).quot>10)
       &&(div(mouse_x+25,50).quot>1)&&(div(mouse_x+25,50).quot<13))
    {
      hidden(div(mouse_x+25,50).quot-2,9,0);
    }
//buttons
    if ((mouse_x>700)&&(mouse_y<120)&&(mouse_y>75))
    {
      level_num=(((unsigned)time(NULL)+SDL_GetTicks())%100000);
      generate_field();
      init_blit();
      blit_screen();
    }
    if ((mouse_x>700)&&(mouse_y>530))
    {
      printf("Quit pressed, quitting...\n");
      free_memory();
      exit(1);
    } 
    if ((mouse_x>700)&&(mouse_y<530)&&(mouse_y>480))
    {
      demo();
    } 
  break;
  }
  blit_screen();
  if ((mouse_x>700)&&(mouse_y<70)&&(!this_turn_demo))
  {
    //Give up
    show_real();
  }
  this_turn_demo=0;
}

int abrand(int a,int b)  //random number between a and b (inclusive)
{
  return(a+(rand() % (b-a+1)));
}

void get_click()
{
  while ( SDL_WaitEvent(&event) >= 0 ) {
    if ((!Mix_PlayingMusic())&&(sound)) {Mix_PlayMusic(music,1);}
    switch (event.type) {
      case SDL_MOUSEMOTION:
        SDL_GetMouseState(&mouse_x, &mouse_y);
        break;
      case SDL_MOUSEBUTTONDOWN:
        click();
        break;
      case SDL_QUIT: {
        printf("Quit requested, quitting...\n");
        free_memory();
        exit(0);}
        break;
    }
    if ((IsItComplete())&&(!won_shown)) {
      won_shown=1;
      blit(150,200,won_pic);
      SDL_UpdateRect(screen,0,0,0,0);
    }
  }
}

void init_sound()
{
  if ( Mix_OpenAudio(44100, AUDIO_S16, 2, 1024) < 0 )
  {
    fprintf(stderr,"Warning: Couldn't set 22025 Hz 16-bit audio\n- Reason: %s\n",SDL_GetError());
    fprintf(stderr,"\t**\nSOUND TURNED OFF\n\t**\n");
  }else{
    sprintf(text,"%s/sound/ein1.mod",DATAPATH);
    music = Mix_LoadMUS(text);
    if (music==NULL) {printf("COULD NOT LOAD MUSIC\n");ComplainAndExit();}
  }
}

main(int argc, char *argv[])
{
  printf("\nBlack-Box version %s, Copyright (C) 2000-2008 Karl Bartel\n",VERSION);
  printf("Black-Box comes with ABSOLUTELY NO WARRANTY; for details see COPYING'.\n");
  printf("This is free software, and you are welcome to redistribute it\n");
  printf("under certain conditions.\n");
  level_num=((unsigned)time(NULL)%100000);
  ReadCommandLine(argv);
  init_SDL();
  load_images();
  InitFont(Font);
  if (sound) init_sound();
  sprintf(text,"%s/font.scl",DATAPATH);
  generate_field();
  blit_screen();
  init_blit();
  blit_screen();
  get_click();
  free_memory();
  exit(0);
}
