//System Headers
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

//Other Headers
#include "SDL/SDL.h"
#include "SDL/SDL_keyboard.h"
#include "SDL/SDL_keysym.h"
#include "audiere.h"
#include "tones.h"

#define TBACKOFF 3

audiere::AudioDevicePtr device;
#define NUM_FREQS 9
//float frequencies[] = {A3, B3, C4, D4, E4, F4, G4, A4, B4, C5, D5, E5, F5, G5, A5, B5, C6, D6, E6, F6, G6, A6, B6, C7, D7, E7, F7, G7, A7, B7, C8}; //c major scale
//float frequencies[] = {A3,  D4, G4, C5, F5, B5, E6, A6, D7, G7, C8}; //ascending 4ths c major scale
float frequencies[] = {C4, D4, E4, Fsharp5, Gsharp5, Asharp5, C5, D5, E5}; //whole tones
//float frequencies[] = {A6, B6, C7, D7, E7, F7, G7, A7, B7, C8}; //c major scale

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;
float longest = 0;
const int SCREEN_BPP = 32;
const int FRAMES_PER_SECOND = 10;
const int BARLIFETIME = 50;
const int NUMROWS = 3;
const int NUMCOLS = 3;

const SDLKey keymap3[9] = {SDLK_q, SDLK_a, SDLK_z,
                           SDLK_w, SDLK_s, SDLK_x,
                           SDLK_e, SDLK_d, SDLK_c};
//Surfaces
SDL_Surface *canvas = NULL;
SDL_Surface *screen = NULL;

SDL_Event event;

//Timer
class Timer{

    private:

    //The clock time when the timer started
    int startTicks;


    //The ticks stored when the timer was paused
    int pausedTicks;


    //The timer status
    bool paused;
    bool started;


    public:

    //Initializes variables
    Timer();


    //The various clock actions
    void start();
    void stop();
    void pause();
    void unpause();


    //Gets the timer's time
    int get_ticks();


    //Checks the status of the timer
    bool is_started();
    bool is_paused();

};

typedef struct _Col{
  int r;
  int g;
  int b;
} ChangoColor;

typedef struct _Pt{
  int x;
  int y;
} ChangoPoint;


class ChangoGrid;//Fwd Declaration

class ChangoCell{

public:
  int id;

  SDL_Rect rect;
  ChangoColor col;
  ChangoPoint ctr;
  SDLKey key; 
  float lastKnownAmplitude;

  bool on;
  int toggleBackoff;
  bool bent;

  ChangoGrid *mom;
  audiere::OutputStreamPtr toneGen;

  ChangoCell(int,int,int,int,int,ChangoGrid *);
  ChangoCell();

  void Draw(SDL_Surface *c);
  void Update(int mousex, int mousey);
  void KeyUpdate(Uint8 *keystates);

};

class ChangoGrid{

public:

  int numrows;
  int numcols;
  int cellwidth;
  int cellheight;

  ChangoCell *cells;
  int *distanceGrid;
  ChangoGrid(int NumRows, int NumCols);
  void DrawAll(SDL_Surface *c); 

};

ChangoCell::ChangoCell(){
  ctr.x = ctr.y = rect.x = rect.y = rect.h = rect.w = id = col.r = col.g = col.b = 0;
  mom = NULL;
  bent = false;
}

ChangoCell::ChangoCell(int x, int y, int w, int h, int ID,ChangoGrid *m){


  on = true;
  bent = false;
  toggleBackoff = 0;
  id = ID;

  rect.x = x;
  rect.y = y;
  rect.h = h;
  rect.w = w;

  mom = m;
  col.r = col.g = col.b = 0;
  ctr.x = x + (((float)mom->cellwidth) / 2);
  ctr.y = y + (((float)mom->cellheight) / 2);

  if(mom->numrows == 3){
    key = keymap3[ID];//only works for 3 for now.
  }else{
    key = SDLK_UNKNOWN;
  }
  
  toneGen = audiere::OpenSound(device,audiere::CreateTone(frequencies[ID % NUM_FREQS]));
  if(toneGen == NULL){
    fprintf(stderr,"ERROR CREATING TONEGEN %d\n",ID);
  }

}

void ChangoCell::Draw(SDL_Surface *c){

  SDL_FillRect( c, &rect, SDL_MapRGB( screen->format, col.r, col.g, col.b )  );

}

void ChangoCell::KeyUpdate(Uint8 *keystates){

  if( this->toggleBackoff > 0){

    this->toggleBackoff--;

  }

  if( keystates[ this->key ] == 1 ){

    if( keystates[ SDLK_LSHIFT ] == 1){ 

      if(this->toggleBackoff == 0){
        this->toggleBackoff = TBACKOFF;
        if(this->on){

          this->col.b = 0;
          this->col.g = 0;
          this->on = false;
          this->bent = false;
          if(this->toneGen->isPlaying()){
            this->toneGen->stop();
          }
          

        }else{
          
          this->col.b = 255.0 * this->lastKnownAmplitude;
          this->on = true;
          this->bent = false;
          if(!this->toneGen->isPlaying()){
            this->toneGen->setVolume( (.7 - lastKnownAmplitude) / mom->numrows );
            this->toneGen->play();
          }

        }
      }

    }else{

      bent = true;

    }

  }else{
    bent = false;
  }

}

void ChangoCell::Update(int mousex, int mousey){


  float xDiff = (float)mousex - (float)ctr.x;
  float yDiff = (float)mousey - (float)ctr.y;

  float scaleFactor = (( sqrt(xDiff * xDiff + yDiff * yDiff) ) / ((float)longest));
  lastKnownAmplitude = scaleFactor; 

  if(!this->on){

    this->col.b = 0;
    if(this->toneGen->isPlaying()){
      this->toneGen->stop();
    }

  }else{
 
    if(this->bent){
      this->col.g = 100;
      this->toneGen->setPitchShift(2.0);
    }else{
      this->col.g = 0;
      this->toneGen->setPitchShift(1.0);
    }
   
    this->col.b = 255.0  * (scaleFactor);
  
    if(scaleFactor > 0){
  
      toneGen->setVolume((0.7 - scaleFactor) / ((float)mom->numrows));
  
      if(!toneGen->isPlaying()){
  
        toneGen->play();
  
      }
  
    }else{
  
      toneGen->stop();
  
    }

  }

}



void ChangoGrid::DrawAll(SDL_Surface *c){

  for(int i = 0; i < numrows; i++){

    for(int j = 0; j < numcols; j++){

      cells[i * numcols + j].Draw(c);

    }

  }

}

ChangoGrid::ChangoGrid(int NumRows, int NumCols){

  //this->distanceGrid = new int[NumRows * NumCols * NumRows * NumCols];
  this->cells = new ChangoCell[NumRows * NumCols]();
  this->numrows = NumRows;
  this->numcols = NumCols;
  this->cellwidth = SCREEN_WIDTH / this->numcols;
  this->cellheight = SCREEN_HEIGHT / this->numrows;
  
  for(int i = 0; i < this->numrows; i++){

    for(int j = 0; j < this->numcols; j++){
      
      this->cells[i * numcols + j] = ChangoCell(i * cellwidth, j * cellheight, cellwidth, cellheight, i * this->numcols + j, this);
  
    }

  }

}




bool startup()
{

    if( SDL_Init( SDL_INIT_EVERYTHING ) == -1 ){
        return false;
    }

    screen = SDL_SetVideoMode( SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_BPP, SDL_SWSURFACE );

    if( screen == NULL ){
        return false;
    }

    SDL_WM_SetCaption( "Digi-Chango", NULL );

    return true;

}


void shutdown(){

    SDL_FreeSurface( canvas );
    SDL_Quit();

}


Timer::Timer() {

    //Initialize the variables
    startTicks = 0;
    pausedTicks = 0;
    paused = false;
    started = false;
}



void Timer::start() {

    //Start the timer
    started = true;


    //Unpause the timer
    paused = false;


    //Get the current clock time
    startTicks = SDL_GetTicks();

}



void Timer::stop()

{

    //Stop the timer
    started = false;

    //Unpause the timer
    paused = false;

}



void Timer::pause()

{

    //If the timer is running and isn't already paused
    if( ( started == true ) && ( paused == false ) ) {

        //Pause the timer
        paused = true;

        //Calculate the paused ticks
        pausedTicks = SDL_GetTicks() - startTicks;

    }

}



void Timer::unpause()

{

    //If the timer is paused
    if( paused == true ) {

        //Unpause the timer
        paused = false;

        //Reset the starting ticks
        startTicks = SDL_GetTicks() - pausedTicks;


        //Reset the paused ticks
        pausedTicks = 0;

    }

}



int Timer::get_ticks()

{

    //If the timer is running
    if( started == true ) {

        //If the timer is paused
        if( paused == true ) {

            //Return the number of ticks when the timer was paused
            return pausedTicks;

        } else {

            //Return the current time minus the start time
            return SDL_GetTicks() - startTicks;

        }

    }


    //If the timer isn't running
    return 0;

}



bool Timer::is_started() {

    return started;

}



bool Timer::is_paused() {

    return paused;

}



int main( int argc, char* args[] )

{

    //Quit flag

    bool quit = false;

    longest = sqrt(SCREEN_WIDTH * SCREEN_WIDTH + SCREEN_HEIGHT * SCREEN_HEIGHT);

    const char* device_name = "";
    device = audiere::OpenDevice(device_name);
    if(!device){
      fprintf(stderr,"Failed Creating a device\n");
    }

    ChangoGrid G = ChangoGrid(NUMROWS,NUMCOLS);


    //The frame rate regulator
    Timer fps;

    //Initialize
    if( startup() == false ) {
        return 1;
    }

    //While the user hasn't quit
    while( quit == false ) {

      //Start the frame timer
      fps.start();

      
      Uint8 *keystates = SDL_GetKeyState(NULL);
      int x; int y; 
      Uint8 bmask = SDL_GetMouseState(&x,&y);

      for(int i = 0; i < G.numrows * G.numcols; i++){
        G.cells[i].KeyUpdate(keystates);
        G.cells[i].Update(x,y);
      }


      //While there's events to handle
      while( SDL_PollEvent( &event ) ) {

        /* 
        if( event.type == SDL_MOUSEMOTION ){

          int x = event.motion.x;
          int y = event.motion.y;
     
          for(int i = 0; i < G.numrows * G.numcols; i++){
            G.cells[i].Update(x,y);
          } 

        }
        */
 
        //If the user has Xed out the window
        if( event.type == SDL_QUIT ) {

          //Quit the program
          quit = true;

        }

      }
       
      SDL_FillRect( screen, NULL, SDL_MapRGB( screen->format, 0x00, 0x00, 0x00));
      G.DrawAll(screen); //canvas?

      //Update the screen
      if( SDL_Flip( screen ) == -1 ) {
        return 1; 
      }

        //Cap the frame rate
      if( fps.get_ticks() < 1000 / FRAMES_PER_SECOND ) {
        SDL_Delay( ( 1000 / FRAMES_PER_SECOND ) - fps.get_ticks() );
      }

  }

  //Clean up
  shutdown();
  return 0;
}


